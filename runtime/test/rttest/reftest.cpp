#include "tests.h"

using namespace primal;

static int sObjCount = 0;

class TestObject : public PlObject
{
public:
    TestObject()
    {
        dbglog(__FUNCTION__);
        mCount = ++sObjCount;
        mCode = 0;
    }
    ~TestObject()
    {
        dbglog(__FUNCTION__);

        mCount = --sObjCount;
    }
    int useCount() const
    {
        return mCount;
    }

    void setCode(int code)
    {
        mCode = code;
    }

    int getCode() const
    {
        return mCode;
    }

private:
    int mCount;
    int mCode;
};

//extern "C" void __chkstk() {}

void testRefCount()
{
    sObjCount = 0;
    TEST("Simple construction 1")
        //PlRef ptr = PlRef<TestObject>::createObject();
        auto obj = _NEWOB(TestObject);

        RES = (sObjCount == 1);
    TESTEND()


    sObjCount = 0;
    // test construction and destruction
    TEST("RefCount construction 1")
        //PrRef ptr = new TestObject;
        //PlRef ptr(new TestObject);
        PlRef ptr = PlRef<TestObject>::createObject();

        RES = (sObjCount == 1);
    TESTEND()

    TEST("RefCount cleanup")
        RES = (sObjCount == 0);
    TESTEND()

    TEST("RefCount construction 2")
        //PrRef ptr = new TestObject;
        PlRef ptr = PlRef<TestObject>::createObject();

        dbglog("Use count:", _D(ptr->useCount()));
        RES = (sObjCount == 1);
    TESTEND()

    TEST("RefCount cleanup")
        RES = (sObjCount == 0);
    TESTEND()

    TEST("RefCount copy constructor")
        PlRef ptr = PlRef<TestObject>::createObject();

        PlRef ptr2(ptr);
        RES = (ptr == ptr2 && (sObjCount == 1));
    TESTEND()

    TEST("RefCount assignment constructor")
        PlRef ptr = PlRef<TestObject>::createObject();
        PlRef ptr2 = ptr;
        RES = (ptr == ptr2 && (sObjCount == 1));
    TESTEND()

    TESTEXP("RefCount cleanup", (sObjCount == 0));
}

void testWeakRefCount()
{
    sObjCount = 0;

    TEST("testWeakRefCount construction and assignment")
        PlWeakRef<TestObject> wptr;
        {
            PlRef ptr = PlRef<TestObject>::createObject();
            wptr = ptr;
        }
        RES = (sObjCount == 0);
    TESTEND()
}


void funcRef(PlRef<TestObject>& obj)
{
    dbglog("funcRef usecount:", _D(obj->useCount()));
    obj->setCode(1776);
}

void funcCRef(const PlRef<TestObject>& obj)
{
    dbglog("funcCRef usecount:", _D(obj->useCount()));
    //obj->setCode(1776);
}

void funcVal(PlRef<TestObject> obj)
{
    dbglog("funcVal usecount:", _D(obj->useCount()));
    obj->setCode(2022);
}

void testRcParam()
{
    sObjCount = 0;

    TEST("Param")
        PlWeakRef<TestObject> wptr;
        {
            PlRef obj = PlRef<TestObject>::createObject();

            funcRef(obj);
            dbglog("code:", _D(obj->getCode()));
            funcCRef(obj);
            dbglog("code:", _D(obj->getCode()));
            funcVal(obj);
            dbglog("code:", _D(obj->getCode()));
        }
        RES = (sObjCount == 0);
    TESTEND()
}

class A
{
public:
    A()
    {
        dbglog("constructor A()");
    }
    ~A()
    {
        dbglog("DEST ~A()");
    }
};

class B
{
public:
    B()
    {
        dbglog("constructor B()");
    }
    ~B()
    {
        dbglog("DEST ~B()");
    }
};


// B b;
// A a;

void testMem()
{
    dbglog("Memtest started");

    void *p1 = sDefaultMemAlloc->_malloc(16);
    void *p2 = sDefaultMemAlloc->_malloc(1000000);
    sDefaultMemAlloc->_free(p1);
    sDefaultMemAlloc->_free(p2);
    p1 = sDefaultMemAlloc->_malloc(16);
    p2 = sDefaultMemAlloc->_malloc(16);
    sDefaultMemAlloc->_free(p1);
    sDefaultMemAlloc->_free(p2);

    p1 = sDefaultMemAlloc->_malloc(64);
    p2 = sDefaultMemAlloc->_malloc(160);
    sDefaultMemAlloc->_free(p2);
    sDefaultMemAlloc->_free(p1);
    //*(int*)p1 = 10;

    //p1 = malloc(20);
}

// void testNew()
// {
//     dbglog("Newtest started\n");
//     A* ap = new A;
//     B* bp = new B;
//     auto aa = new A[20];

//     delete ap;
//     delete bp;
//     delete[] aa;
// }

void testAtomic()
{
    int64 i;
    atomicIncrement(&i);

}

void testFormat()
{
    int32 i = 9001010;
    // $2021, $i
    printv("Hello", "world\n", " another\n", "and another\n", _D(0xdeadbeef, 16), " ", _D(47));
    printv("Hello", "world", _D(2021));
    printv(_D(-2021), _D(i));
    printv(_D(0xFFF0F0F0FFF0F0F0, 2));
    // $(2021, rightPad(10), upper)
    //print(rightPad(_D(2021), 10))
}


void testLambda()
{
    int z = 10;

    // Assign the lambda expression that adds two numbers to an auto variable.
    // Embedded function
    auto f1 = [z](int x, int y)
    {
        return x + y + z;
    };

    //cout << f1(2, 3) << endl;
    printv("Lambda", _D(f1(2, 4)) );
}


template <typename T>
class BoxObj : public PlObject
{
public:
    T mData;
};


void testBox()
{

}


void pcrtmain()
{
    RUNTESTS();
}
