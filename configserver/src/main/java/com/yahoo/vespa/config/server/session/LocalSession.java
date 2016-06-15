// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.session;

import com.yahoo.config.application.api.ApplicationFile;
import com.yahoo.config.application.api.ApplicationPackage;
import com.yahoo.config.application.api.ApplicationMetaData;
import com.yahoo.config.application.api.DeployLogger;
import com.yahoo.config.provision.ProvisionInfo;
import com.yahoo.transaction.Transaction;
import com.yahoo.io.IOUtils;
import com.yahoo.path.Path;
import com.yahoo.vespa.config.server.*;
import com.yahoo.config.provision.ApplicationId;
import com.yahoo.config.provision.TenantName;
import com.yahoo.vespa.config.server.application.ApplicationRepo;
import com.yahoo.vespa.config.server.configchange.ConfigChangeActions;
import com.yahoo.vespa.curator.Curator;

import java.io.File;
import java.util.Optional;

/**
 * A LocalSession is a session that has been created locally on this configserver. A local session can be edited and
 * prepared. Deleting a local session will ensure that the local filesystem state and global zookeeper state is
 * cleaned for this session.
 *
 * @author lulf
 * @since 5.1
 */
public class LocalSession extends Session implements Comparable<LocalSession> {

    private final ApplicationPackage applicationPackage;
    private final ApplicationRepo applicationRepo;
    private final SessionZooKeeperClient zooKeeperClient;
    private final SessionPreparer sessionPreparer;
    private final SessionContext sessionContext;
    private final File serverDB;
    private final SuperModelGenerationCounter superModelGenerationCounter;

    /**
     * Create a session. This involves loading the application, validating it and distributing it.
     *
     * @param sessionId The session id for this session.
     */
    // TODO tenant in SessionContext?
    public LocalSession(TenantName tenant, long sessionId, SessionPreparer sessionPreparer, SessionContext sessionContext) {
        super(tenant, sessionId);
        this.serverDB = sessionContext.getServerDBSessionDir();
        this.applicationPackage = sessionContext.getApplicationPackage();
        this.zooKeeperClient = sessionContext.getSessionZooKeeperClient();
        this.applicationRepo = sessionContext.getApplicationRepo();
        this.sessionPreparer = sessionPreparer;
        this.sessionContext = sessionContext;
        this.superModelGenerationCounter = sessionContext.getSuperModelGenerationCounter();
    }

    public ConfigChangeActions prepare(DeployLogger logger, PrepareParams params, Optional<ApplicationSet> currentActiveApplicationSet, Path tenantPath) {
        Curator.CompletionWaiter waiter = zooKeeperClient.createPrepareWaiter();
        ConfigChangeActions actions = sessionPreparer.prepare(sessionContext, logger, params, currentActiveApplicationSet, tenantPath);
        setPrepared();
        waiter.awaitCompletion(params.getTimeoutBudget().timeLeft());
        return actions;
    }

    public ApplicationFile getApplicationFile(Path relativePath, Mode mode) {
        if (mode.equals(Mode.WRITE)) {
            markSessionEdited();
        }
        return applicationPackage.getFile(relativePath);
    }

    private void setPrepared() {
        setStatus(Session.Status.PREPARE);
    }

    private Transaction setActive() {
        Transaction transaction = createSetStatusTransaction(Status.ACTIVATE);
        transaction.add(applicationRepo.createPutApplicationTransaction(zooKeeperClient.readApplicationId(getTenant()), getSessionId()).operations());
        return transaction;
    }

    private Transaction createSetStatusTransaction(Status status) {
        return zooKeeperClient.createWriteStatusTransaction(status);
    }

    public Session.Status getStatus() {
        return zooKeeperClient.readStatus();
    }

    private void setStatus(Session.Status newStatus) {
        zooKeeperClient.writeStatus(newStatus);
    }

    public Transaction createActivateTransaction() {
        zooKeeperClient.createActiveWaiter();
        superModelGenerationCounter.increment();
        return setActive();
    }

    public Transaction createDeactivateTransaction() {
        return createSetStatusTransaction(Status.DEACTIVATE);
    }

    private void markSessionEdited() {
        setStatus(Session.Status.NEW);
    }

    public long getActiveSessionAtCreate() {
        return applicationPackage.getMetaData().getPreviousActiveGeneration();
    }

    // Note: Assumes monotonically increasing session ids
    public boolean isNewerThan(long sessionId) {
        return getSessionId() > sessionId;
    }

    /**
     * Deletes this session from ZooKeeper and filesystem, as well as making sure the supermodel generation counter is incremented.
     */
    public void delete() {
        superModelGenerationCounter.increment();
        IOUtils.recursiveDeleteDir(serverDB);
        zooKeeperClient.delete();
    }

    @Override
    public int compareTo(LocalSession rhs) {
        Long lhsId = getSessionId();
        Long rhsId = rhs.getSessionId();
        return lhsId.compareTo(rhsId);
    }

    // in seconds
    public long getCreateTime() {
        return zooKeeperClient.readCreateTime();
    }

    public void waitUntilActivated(TimeoutBudget timeoutBudget) {
        zooKeeperClient.getActiveWaiter().awaitCompletion(timeoutBudget.timeLeft());
    }

    public void setApplicationId(ApplicationId applicationId) {
        zooKeeperClient.writeApplicationId(applicationId);
    }

    public enum Mode {
        READ, WRITE
    }

    public ApplicationMetaData getMetaData() {
        return applicationPackage.getMetaData();
    }

    public ApplicationId getApplicationId() {
        return zooKeeperClient.readApplicationId(getTenant());
    }

    public ProvisionInfo getProvisionInfo() {
        return zooKeeperClient.getProvisionInfo();
    }

    @Override
    public String logPre() {
        if (getApplicationId().equals(ApplicationId.defaultId())) {
            return Tenants.logPre(getTenant());
        } else {
            return Tenants.logPre(getApplicationId());
        }
    }
}
