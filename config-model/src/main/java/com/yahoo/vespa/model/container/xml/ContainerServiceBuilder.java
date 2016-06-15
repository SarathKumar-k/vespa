// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.container.xml;

import com.yahoo.component.ComponentSpecification;
import com.yahoo.config.model.ConfigModelUtils;
import com.yahoo.config.model.builder.xml.XmlHelper;
import com.yahoo.config.model.producer.AbstractConfigProducer;
import com.yahoo.text.XML;
import com.yahoo.vespa.model.builder.xml.dom.VespaDomBuilder;
import com.yahoo.vespa.model.container.Container;
import org.w3c.dom.Element;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;

/**
 * @author tonytv
 */
public class ContainerServiceBuilder extends VespaDomBuilder.DomConfigProducerBuilder<Container> {

    private final String id;

    public ContainerServiceBuilder(String id) {
        this.id = id;
    }

    @Override
    protected Container doBuild(AbstractConfigProducer parent, Element nodeElem) {
        return new Container(parent, id, readServerPortOverrides(nodeElem));
    }

    private List<Container.PortOverride> readServerPortOverrides(Element spec) {
        List<Container.PortOverride> portOverrides = new ArrayList<>();

        for (Element serverPort: XML.getChildren(spec, "server-port")) {
            ComponentSpecification serverId = XmlHelper.getIdRef(serverPort);
            int port = Integer.parseInt(serverPort.getAttribute("port"));

            portOverrides.add(new Container.PortOverride(serverId, port));
        }

        return portOverrides;
    }
}
