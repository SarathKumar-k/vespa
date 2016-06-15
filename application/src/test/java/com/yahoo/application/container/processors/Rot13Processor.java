// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.application.container.processors;

import com.yahoo.processing.Processor;
import com.yahoo.processing.Request;
import com.yahoo.processing.Response;
import com.yahoo.processing.execution.Execution;
import com.yahoo.processing.test.ProcessorLibrary;

import static com.yahoo.application.container.docprocs.Rot13DocumentProcessor.rot13;

/**
 * @author <a href="mailto:einarmr@yahoo-inc.com">Einar M R Rosenvinge</a>
 */
public class Rot13Processor extends Processor {
    @Override
    public Response process(Request request, Execution execution) {
        Object fooObj = request.properties().get("title");

        Response response = new Response(request);
        if (fooObj != null) {
            response.data().add(new ProcessorLibrary.StringData(request, rot13(fooObj.toString())));
        }
        return response;
    }
}
