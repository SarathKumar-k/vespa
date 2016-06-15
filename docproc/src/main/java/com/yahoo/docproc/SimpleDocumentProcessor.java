// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.docproc;

import com.yahoo.document.Document;
import com.yahoo.document.DocumentOperation;
import com.yahoo.document.DocumentPut;
import com.yahoo.document.DocumentRemove;
import com.yahoo.document.DocumentUpdate;
import com.yahoo.log.LogLevel;

/**
 * <p>Simple layer on top of {@link DocumentProcessor}, in order to make docproc
 * development more user friendly and to the point.</p>
 *
 * <p>This simply iterates over the {@link DocumentOperation}s in {@link Processing#getDocumentOperations}, and calls
 * the appropriate process() method given by this class.</p>
 *
 * <p>Note that more sophisticated use cases should subclass {@link DocumentProcessor} instead. Specifically,
 * it is not possible to return a {@link DocumentProcessor.LaterProgress} from any of the process() methods that SimpleDocumentProcessor
 * provides - since their return type is void.</p>
 *
 * <p>SimpleDocumentProcessor is for the <em>simple</em> cases. For complete control over document processing,
 * like returning instances of {@link DocumentProcessor.LaterProgress}, subclass {@link DocumentProcessor} instead.</p>
 *
 * @author <a href="mailto:einarmr@yahoo-inc.com">Einar M R Rosenvinge</a>
 * @author <a href="mailto:havardpe@yahoo-inc.com">Haavard Pettersen</a>
 */
public class SimpleDocumentProcessor extends DocumentProcessor {
    /**
     * Override this to process the Document inside a DocumentPut.
     * @deprecated use process(DocumentPut)
     *
     * @param document the Document to process.
     */
    public void process(Document document) {
        if (log.isLoggable(LogLevel.DEBUG)) {
            log.log(LogLevel.DEBUG, "Ignored " + document);
        }
    }

    /**
     * Override this to process DocumentPuts. If this method is not overridden, the implementation in this class
     * will ignore DocumentPuts (passing them through un-processed). If processing of this DocumentPut fails, the
     * implementation must throw a {@link RuntimeException}.
     *
     * @param put the DocumentPut to process.
     */
    public void process(DocumentPut put) {
        process(put.getDocument());
    }

    /**
     * Override this to process DocumentUpdates. If this method is not overridden, the implementation in this class
     * will ignore DocumentUpdates (passing them through un-processed). If processing of this DocumentUpdate fails, the
     * implementation must throw a {@link RuntimeException}.
     *
     * @param update the DocumentUpdate to process.
     */
    public void process(DocumentUpdate update) {
        if (log.isLoggable(LogLevel.DEBUG)) {
            log.log(LogLevel.DEBUG, "Ignored " + update);
        }
    }

    /**
     * Override this to process DocumentRemoves. If this method is not overridden, the implementation in this class
     * will ignore DocumentRemoves (passing them through un-processed). If processing of this DocumentRemove fails, the
     * implementation must throw a {@link RuntimeException}.
     *
     * @param remove the DocumentRemove to process.
     */
    public void process(DocumentRemove remove) {
        if (log.isLoggable(LogLevel.DEBUG)) {
            log.log(LogLevel.DEBUG, "Ignored " + remove);
        }
    }

    /**
     * Simple process() that follows the official guidelines for
     * looping over {@link DocumentOperation}s, and then calls the appropriate,
     * overloaded process() depending on the type of base.
     * <p>
     * Declared as final, so if you want to handle everything yourself
     * you should of course extend DocumentProcessor instead of
     * SimpleDocumentProcessor and just go about as usual.
     * <p>
     * It is important to note that when iterating over the {@link DocumentOperation}s in
     * {@link com.yahoo.docproc.Processing#getDocumentOperations()}, an exception thrown
     * from any of the process() methods provided by this class will be thrown straight
     * out of this here. This means that failing one document will fail the
     * entire batch.
     *
     * @param processing the Processing to process.
     * @return Progress.DONE, unless a subclass decides to throw an exception
     */
    @Override
    public final Progress process(Processing processing) {
        final int initialSize = processing.getDocumentOperations().size();
        for (DocumentOperation op : processing.getDocumentOperations()) {
            try {
                if (op instanceof DocumentPut) {
                    process((DocumentPut) op);
                } else if (op instanceof DocumentUpdate) {
                    process((DocumentUpdate) op);
                } else if (op instanceof DocumentRemove) {
                    process((DocumentRemove) op);
                }
            } catch (RuntimeException e) {
                if (log.isLoggable(LogLevel.DEBUG) && initialSize != 1) {
                    log.log(LogLevel.DEBUG,
                            "Processing of document failed, from processing.getDocumentOperations() containing " +
                            initialSize + " DocumentOperation(s).", e);
                }
                throw e;
            }
        }

        return Progress.DONE;
    }
}
