// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.builder.xml.dom.chains.processing;

import com.yahoo.config.model.producer.AbstractConfigProducer;
import com.yahoo.config.application.api.ApplicationPackage;
import com.yahoo.vespa.model.builder.xml.dom.chains.ComponentsBuilder;
import com.yahoo.vespa.model.builder.xml.dom.chains.DomChainsBuilder;
import com.yahoo.vespa.model.container.processing.ProcessingChain;
import com.yahoo.vespa.model.container.processing.ProcessingChains;
import com.yahoo.vespa.model.container.processing.Processor;
import org.w3c.dom.Element;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * Root builder of the processing model
 *
 * @author  <a href="mailto:bratseth@yahoo-inc.com">Jon Bratseth</a>
 * @since   5.1.6
 */
public class DomProcessingBuilder extends DomChainsBuilder<Processor, ProcessingChain, ProcessingChains> {

    public DomProcessingBuilder(Element outerChainsElem) {
        super(outerChainsElem, Arrays.asList(ComponentsBuilder.ComponentType.processor), ApplicationPackage.PROCESSORCHAINS_DIR);
    }

    @Override
    protected ProcessingChains newChainsInstance(AbstractConfigProducer parent) {
        return new ProcessingChains(parent, "processing");
    }

    @Override
    protected ProcessingChainsBuilder readChains(AbstractConfigProducer ancestor, List<Element> processingChainsElements,
                                                Map<String, ComponentsBuilder.ComponentType> outerComponentTypeByComponentName) {
        return new ProcessingChainsBuilder(ancestor, processingChainsElements, outerComponentTypeByComponentName);
    }

}
