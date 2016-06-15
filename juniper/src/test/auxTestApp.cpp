// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
LOG_SETUP("auxTest");
#include "auxTest.h"

class AuxTestApp : public FastOS_Application
{
public:
    virtual int Main();
};



void Usage(char* s)
{
    fprintf(stderr, "Usage: %s [-d debug_level]\n", s);
}


int AuxTestApp::Main()
{
    juniper::TestEnv te(this, "../rpclient/testclient.rc");
    AuxTest pta;
    pta.SetStream(&std::cout);
    pta.Run(_argc, _argv);
    return pta.Report();
}

FASTOS_MAIN(AuxTestApp);
