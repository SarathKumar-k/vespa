// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.jrt.slobrok.api;


import com.yahoo.jrt.*;

import java.util.Arrays;
import java.util.Random;
import java.util.ArrayList;
import java.util.logging.Logger;
import java.util.logging.Level;


/**
 * A Mirror object is used to keep track of the services registered
 * with a slobrok cluster.
 *
 * Updates to the service repository are fetched in the
 * background. Lookups against this object is done using an internal
 * mirror of the service repository.
 **/
public class Mirror implements IMirror {

    private static Logger log = Logger.getLogger(Mirror.class.getName());

    /**
     * An Entry contains the name and connection spec for a single
     * service.
     **/
    public static final class Entry implements Comparable<Entry> {
        private final String name;
        private final String spec;
        private final char [] nameArray;

        public Entry(String name, String spec) {
            this.name = name;
            this.spec = spec;
            this.nameArray = name.toCharArray();
        }

        public boolean equals(Object rhs) {
            if (rhs == null || !(rhs instanceof Entry)) {
                return false;
            }
            Entry e = (Entry) rhs;
            return (name.equals(e.name) && spec.equals(e.spec));
        }

        public int hashCode() {
            return (name.hashCode() + spec.hashCode());
        }

        public int compareTo(Entry b) {
            int diff = name.compareTo(b.name);
            return diff != 0
                       ? diff
                       : spec.compareTo(b.spec);
        }
        char [] getNameArray() { return nameArray; }
        public String getName() { return name; }
        public String getSpec() { return spec; }
    }

    private Supervisor        orb;
    private SlobrokList       slobroks;
    private String            currSlobrok;
    private BackOffPolicy     backOff;
    private volatile int      updates    = 0;
    private boolean           reqDone    = false;
    private volatile Entry[]  specs      = new Entry[0];
    private int               specsGen   = 0;
    private Task              updateTask = null;
    private RequestWaiter     reqWait    = null;
    private Target            target     = null;
    private Request           req        = null;

    /**
     * Create a new MirrorAPI using the given Supervisor and slobrok
     * connect specs.
     *
     * @param orb the Supervisor to use
     * @param slobroks slobrok connect spec list
     * @param bop custom backoff policy, mostly useful for testing
     **/
    public Mirror(Supervisor orb, SlobrokList slobroks, BackOffPolicy bop) {
        this.orb = orb;
        this.slobroks = slobroks;
        this.backOff = bop;
        updateTask = orb.transport().createTask(new Runnable() {
                public void run() { handleUpdate(); }
            });
        reqWait = new RequestWaiter() {
                public void handleRequestDone(Request req) {
                    reqDone = true;
                    updateTask.scheduleNow();
                }
            };
        updateTask.scheduleNow();
    }

    /**
     * Create a new MirrorAPI using the given Supervisor and slobrok
     * connect specs.
     *
     * @param orb the Supervisor to use
     * @param slobroks slobrok connect spec list
     **/
    public Mirror(Supervisor orb, SlobrokList slobroks) {
        this(orb, slobroks, new BackOff());
    }

    /**
     * Shut down the Mirror. This will close any open connections and
     * stop the regular mirror updates.
     **/
    public void shutdown() {
        updateTask.kill();
        orb.transport().perform(new Runnable() {
                public void run() { handleShutdown(); }
            });
    }

    @Override
    public Entry[] lookup(String pattern) {
        ArrayList<Entry> found = new ArrayList<Entry>();
        Entry [] e = specs;
        char [] p = pattern.toCharArray();
        for (int i = 0; i < e.length; i++) {
            if (match(e[i].getNameArray(), p)) {
                found.add(e[i]);
            }
        }
        return found.toArray(new Entry[found.size()]);
    }

    @Override
    public int updates() {
        return updates;
    }

    /**
     * Ask if the MirrorAPI has got any useful information from the Slobrok.
     *
     * On application startup it is often useful to run the event loop for some time until this functions returns true
     * (or if it never does, time out and tell the user there was no answer from any Service Location Broker).
     *
     * @return true if the MirrorAPI object has asked for updates from a Slobrok and got any answer back
     **/
    public boolean ready() {
        return (updates != 0);
    }

    /**
     * Returns whether this mirror is connected to the slobrok server at this
     * time or not.
     */
    public boolean connected() {
        return (target != null);
    }

    /**
     * Match a single name against a pattern.
     * A pattern can contain '*' to match until the next '/' separator,
     * and end with '**' to match the rest of the name.
     * Note that this isn't quite globbing, as there is no backtracking.
     *
     * @return true if the name matches the pattern
     * @param name the name
     * @param pattern the pattern
     **/
    static boolean match(char [] name, char [] pattern) {
        int ni = 0;
        int pi = 0;

        while (ni < name.length && pi < pattern.length) {
            if (name[ni] == pattern[pi]) {
                ni++;
                pi++;
            } else if (pattern[pi] == '*') {
                pi++;
                while (ni < name.length && name[ni] != '/') {
                    ni++;
                }
                if (pi < pattern.length && pattern[pi] == '*') {
                    pi++;
                    ni = name.length;
                }
            } else {
                return false;
            }
        }
        while (pi < pattern.length && pattern[pi] == '*') {
            pi++;
        }
        return ((ni == name.length) && (pi == pattern.length));
    }

    /**
     * Invoked by the update task.
     **/
    private void handleUpdate() {
        if (reqDone) {
            reqDone = false;

            if (req.errorCode() == ErrorCode.NONE &&
                req.returnValues().satisfies("SSi") &&
                req.returnValues().get(0).count() == req.returnValues().get(1).count())
            {
                Values answer = req.returnValues();

                if (specsGen != answer.get(2).asInt32()) {

                    int      numNames = answer.get(0).count();
                    String[]        n = answer.get(0).asStringArray();
                    String[]        s = answer.get(1).asStringArray();
                    Entry[]  newSpecs = new Entry[numNames];

                    for (int idx = 0; idx < numNames; idx++) {
                        newSpecs[idx] = new Entry(n[idx], s[idx]);
                    }

                    specs = newSpecs;

                    specsGen = answer.get(2).asInt32();
                    int u = (updates + 1);
                    if (u == 0) {
                        u++;
                    }
                    updates = u;
                }
                backOff.reset();
                updateTask.schedule(0.1); // be nice
                return;
            }
            if (!req.checkReturnTypes("iSSSi")
                || (req.returnValues().get(2).count() !=
                    req.returnValues().get(3).count()))
            {
                target.close();
                target = null;
                updateTask.scheduleNow(); // try next slobrok
                return;
            }


            Values answer = req.returnValues();

            int diffFrom = answer.get(0).asInt32();
            int diffTo   = answer.get(4).asInt32();

            if (specsGen != diffTo) {

                int      nRemoves = answer.get(1).count();
                String[]        r = answer.get(1).asStringArray();

                int      numNames = answer.get(2).count();
                String[]        n = answer.get(2).asStringArray();
                String[]        s = answer.get(3).asStringArray();


                Entry[]  newSpecs;
                if (diffFrom == 0) {
                    newSpecs = new Entry[numNames];

                    for (int idx = 0; idx < numNames; idx++) {
                        newSpecs[idx] = new Entry(n[idx], s[idx]);
                    }
                } else {
                    java.util.HashMap<String, Entry> map = new java.util.HashMap<String, Entry>();
                    for (Entry e : specs) {
                        map.put(e.getName(), e);
                    }
                    for (String rem : r) {
                        map.remove(rem);
                    }
                    for (int idx = 0; idx < numNames; idx++) {
                        map.put(n[idx], new Entry(n[idx], s[idx]));
                    }
                    newSpecs = new Entry[map.size()];
                    int idx = 0;
                    for (Entry e : map.values()) {
                        newSpecs[idx++] = e;
                    }
                }

                specs = newSpecs;

                specsGen = diffTo;
                int u = (updates + 1);
                if (u == 0) {
                    u++;
                }
                updates = u;
            }
            backOff.reset();
            updateTask.schedule(0.1); // be nice
            return;
        }
        if (target != null && ! slobroks.contains(currSlobrok)) {
            target.close();
            target = null;
        }
        if (target == null) {
            currSlobrok = slobroks.nextSlobrokSpec();
            if (currSlobrok == null) {
                double delay = backOff.get();
                updateTask.schedule(delay);
                if (backOff.shouldWarn(delay)) {
                    log.log(Level.INFO, "no location brokers available "
                            + "(retry in " + delay + " seconds) for: " + slobroks);
                }
                return;
            }
            target = orb.connect(new Spec(currSlobrok));
            specsGen = 0;
        }
        req = new Request("slobrok.incremental.fetch");
        req.parameters().add(new Int32Value(specsGen)); // gencnt
        req.parameters().add(new Int32Value(5000));     // mstimeout
        target.invokeAsync(req, 40.0, reqWait);
    }

    /**
     * Invoked from the transport thread, requested by the shutdown
     * method.
     **/
    private void handleShutdown() {
        if (req != null) {
            req.abort();
            req = null;
        }
        if (target != null) {
            target.close();
            target = null;
        }
    }
}
