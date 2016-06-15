// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.document.restapi;

import com.yahoo.document.Document;
import com.yahoo.document.DocumentId;
import com.yahoo.document.DocumentRemove;
import com.yahoo.document.TestAndSetCondition;
import com.yahoo.document.json.JsonWriter;
import com.yahoo.document.DocumentPut;
import com.yahoo.documentapi.DocumentAccess;
import com.yahoo.documentapi.DocumentAccessException;
import com.yahoo.documentapi.SyncParameters;
import com.yahoo.documentapi.SyncSession;
import com.yahoo.documentapi.VisitorControlHandler;
import com.yahoo.documentapi.VisitorParameters;
import com.yahoo.documentapi.VisitorSession;
import com.yahoo.documentapi.messagebus.protocol.DocumentProtocol;
import com.yahoo.messagebus.StaticThrottlePolicy;
import com.yahoo.storage.searcher.ContinuationHit;
import com.yahoo.vdslib.VisitorOrdering;
import com.yahoo.vespaclient.ClusterDef;
import com.yahoo.vespaclient.ClusterList;
import com.yahoo.vespaxmlparser.VespaXMLFeedReader;
import org.apache.commons.lang3.exception.ExceptionUtils;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Optional;
import java.util.Set;

/**
 * Sends operations to messagebus via document api.
 *
 * @author dybdahl 
 */
public class OperationHandlerImpl implements OperationHandler {

    public static final int VISIT_TIMEOUT_MS = 120000;
    private final DocumentAccess documentAccess;

    public OperationHandlerImpl(DocumentAccess documentAccess) {
        this.documentAccess = documentAccess;
    }

    @Override
    public void shutdown() {
        documentAccess.shutdown();
    }

    private static final int HTTP_STATUS_BAD_REQUEST = 400;
    private static final int HTTP_STATUS_INSUFFICIENT_STORAGE = 507;

    private static int getHTTPStatusCode(Set<Integer> errorCodes) {
        if (errorCodes.size() == 1 && errorCodes.contains(DocumentProtocol.ERROR_NO_SPACE)) {
            return HTTP_STATUS_INSUFFICIENT_STORAGE;
        }
        return HTTP_STATUS_BAD_REQUEST;
    }

    private static Response createErrorResponse(DocumentAccessException documentException, RestUri restUri) {
        return Response.createErrorResponse(getHTTPStatusCode(documentException.getErrorCodes()), documentException.getMessage(), restUri);
    }

    @Override
    public VisitResult visit(
            RestUri restUri,
            String documentSelection,
            Optional<String> cluster,
            Optional<String> continuation) throws RestApiException {

        VisitorParameters visitorParameters = createVisitorParameters(restUri, documentSelection, cluster, continuation);

        VisitorControlHandler visitorControlHandler = new VisitorControlHandler();
        visitorParameters.setControlHandler(visitorControlHandler);
        LocalDataVisitorHandler localDataVisitorHandler = new LocalDataVisitorHandler();
        visitorParameters.setLocalDataHandler(localDataVisitorHandler);

        final VisitorSession visitorSession;
        try {
            visitorSession = documentAccess.createVisitorSession(visitorParameters);
            // Not sure if this line is required
            visitorControlHandler.setSession(visitorSession);
        } catch (Exception e) {
            throw new RestApiException(Response.createErrorResponse(
                    500,
                    "Failed during parsing of arguments for visiting: " + ExceptionUtils.getStackTrace(e),
                    restUri));
        }
        try {
            if (! visitorControlHandler.waitUntilDone(VISIT_TIMEOUT_MS)) {
                throw new RestApiException(Response.createErrorResponse(500, "Timed out", restUri));
            }
            if (visitorControlHandler.getResult().code != VisitorControlHandler.CompletionCode.SUCCESS) {
                throw new RestApiException(Response.createErrorResponse(400, visitorControlHandler.getResult().toString()));
            }
        } catch (InterruptedException e) {
            throw new RestApiException(Response.createErrorResponse(500, ExceptionUtils.getStackTrace(e), restUri));
        }
        if (localDataVisitorHandler.getErrors().isEmpty()) {
            final Optional<String> continuationToken;
            if (! visitorControlHandler.getProgress().isFinished()) {
                final ContinuationHit continuationHit = new ContinuationHit(visitorControlHandler.getProgress());
                continuationToken = Optional.of(continuationHit.getValue());
            } else {
                continuationToken = Optional.empty();
            }
            return new VisitResult(continuationToken, localDataVisitorHandler.getCommaSeparatedJsonDocuments());
        }
        throw new RestApiException(Response.createErrorResponse(500, localDataVisitorHandler.getErrors(), restUri));
    }

    @Override
    public void put(RestUri restUri, VespaXMLFeedReader.Operation data) throws RestApiException {
        try {
            SyncSession syncSession = documentAccess.createSyncSession(new SyncParameters());
            DocumentPut put = new DocumentPut(data.getDocument());
            put.setCondition(data.getCondition());
            syncSession.put(put);
        } catch (DocumentAccessException documentException) {
            throw new RestApiException(createErrorResponse(documentException, restUri));
        } catch (Exception e) {
            throw new RestApiException(Response.createErrorResponse(500, ExceptionUtils.getStackTrace(e), restUri));
        }
    }

    @Override
    public void update(RestUri restUri, VespaXMLFeedReader.Operation data) throws RestApiException {
        try {
            SyncSession syncSession = documentAccess.createSyncSession(new SyncParameters());
            syncSession.update(data.getDocumentUpdate());
        } catch (DocumentAccessException documentException) {
            throw new RestApiException(createErrorResponse(documentException, restUri));
        } catch (Exception e) {
            throw new RestApiException(Response.createErrorResponse(500, ExceptionUtils.getStackTrace(e), restUri));
        }
    }

    @Override
    public void delete(RestUri restUri, String condition) throws RestApiException {
        try {
            DocumentId id = new DocumentId(restUri.generateFullId());
            SyncSession syncSession = documentAccess.createSyncSession(new SyncParameters());
            DocumentRemove documentRemove = new DocumentRemove(id);
            if (condition != null && ! condition.isEmpty()) {
                documentRemove.setCondition(new TestAndSetCondition(condition));
            }
            syncSession.remove(documentRemove);
        } catch (DocumentAccessException documentException) {
            throw new RestApiException(Response.createErrorResponse(400, documentException.getMessage(), restUri));
        } catch (Exception e) {
            throw new RestApiException(Response.createErrorResponse(500, ExceptionUtils.getStackTrace(e), restUri));
        }
    }

    @Override
    public Optional<String> get(RestUri restUri) throws RestApiException {
        try {
            DocumentId id = new DocumentId(restUri.generateFullId());
            SyncSession syncSession = documentAccess.createSyncSession(new SyncParameters());
            final Document document = syncSession.get(id);
            if (document == null) {
                return Optional.empty();
            }
            ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
            JsonWriter jsonWriter = new JsonWriter(outputStream);
            jsonWriter.write(document);
            return Optional.of(outputStream.toString(StandardCharsets.UTF_8.name()));

        } catch (Exception e) {
            throw new RestApiException(Response.createErrorResponse(500, ExceptionUtils.getStackTrace(e), restUri));
        }
    }

    private static String resolveClusterRoute(Optional<String> wantedCluster) throws RestApiException {
        List<ClusterDef> clusters = new ClusterList("client").getStorageClusters();
        return resolveClusterRoute(wantedCluster, clusters);
    }

    // Based on resolveClusterRoute in VdsVisit, protected for testability
    protected static String resolveClusterRoute(Optional<String> wantedCluster, List<ClusterDef> clusters) throws RestApiException {
        if (clusters.size() == 0) {
            throw new IllegalArgumentException("Your Vespa cluster does not have any content clusters " +
                    "declared. Visiting feature is not available.");
        }
        if (! wantedCluster.isPresent()) {
            if (clusters.size() != 1) {
                new RestApiException(Response.createErrorResponse(400, "Several clusters exist: " +
                        clusterListToString(clusters) + " you must specify one.. "));
            }
            return clusterDefToRoute(clusters.get(0));
        }

        for (ClusterDef clusterDef : clusters) {
            if (clusterDef.getName().equals(wantedCluster.get())) {
                return clusterDefToRoute(clusterDef);
            }
        }
        throw new RestApiException(Response.createErrorResponse(400, "Your vespa cluster contains the content clusters " +
                clusterListToString(clusters) + " not " + wantedCluster.get() + ". Please select a valid vespa cluster."));

    }

    private static String clusterDefToRoute(ClusterDef clusterDef) {
        return "[Storage:cluster=" + clusterDef.getName() + ";clusterconfigid=" + clusterDef.getConfigId() + "]";
    }

    private static String clusterListToString(List<ClusterDef> clusters) {
        StringBuilder clusterListString = new StringBuilder();
        clusters.forEach(x -> clusterListString.append(x.getName()).append(" (").append(x.getConfigId()).append("), "));
        return clusterListString.toString();
    }

    private VisitorParameters createVisitorParameters(
            RestUri restUri,
            String documentSelection,
            Optional<String> clusterName,
            Optional<String> continuation)
            throws RestApiException {

        StringBuilder selection = new StringBuilder();

        if (! documentSelection.isEmpty()) {
            selection.append("(").append(documentSelection).append(" and ");
        }
        selection.append(restUri.getDocumentType()).append(" and (id.namespace=='").append(restUri.getNamespace()).append("')");
        if (! documentSelection.isEmpty()) {
            selection.append(")");
        }

        VisitorParameters params = new VisitorParameters(selection.toString());

        params.setMaxBucketsPerVisitor(1);
        params.setMaxPending(32);
        params.setMaxFirstPassHits(1);
        params.setMaxTotalHits(10);
        params.setThrottlePolicy(new StaticThrottlePolicy().setMaxPendingCount(1));
        params.setToTimestamp(0L);
        params.setFromTimestamp(0L);

        params.visitInconsistentBuckets(true);
        params.setVisitorOrdering(VisitorOrdering.ASCENDING);

        params.setRoute(resolveClusterRoute(clusterName));

        params.setTraceLevel(0);
        params.setPriority(DocumentProtocol.Priority.NORMAL_4);
        params.setVisitRemoves(false);

        if (continuation.isPresent()) {
            try {
                params.setResumeToken(ContinuationHit.getToken(continuation.get()));
            } catch (Exception e) {
                throw new RestApiException(Response.createErrorResponse(500, ExceptionUtils.getStackTrace(e), restUri));
            }
        }
        return params;
    }

}
