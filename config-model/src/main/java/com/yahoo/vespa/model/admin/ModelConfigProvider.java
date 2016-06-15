// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.admin;

import com.yahoo.config.model.ApplicationConfigProducerRoot;
import com.yahoo.config.model.producer.AbstractConfigProducer;

/**
 * A config provider for the model config. The ModelConfig is a common config and produced by {@link ApplicationConfigProducerRoot} , this config
 * producer exists to make the admin/model config id exist for legacy reasons.
 * @author <a href="mailto:musum@yahoo-inc.com">musum</a>
 * @author gjoranv
 * @since 5.0.8
 */
public class ModelConfigProvider extends AbstractConfigProducer {

    public ModelConfigProvider(AbstractConfigProducer<?> parent) {
        super(parent, "model");
    }
}
