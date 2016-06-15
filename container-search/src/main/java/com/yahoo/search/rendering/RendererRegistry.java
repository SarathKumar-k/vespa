// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.rendering;

import com.yahoo.component.ComponentId;
import com.yahoo.component.ComponentSpecification;
import com.yahoo.component.provider.ComponentRegistry;
import com.yahoo.prelude.templates.PageTemplateSet;
import com.yahoo.prelude.templates.SearchRendererAdaptor;
import com.yahoo.prelude.templates.TiledTemplateSet;
import com.yahoo.prelude.templates.UserTemplate;
import com.yahoo.processing.rendering.Renderer;
import com.yahoo.search.Result;

import java.util.Collection;
import java.util.Collections;

/**
 * Holds all configured and built-in renderers.
 * This registry is always frozen.
 *
 * @author bratseth
 */
public final class RendererRegistry extends ComponentRegistry<com.yahoo.processing.rendering.Renderer<Result>> {

    public static final ComponentId xmlRendererId = ComponentId.fromString("DefaultRenderer");
    public static final ComponentId jsonRendererId = ComponentId.fromString("JsonRenderer");
    public static final ComponentId defaultRendererId = jsonRendererId;

    /** Creates a registry containing the built-in renderers only */
    public RendererRegistry() {
        this(Collections.emptyList());
    }

    /** Creates a registry of the given renderers plus the built-in ones */
    public RendererRegistry(Collection<Renderer> renderers) {
        // add json renderer
        Renderer jsonRenderer = new JsonRenderer();
        jsonRenderer.initId(RendererRegistry.jsonRendererId);
        register(jsonRenderer.getId(), jsonRenderer);

        // Add xml renderer
        Renderer xmlRenderer = new DefaultRenderer();
        xmlRenderer.initId(xmlRendererId);
        register(xmlRenderer.getId(), xmlRenderer);

        // add application renderers
        for (Renderer renderer : renderers)
            register(renderer.getId(), renderer);

        // add legacy "templates" converted to renderers
        addTemplateSet(new TiledTemplateSet());
        addTemplateSet(new PageTemplateSet());

        freeze();
    }

    @SuppressWarnings({"deprecation", "unchecked"})
    private void addTemplateSet(UserTemplate<?> templateSet) {
        Renderer renderer = new SearchRendererAdaptor(templateSet);
        ComponentId rendererId = new ComponentId(templateSet.getName());
        renderer.initId(rendererId);
        register(rendererId, renderer);
    }

    /**
     * Returns the default JSON renderer
     *
     * @return the default built-in result renderer
     */
    public com.yahoo.processing.rendering.Renderer<Result> getDefaultRenderer() {
        return getComponent(jsonRendererId);
    }

    /**
     * Returns the requested renderer.
     *
     * @param format the id or format alias of the renderer to return. If null is passed the default renderer
     *               is returned
     * @throws IllegalArgumentException if the renderer cannot be resolved
     */
    public com.yahoo.processing.rendering.Renderer<Result> getRenderer(ComponentSpecification format) {
        if (format == null || format.stringValue().equals("default")) return getDefaultRenderer();
        if (format.stringValue().equals("json")) return getComponent(jsonRendererId);
        if (format.stringValue().equals("xml")) return getComponent(xmlRendererId);

        com.yahoo.processing.rendering.Renderer<Result> renderer = getComponent(format);
        if (renderer == null)
            throw new IllegalArgumentException("No renderer with id or alias '" + format + "'. " +
                                               "Available renderers are: [" + rendererNames() + "].");
        return renderer;
    }

    private String rendererNames() {
        StringBuilder r = new StringBuilder();
        for (Renderer<Result> c : allComponents()) {
            if (r.length() > 0)
                r.append(", ");
            r.append(c.getId().stringValue());
        }
        return r.toString();
    }

}
