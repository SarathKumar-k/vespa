// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.jrt;


public class TimeoutTest extends junit.framework.TestCase {

    Supervisor   server;
    Acceptor     acceptor;
    Supervisor   client;
    Target       target;
    Test.Barrier barrier;

    public TimeoutTest(String name) {
        super(name);
    }

    public void setUp() throws ListenFailedException {
        server   = new Supervisor(new Transport());
        client   = new Supervisor(new Transport());
        acceptor = server.listen(new Spec(Test.PORT));
        target   = client.connect(new Spec("localhost", Test.PORT));
        server.addMethod(new Method("concat", "ss", "s", this, "rpc_concat")
                         .methodDesc("Concatenate 2 strings")
                         .paramDesc(0, "str1", "a string")
                         .paramDesc(1, "str2", "another string")
                         .returnDesc(0, "ret", "str1 followed by str2"));
        barrier = new Test.Barrier();
    }

    public void tearDown() {
        target.close();
        acceptor.shutdown().join();
        client.transport().shutdown().join();
        server.transport().shutdown().join();
    }

    public void rpc_concat(Request req) {
        barrier.waitFor();
        req.returnValues().add(new StringValue(req.parameters()
                                               .get(0).asString() +
                                               req.parameters()
                                               .get(1).asString()));
    }

    public void testTimeout() {
        Request req = new Request("concat");
        req.parameters().add(new StringValue("abc"));
        req.parameters().add(new StringValue("def"));

        target.invokeSync(req, 0.1);
        barrier.breakIt();

        Request flush = new Request("frt.rpc.ping");
        target.invokeSync(flush, 5.0);
        assertTrue(!flush.isError());

        assertTrue(req.isError());
        assertEquals(ErrorCode.TIMEOUT, req.errorCode());
        assertEquals(0, req.returnValues().size());
    }

    public void testNotTimeout() {
        Request req = new Request("concat");
        req.parameters().add(new StringValue("abc"));
        req.parameters().add(new StringValue("def"));

        Test.Waiter w = new Test.Waiter();
        target.invokeAsync(req, 30.0, w);
        try { Thread.sleep(2500); } catch (InterruptedException e) {}
        barrier.breakIt();
        w.waitDone();

        assertTrue(!req.isError());
        assertEquals(1, req.returnValues().size());
        assertEquals("abcdef", req.returnValues().get(0).asString());
    }
}
