// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.search.query.profile.compiled;

import com.yahoo.component.ComponentSpecification;
import com.yahoo.component.provider.ComponentRegistry;
import com.yahoo.search.query.profile.types.QueryProfileType;
import com.yahoo.search.query.profile.types.QueryProfileTypeRegistry;

/**
 * A set of compiled query profiles.
 *
 * @author bratseth
 */
public class CompiledQueryProfileRegistry extends ComponentRegistry<CompiledQueryProfile> {

    private final QueryProfileTypeRegistry typeRegistry;

    /** Creates a compiled query profile registry with no types */
    public CompiledQueryProfileRegistry() {
        this(QueryProfileTypeRegistry.emptyFrozen());
    }

    public CompiledQueryProfileRegistry(QueryProfileTypeRegistry typeRegistry) {
        this.typeRegistry = typeRegistry;
    }

    /** Registers a type by its id */
    public void register(CompiledQueryProfile profile) {
        super.register(profile.getId(), profile);
    }

    public QueryProfileTypeRegistry getTypeRegistry() { return typeRegistry; }

    /**
     * <p>Returns a query profile for the given request string, or null if a suitable one is not found.</p>
     *
     * The request string must be a valid {@link com.yahoo.component.ComponentId} or null.<br>
     * If the string is null, the profile named "default" is returned, or null if that does not exists.
     *
     * <p>
     * The version part (if any) is matched used the usual component version patching rules.
     * If the name part matches a query profile name perfectly, that profile is returned.
     * If not, and the name is a slash-separated path, the profile with the longest matching left sub-path
     * which has a type which allows path matching is used. If there is no such profile, null is returned.
     */
    public CompiledQueryProfile findQueryProfile(String idString) {
        if (idString==null || idString.isEmpty()) return getComponent("default");
        ComponentSpecification id=new ComponentSpecification(idString);
        CompiledQueryProfile profile=getComponent(id);
        if (profile!=null) return profile;

        return findPathParentQueryProfile(new ComponentSpecification(idString));
    }

    private CompiledQueryProfile findPathParentQueryProfile(ComponentSpecification id) {
        // Try the name with "/" appended - should have the same semantics with path matching
        CompiledQueryProfile slashedProfile=getComponent(new ComponentSpecification(id.getName() + "/",id.getVersionSpecification()));
        if (slashedProfile!=null && slashedProfile.getType()!=null && slashedProfile.getType().getMatchAsPath())
            return slashedProfile;

        // Extract the parent (if any)
        int slashIndex=id.getName().lastIndexOf("/");
        if (slashIndex<1) return null;
        String parentName=id.getName().substring(0,slashIndex);
        if (parentName.equals("")) return null;

        ComponentSpecification parentId=new ComponentSpecification(parentName,id.getVersionSpecification());

        CompiledQueryProfile pathParentProfile=getComponent(parentId);

        if (pathParentProfile!=null && pathParentProfile.getType()!=null && pathParentProfile.getType().getMatchAsPath())
            return pathParentProfile;
        return findPathParentQueryProfile(parentId);
    }

}
