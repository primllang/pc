#pragma once

#include "pcrt.h"
#include "ytest.h"

#define TESTLIST(TS) \
    TS(testRefCount) \
    TS(testWeakRefCount) \
    TS(testRcParam) \
    TS(testArray) \
    TS(testArray2) \
    TS(testMemPool) \
    TS(testMemPool2) \
    TS(testPlVector) \
    TS(testAtomic) \
    TS(testMem) \
    TS(testFormat) \
    TS(testLambda)


DECLTESTS()
