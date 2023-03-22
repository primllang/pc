#include "tests.h"

using namespace primal;

class MyObj : public PlObject
{
public:
    MyObj() :
        mCount(0)
    {
        dbgfnc();
    }
    void init(int cnt)
    {
        dbgfnc();
        mCount = cnt;
    }
    int getCnt()
    {
        return mCount;
    }
    ~MyObj()
    {
        dbgfnc();
    }

private:
    int mCount;
};


class ArrTestObject// : public PlObject
{
public:
    ArrTestObject() :
        mCount(0)
    {
        dbgfnc();
    }
    ArrTestObject(int cnt) :
        mCount(cnt)
    {
        dbgfnc();
    }
    ~ArrTestObject()
    {
        dbgfnc();
    }

private:
    int mCount;
};

void testArray2()
{
    TEST("append")
        ObjArray<ArrTestObject> arrt;
        ArrTestObject o(5);
        arrt.append(o);
    TESTEND()

    TEST("appendMem")
        ObjArray<ArrTestObject> arrt;
        new (arrt.appendMem()) ArrTestObject(5);
    TESTEND()
}

#define ARR_MAX_INS   10000000

void testArray()
{
    const uint64 cnt = ARR_MAX_INS;

    class objtype
    {
    public:
        char buf[40] = "Yasser";
        uint64 n;
    } r;

    // ObjArray
    TIME("ObjArray inserts")
    ObjArray<objtype> arrt;
    for (uint64 i = 0; i < cnt; i++)
    {
        r.n = i;
        arrt.append(r);
    }
    TIMEEND()

    // MemArray
    TIME("MemArray inserts")
    MemArray arr(sizeof(objtype));
    for (uint64 i = 0; i < cnt; i++)
    {
        //*(long int*)arr.append() = i;
        r.n = i;
        memcpy(arr.append(), &r, sizeof(r));
    }
    TIMEEND()

#if 0
    // Stl
    start = getTickMS();
    std::vector<objtype> v;
    for (uint64 i = 0; i < cnt; i++)
    {
        r.n = i;
        v.push_back(r);
    }
    tstl = getTickMS() - start;

    printf("STL time=%llu added=%zu\n", tstl, v.size());
#endif
}

void testMemPool()
{
    uint64 start;
    uint64 t;

    printv("Testing MemPool");

    class objtype
    {
    public:
        char buf[40] = "Yasser";
        uint64 n;
    } r;

    const uint64 cnt = ARR_MAX_INS;

    TIME("MemPool with STATIC pool size")
        MemPool pool(sizeof(objtype), 1024);
        MemArray arr(sizeof(void*));

        bool createdblock;
        for (uint64 i = 0; i < cnt; i++)
        {
            r.n = i;

            void* p = pool.allocElem(&createdblock);
            *(void**)(arr.append()) = p;
            memcpy(p, &r, sizeof(r));
        }
    TIMEEND()

    TIME("MemPool with DYNAMIC pool size inc every 8")
        MemPool pool(sizeof(objtype), 1024, 8);
        MemArray arr(sizeof(void*));

        bool createdblock;
        for (uint64 i = 0; i < cnt; i++)
        {
            r.n = i;

            void* p = pool.allocElem(&createdblock);
            *(void**)(arr.append()) = p;
            memcpy(p, &r, sizeof(r));
        }
    TIMEEND()
}



void testMemPool2()
{
    uint64 start;
    uint64 t;

    printv("Testing MemPool");

    class objtype
    {
    public:
        char buf[40] = "Yasser";
        uint64 n;
    } r;

    const uint64 cnt = 6000000;

    MemPool pool(sizeof(objtype), 1024);
    MemArray arr(sizeof(void*));

    bool createdblock;
    for (uint64 i = 0; i < cnt; i++)
    {
        r.n = i;

        void* p = pool.allocElem(&createdblock);
        // if (createdblock)
        // {
        //     dbglog("NewBlock bs=", dollar(pool.currentBlockSize()), " bcount=", dollar(pool.blockCount()));
        // }

        *(void**)(arr.append()) = p;
        memcpy(p, &r, sizeof(r));
    }

    TESTEXP("MemPool length", (arr.length() == cnt));

    TEST("Verifying MemPool content")
        for (uint64 i = 0; i < arr.length(); i++)
        {
            objtype** p = (objtype**)arr.get(i);
            RES = (*p)->n == i;
            if (!RES)
            {
                break;
            }
        }
    TESTEND()
}


void testArray3()
{
    const uint64 initsize = 2000000;
    const uint64 inscount = 6;
    const uint64 insat = 5000;

    class objtype
    {
    public:
        char buf[40] = "Yasser";
        uint64 n;
    } r;

    ObjArray<objtype> obarr;
    TIME("ObjArray initial appends")
        for (uint64 i = 0; i < initsize; i++)
        {
            r.n = i;
            obarr.append(r);
        }
    TIMEEND()

    TIME("ObjArray inserts inside")
        for (uint64 i = 0; i < obarr.length(); i++)
        {
            if (i % insat == 0)
            {
                for (uint64 j = 0; j < inscount; j++)
                {
                    r.n = inscount * 10 + i + j;
                    memmove(obarr.insertMem(i), &r, sizeof(r));
                }
            }
        }
    TIMEEND()

    MemPool pool(sizeof(objtype), 1024, 8);
    MemArray poolarr(sizeof(void*));
    bool createdblock;
    TIME("MemPool initial appends")
        for (uint64 i = 0; i < initsize; i++)
        {
            r.n = i;

            void* p = pool.allocElem(&createdblock);
            *(void**)(poolarr.append()) = p;
            memcpy(p, &r, sizeof(r));
        }
    TIMEEND()

    TIME("MemPool inserts inside")
        for (uint64 i = 0; i < poolarr.length(); i++)
        {
            if (i % insat == 0)
            {
                for (uint64 j = 0; j < inscount; j++)
                {
                    r.n = inscount * 10 + i + j;

                    void* p = pool.allocElem(&createdblock);
                    *(void**)(poolarr.insert(i)) = p;
                    memcpy(p, &r, sizeof(r));
                }
            }
        }
    TIMEEND()
}


void testPlVector()
{
    /*
    Primal:
    var Vector<MyObj> arr = new Vector<MyObj>
    var MyObj obj = arr.appendNew()
    obj.init(1968)
    arr.appendNew().init(1991)
    arr.appendNew().init(1993)
    arr.appendNew().init(1996)
    arr.appendNew().init(1969)
    arr.appendNew().init(1971)
    for i in arr
    {
        print("> ", $i.getCnt())
    }
    print("reverse")
    for i in rev arr
    {
        print("> ", $i.getCnt())
    }
    */

    TEST("PlArray simple")
        _OB(PlVector<MyObj>) arr = _NEWOB(PlVector<MyObj>);
        _OB(MyObj) obj = arr->appendNew();
        obj->init(1968);
        arr->appendNew()->init(1991);
        arr->appendNew()->init(1993);
        arr->appendNew()->init(1996);
        arr->appendNew()->init(1969);
        arr->appendNew()->init(1971);

        //arr->insert(500);

        for (auto i = arr->iterFwd(); arr->iterFwdLoop(i); )
        {
            printv("> ", _D(i->getCnt()));
        }

        printv("reverse");
        for (auto i = arr->iterRev(); arr->iterRevLoop(i); )
        {
            printv("> ", _D(i->getCnt()));
        }

    TESTEND()
}


constDef CONSTDEF_AUTOSTR = "auto";
constDef CONSTDEF_AUTOUINT = 25ul;
constDef_(size_t) CONSTDEF_SIZET = 20;

class testc
{
private:
    constMemb CONSTMEMB_AUTOSTR = "auto";
    constMemb CONSTMEMB_AUTOFLOAT = 25.0;
    constMemb_(size_t) CONSTMEMB_SIZET = 640;
};
