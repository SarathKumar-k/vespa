// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.container.processing;

import com.yahoo.component.chain.model.ChainedComponentModel;
import com.yahoo.config.model.producer.AbstractConfigProducer;
import com.yahoo.vespa.model.container.component.chain.ChainedComponent;

/**
 * Representation of a Processor in the configuration model
 *
 * @author  <a href="mailto:bratseth@yahoo-inc.com">Jon Bratseth</a>
 * @since   5.1.6
 */
public class Processor extends ChainedComponent<ChainedComponentModel> {

    public Processor(ChainedComponentModel model) {
        super(model);
    }

}
