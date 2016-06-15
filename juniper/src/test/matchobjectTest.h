// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
/**
 * Definition of the automated unit test class for the MatchObject class.
 *
 * @file matchobjectTest.h
 *
 * @author Knut Omang
 *
 * @date Created 21 Feb 2003
 *
 * $Id$
 *
 * <pre>
 *              Copyright (c) : 2003 Fast Search & Transfer ASA
 *                              ALL RIGHTS RESERVED
 * </pre>
 ***************************************************************************/
#pragma once

#include <map>
#include <vespa/fastlib/testsuite/test.h>
#include "testenv.h"

/**
 * The MatchObjectTest class holds
 * the unit tests for the MatchObject class.
 *
 * @sa      MatchObject
 * @author  Knut Omang
 */
class MatchObjectTest : public Test {

    /*************************************************************************
     *                      Test methods
     *
     * This section contains boolean methods for testing each public method
     * in the class ing tested
     *************************************************************************/

    /**
     * Test of the Term method.
     */
    void testTerm();


    /**
     * Test of performance
     */
    void testPerformance();

    /**
     * Test of the Match method.
     */
    void testMatch();

    /**
     * Test of the Match method on annotated buffers.
     */
    void testMatchAnnotated();

    /**
     * Test of the the expansion based (langid) constructor
     */
    void testLangid();
    void testCombined();


    /** Test parameter input via query handle options
     */
    void testParams();


    /*************************************************************************
     *                      Test administration methods
     *************************************************************************/

    /**
     * Set up common stuff for all test methods.
     * This method is called immediately before each test method is called
     */
    bool setUp();

    /**
     * Tear down common stuff for all test methods.
     * This method is called immediately after each test method is called
     */
    void tearDown();

    typedef void(MatchObjectTest::* tst_method_ptr) ();
    typedef std::map<std::string, tst_method_ptr> MethodContainer;
    MethodContainer test_methods_;
    void init();

protected:

    /**
     * Since we are running within Emacs, the default behavior of
     * print_progress which includes backspace does not work.
     * We'll use a single '.' instead.
     */
    virtual void print_progress() { *m_osptr << '.' << std::flush; }

public:

    MatchObjectTest() : Test("MatchObject"), test_methods_() { init(); }
    ~MatchObjectTest() {}

    /*************************************************************************
     *                         main entry points
     *************************************************************************/
    void Run(MethodContainer::iterator &itr);
    virtual void Run();
    void Run(const char *method);
    void Run(int argc, char* argv[]);
};


// Local Variables:
// mode:c++
// End:
