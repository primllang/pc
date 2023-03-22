#pragma once

#include "ytest.h"

#define TESTLIST(TS) \
    TS(testStringParam) \
    TS(testList1) \
    TS(testList2) \
    TS(testEnum) \
    TS(testScope) \
    TS(testSplitter) \
    TS(testFile) \
    TS(testYaml) \
    TS(testObjPool) \
    TS(testFmt) \
    TS(testSsoBuf) \
    TS(testClocks) \
    TS(testFlags) \
    TS(testPersist) \
    TS(testLoops)

DECLTESTS()



