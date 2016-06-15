// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.session;

import com.google.common.collect.ImmutableList;
import com.yahoo.cloud.config.ConfigserverConfig;
import com.yahoo.config.application.api.ApplicationPackage;
import com.yahoo.config.application.api.DeployLogger;
import com.yahoo.config.application.api.FileRegistry;
import com.yahoo.config.model.api.ConfigDefinitionRepo;
import com.yahoo.config.model.api.ModelContext;
import com.yahoo.config.provision.*;
import com.yahoo.log.LogLevel;
import com.yahoo.path.Path;
import com.yahoo.vespa.config.server.ApplicationSet;
import com.yahoo.vespa.config.server.ConfigServerSpec;
import com.yahoo.vespa.config.server.application.PermanentApplicationPackage;
import com.yahoo.vespa.config.server.modelfactory.ModelFactoryRegistry;
import com.yahoo.vespa.config.server.RotationsCache;
import com.yahoo.vespa.config.server.configchange.ConfigChangeActions;
import com.yahoo.vespa.config.server.deploy.ModelContextImpl;
import com.yahoo.vespa.config.server.deploy.ZooKeeperDeployer;
import com.yahoo.vespa.config.server.http.InvalidApplicationException;
import com.yahoo.vespa.config.server.modelfactory.PreparedModelsBuilder;
import com.yahoo.vespa.config.server.provision.HostProvisionerProvider;

import com.yahoo.vespa.curator.Curator;
import org.xml.sax.SAXException;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.logging.Logger;
import java.util.stream.Collectors;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.TransformerException;

/**
 * A SessionPreparer is responsible for preparing a session given an application package.
 *
 * @author lulf
 * @since 5.1
 */
public class SessionPreparer {

    private static final Logger log = Logger.getLogger(SessionPreparer.class.getName());

    private final ModelFactoryRegistry modelFactoryRegistry;
    private final FileDistributionFactory fileDistributionFactory;
    private final HostProvisionerProvider hostProvisionerProvider;
    private final PermanentApplicationPackage permanentApplicationPackage;
    private final ConfigserverConfig configserverConfig;
    private final ConfigDefinitionRepo configDefinitionRepo;
    private final Curator curator;
    private final Zone zone;

    public SessionPreparer(ModelFactoryRegistry modelFactoryRegistry,
                           FileDistributionFactory fileDistributionFactory,
                           HostProvisionerProvider hostProvisionerProvider,
                           PermanentApplicationPackage permanentApplicationPackage,
                           ConfigserverConfig configserverConfig,
                           ConfigDefinitionRepo configDefinitionRepo,
                           Curator curator,
                           Zone zone) {
        this.modelFactoryRegistry = modelFactoryRegistry;
        this.fileDistributionFactory = fileDistributionFactory;
        this.hostProvisionerProvider = hostProvisionerProvider;
        this.permanentApplicationPackage = permanentApplicationPackage;
        this.configserverConfig = configserverConfig;
        this.configDefinitionRepo = configDefinitionRepo;
        this.curator = curator;
        this.zone = zone;
    }

    /**
     * Prepares a session (validates, builds model, writes to zookeeper and distributes files)
     *
     * @param context                     Contains classes needed to read/write session data.
     * @param logger                      For storing logs returned in response to client.
     * @param params                      parameters controlling behaviour of prepare.
     * @param currentActiveApplicationSet Set of currently active applications.
     * @param tenantPath Zookeeper path for the tenant for this session
     * @return The config change actions that must be done to handle the activation of the models prepared.
     */
    public ConfigChangeActions prepare(SessionContext context, DeployLogger logger, PrepareParams params,
                                       Optional<ApplicationSet> currentActiveApplicationSet, Path tenantPath)
    {
        Preparation prep = new Preparation(context, logger, params, currentActiveApplicationSet, tenantPath);
        prep.preprocess();
        try {
            prep.buildModels();
            prep.makeResult();
            if (!params.isDryRun()) {
                prep.writeStateZK();
                prep.writeRotZK();
                prep.distribute();
                prep.reloadDeployFileDistributor();
            }
            return prep.result();
        } catch (IllegalArgumentException e) {
            throw new InvalidApplicationException("Invalid application package", e);
        }
    }

    private class Preparation {

        final SessionContext context;
        final DeployLogger logger;
        final PrepareParams params;

        final Optional<ApplicationSet> currentActiveApplicationSet;
        final Path tenantPath;
        final ApplicationId applicationId;
        final RotationsCache rotationsCache;
        final Set<Rotation> rotations;
        final ModelContext.Properties properties;

        private ApplicationPackage applicationPackage;
        private List<PreparedModelsBuilder.PreparedModelResult> modelResultList;
        private PrepareResult prepareResult;

        private final PreparedModelsBuilder preparedModelsBuilder;

        Preparation(SessionContext context, DeployLogger logger, PrepareParams params,
                    Optional<ApplicationSet> currentActiveApplicationSet, Path tenantPath) {
            this.context = context;
            this.logger = logger;
            this.params = params;
            this.currentActiveApplicationSet = currentActiveApplicationSet;
            this.tenantPath = tenantPath;

            this.applicationId = params.getApplicationId();
            this.rotationsCache = new RotationsCache(curator, tenantPath);
            this.rotations = getRotations(params.rotations());
            this.properties = new ModelContextImpl.Properties(params.getApplicationId(),
                                                              configserverConfig.multitenant(),
                                                              ConfigServerSpec.fromConfig(configserverConfig),
                                                              configserverConfig.hostedVespa(),
                                                              zone,
                                                              rotations);
            this.preparedModelsBuilder = new PreparedModelsBuilder(modelFactoryRegistry,
                                                                   permanentApplicationPackage,
                                                                   configserverConfig,
                                                                   configDefinitionRepo,
                                                                   curator,
                                                                   zone,
                                                                   fileDistributionFactory,
                                                                   hostProvisionerProvider,
                                                                   context,
                                                                   logger,
                                                                   params,
                                                                   currentActiveApplicationSet,
                                                                   tenantPath);
        }

        void checkTimeout(String step) {
            if (! params.getTimeoutBudget().hasTimeLeft()) {
                String used = params.getTimeoutBudget().timesUsed();
                throw new RuntimeException("prepare timed out "+used+" after "+step+" step: " + applicationId);
            }
        }

        void preprocess() {
            try {
                this.applicationPackage = context.getApplicationPackage().preprocess(
                        properties.zone(),
                        null,
                        logger);
            } catch (IOException | TransformerException | ParserConfigurationException | SAXException e) {
                throw new RuntimeException("Error deploying application package", e);
            }
            checkTimeout("preprocess");
        }

        void buildModels() {
            this.modelResultList = preparedModelsBuilder.buildModels(applicationId, applicationPackage);
            checkTimeout("build models");
        }

        void makeResult() {
            this.prepareResult = new PrepareResult(modelResultList);
            checkTimeout("making result from models");
        }

        void writeStateZK() {
            log.log(LogLevel.DEBUG, "Writing application package state to zookeeper");
            writeStateToZooKeeper(context.getSessionZooKeeperClient(), applicationPackage, params, logger,
                                  prepareResult.getFileRegistries(), prepareResult.getProvisionInfos());
            checkTimeout("write state to zookeeper");
        }

        void writeRotZK() {
            rotationsCache.writeRotationsToZooKeeper(applicationId, rotations);
            checkTimeout("write rotations to zookeeper");
        }

        void distribute() {
            prepareResult.asList().forEach(modelResult -> modelResult.model
                                           .distributeFiles(modelResult.fileDistributionProvider.getFileDistribution()));
            checkTimeout("distribute files");
        }

        void reloadDeployFileDistributor() {
            if (prepareResult.asList().isEmpty()) return;
            PreparedModelsBuilder.PreparedModelResult aModelResult = prepareResult.asList().get(0);
            aModelResult.model.reloadDeployFileDistributor(aModelResult.fileDistributionProvider.getFileDistribution());
            checkTimeout("reload all deployed files in file distributor");
        }

        ConfigChangeActions result() {
            return prepareResult.getConfigChangeActions();
        }

        private Set<Rotation> getRotations(Set<Rotation> rotations) {
            if (rotations == null || rotations.isEmpty()) {
                rotations = rotationsCache.readRotationsFromZooKeeper(applicationId);
            }
            return rotations;
        }

    }

    private void writeStateToZooKeeper(SessionZooKeeperClient zooKeeperClient,
                                       ApplicationPackage applicationPackage,
                                       PrepareParams prepareParams,
                                       DeployLogger deployLogger,
                                       Map<Version, FileRegistry> fileRegistryMap,
                                       Map<Version, ProvisionInfo> provisionInfoMap) {
        ZooKeeperDeployer zkDeployer = zooKeeperClient.createDeployer(deployLogger);
        try {
            zkDeployer.deploy(applicationPackage, fileRegistryMap, provisionInfoMap);
            zooKeeperClient.writeApplicationId(prepareParams.getApplicationId());
        } catch (RuntimeException | IOException e) {
            zkDeployer.cleanup();
            throw new RuntimeException("Error preparing session", e);
        }
    }

    /** The result of preparation over all model versions */
    private static class PrepareResult {

        private final ImmutableList<PreparedModelsBuilder.PreparedModelResult> results;

        public PrepareResult(List<PreparedModelsBuilder.PreparedModelResult> results) {
            this.results = ImmutableList.copyOf(results);
        }

        /** Returns the results for each model as an immutable list */
        public List<PreparedModelsBuilder.PreparedModelResult> asList() { return results; }

        public Map<Version, ProvisionInfo> getProvisionInfos() {
            return results.stream()
                    .filter(result -> result.model.getProvisionInfo().isPresent())
                    .collect(Collectors.toMap((prepareResult -> prepareResult.version),
                            (prepareResult -> prepareResult.model.getProvisionInfo().get())));
        }

        public Map<Version, FileRegistry> getFileRegistries() {
            return results.stream()
                    .collect(Collectors.toMap((prepareResult -> prepareResult.version),
                            (prepareResult -> prepareResult.fileDistributionProvider.getFileRegistry())));
        }

        /**
         * Collects the config change actions from all model factory creations and returns the aggregated union of these actions.
         * A system in the process of upgrading Vespa will have hosts running both version X and Y, and this will change
         * during the upgrade process. Trying to be smart about which actions to perform on which hosts depending
         * on the version running will be a nightmare to maintain. A pragmatic approach is therefore to just use the
         * union of all actions as this will give the correct end result at the cost of perhaps restarting nodes twice
         * (once for the upgrading case and once for a potential restart action).
         */
         public ConfigChangeActions getConfigChangeActions() {
            return new ConfigChangeActions(results.stream().
                map(result -> result.actions).
                flatMap(actions -> actions.stream()).
                collect(Collectors.toList()));
         }

    }

}
