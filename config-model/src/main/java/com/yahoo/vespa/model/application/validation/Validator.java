// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation;

import com.yahoo.config.model.deploy.DeployState;
import com.yahoo.vespa.model.VespaModel;

/**
 * Abstract superclass of all application package validators.
 *
 * @author <a href="mailto:musum@yahoo-inc.com">Harald Musum</a>
 * @since 2010-01-29
 */
public abstract class Validator {

    /**
     * Validates the input vespamodel
     *
     * @param model a VespaModel object
     * @param deployState The {@link DeployState} built from building the model
     */
    public abstract void validate(VespaModel model, DeployState deployState);

}
