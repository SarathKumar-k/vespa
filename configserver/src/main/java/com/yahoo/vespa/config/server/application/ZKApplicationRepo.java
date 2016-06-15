// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.application;

import com.yahoo.concurrent.ThreadFactoryFactory;
import com.yahoo.config.provision.ApplicationId;
import com.yahoo.config.provision.TenantName;
import com.yahoo.log.LogLevel;
import com.yahoo.path.Path;
import com.yahoo.text.Utf8;
import com.yahoo.transaction.Transaction;
import com.yahoo.vespa.config.server.ReloadHandler;
import com.yahoo.vespa.config.server.Tenants;
import com.yahoo.vespa.curator.Curator;
import com.yahoo.vespa.curator.transaction.CuratorOperations;
import com.yahoo.vespa.curator.transaction.CuratorTransaction;
import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.recipes.cache.PathChildrenCacheEvent;
import org.apache.curator.framework.recipes.cache.PathChildrenCacheListener;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.logging.Logger;

/**
 * Application repo backed by zookeeper.
 *
 * @author lulf
 * @since 5.1
 */
public class ZKApplicationRepo implements ApplicationRepo, PathChildrenCacheListener {

    private static final Logger log = Logger.getLogger(ZKApplicationRepo.class.getName());
    private final Curator curator;
    private final Path root;
    private final ExecutorService pathChildrenExecutor = Executors.newFixedThreadPool(1, ThreadFactoryFactory.getThreadFactory(ZKApplicationRepo.class.getName()));
    private final Curator.DirectoryCache directoryCache;
    private final ReloadHandler reloadHandler;
    private final TenantName tenant;

    private ZKApplicationRepo(Curator curator, Path root, ReloadHandler reloadHandler, TenantName tenant) throws Exception {
        this.curator = curator;
        this.root = root;
        this.reloadHandler = reloadHandler;
        this.tenant = tenant;
        rewriteApplicationIds();
        this.directoryCache = curator.createDirectoryCache(root.getAbsolute(), false, false, pathChildrenExecutor);
        this.directoryCache.start();
        this.directoryCache.addListener(this);
    }

    private void rewriteApplicationIds() {
        try {
            List<String> appNodes = curator.framework().getChildren().forPath(root.getAbsolute());
            for (String appNode : appNodes) {
                Optional<ApplicationId> appId = parseApplication(appNode);
                appId.filter(id -> shouldBeRewritten(appNode, id))
                     .ifPresent(id -> rewriteApplicationId(id, appNode, readSessionId(id, appNode)));
            }
        } catch (Exception e) {
            log.log(LogLevel.WARNING, "Error rewriting application ids on upgrade", e);
        }
    }

    private long readSessionId(ApplicationId appId, String appNode) {
        String path = root.append(appNode).getAbsolute();
        try {
            return Long.parseLong(Utf8.toString(curator.framework().getData().forPath(path)));
        } catch (Exception e) {
            throw new IllegalArgumentException(Tenants.logPre(appId) + "Unable to read the session id from '" + path + "'", e);
        }
    }

    private boolean shouldBeRewritten(String appNode, ApplicationId appId) {
        return !appNode.equals(appId.serializedForm());
    }

    private void rewriteApplicationId(ApplicationId appId, String origNode, long sessionId) {
        String newPath = root.append(appId.serializedForm()).getAbsolute();
        String oldPath = root.append(origNode).getAbsolute();
        try (CuratorTransaction transaction = new CuratorTransaction(curator)) {
            if (curator.framework().checkExists().forPath(newPath) == null) {
                transaction.add(CuratorOperations.create(newPath, Utf8.toAsciiBytes(sessionId)));
            }
            transaction.add(CuratorOperations.delete(oldPath));
            transaction.commit();
        } catch (Exception e) {
            log.log(LogLevel.WARNING, "Error rewriting application id from " + origNode + " to " + appId.serializedForm());
        }
    }

    public static ApplicationRepo create(Curator curator, Path root, ReloadHandler reloadHandler, TenantName tenant) {
        try {
            return new ZKApplicationRepo(curator, root, reloadHandler, tenant);
        } catch (Exception e) {
            throw new RuntimeException(Tenants.logPre(tenant)+"Error creating application repo", e);
        }
    }

    @Override
    public List<ApplicationId> listApplications() {
        try {
            List<String> appNodes = curator.framework().getChildren().forPath(root.getAbsolute());
            List<ApplicationId> applicationIds = new ArrayList<>();
            for (String appNode : appNodes) {
                parseApplication(appNode).ifPresent(applicationIds::add);
            }
            return applicationIds;
        } catch (Exception e) {
            throw new RuntimeException(Tenants.logPre(tenant)+"Unable to list applications", e);
        }
    }

    private Optional<ApplicationId> parseApplication(String appNode) {
        try {
            return Optional.of(ApplicationId.fromSerializedForm(tenant, appNode));
        } catch (IllegalArgumentException e) {
            log.log(LogLevel.INFO, Tenants.logPre(tenant)+"Unable to parse application with id '" + appNode + "', ignoring.");
            return Optional.empty();
        }
    }

    @Override
    public Transaction createPutApplicationTransaction(ApplicationId applicationId, long sessionId) {
        if (listApplications().contains(applicationId)) {
            return new CuratorTransaction(curator).add(CuratorOperations.setData(root.append(applicationId.serializedForm()).getAbsolute(), Utf8.toAsciiBytes(sessionId)));
        } else {
            return new CuratorTransaction(curator).add(CuratorOperations.create(root.append(applicationId.serializedForm()).getAbsolute(), Utf8.toAsciiBytes(sessionId)));
        }
    }

    @Override
    public long getSessionIdForApplication(ApplicationId applicationId) {
        return readSessionId(applicationId, applicationId.serializedForm());
    }

    @Override
    public void deleteApplication(ApplicationId applicationId) {
        Path path = root.append(applicationId.serializedForm());
        curator.delete(path);
    }

    @Override
    public void close() {
        directoryCache.close();
        pathChildrenExecutor.shutdown();
    }


    @Override
    public void childEvent(CuratorFramework client, PathChildrenCacheEvent event) throws Exception {
        switch (event.getType()) {
            case CHILD_ADDED:
                applicationAdded(ApplicationId.fromSerializedForm(tenant, Path.fromString(event.getData().getPath()).getName()));
                break;
            case CHILD_REMOVED:
                applicationRemoved(ApplicationId.fromSerializedForm(tenant, Path.fromString(event.getData().getPath()).getName()));
                break;
        }
    }

    private void applicationRemoved(ApplicationId applicationId) {
        reloadHandler.removeApplication(applicationId);
        log.log(LogLevel.DEBUG, Tenants.logPre(applicationId)+"Application removed: " + applicationId);
    }

    private void applicationAdded(ApplicationId applicationId) {
        log.log(LogLevel.DEBUG, Tenants.logPre(applicationId)+"Application " + applicationId + " was added to repo");
    }    
}
