
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <initializer_list>
#include <list>
#include <stack>
#include <unordered_map>
#include <string>
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/time.h>
#endif
#include "util.h"
#include "parse.h"
#include "var.h"

#include "tests.h"

void testList1()
{
    dbgfnc();
    dbglog(" Enter\n");

    base::List<std::string> lst;
    std::string todels;

    for (int i = 0; i < 10; i++)
    {
        std::string s;
        base::format(s, "std::string item associated with %d", i);
        if (i == 3)
        {
            todels = s;
        }

        lst.addBack(s);
    }

    for (base::Iter<std::string> t; lst.forEach(t); )
    {
        dbglog("List entry: %d  is '%s'\n", t.pos(), t->c_str());
    }
    dbglog("Rev:\n");
    for (base::Iter<std::string> t; lst.forEachRev(t); )
    {
        dbglog("base::List: %d  is '%s'\n", t.pos(), t->c_str());
    }
    dbglog("remove '%s'\n", todels.c_str());

    lst.findAndRemove(todels);

    dbglog("After\n");
    for (base::Iter<std::string> t; lst.forEach(t); )
    {
        dbglog("List entry: %d  is '%s'\n", t.pos(), t->c_str());
    }

    void* s;
    dbglog("Front/next\n");
    s = lst.front();
    while (s)
    {
        dbglog("List entry:  '%s'\n", ((std::string*)s)->c_str());
        s = lst.next(s);
    }

    dbglog("Back/prev\n");
    s = lst.back();
    while (s)
    {
        dbglog("List entry:  '%s'\n", ((std::string*)s)->c_str());
        s = lst.prev(s);
    }

    // Remove all manually--can also call clear
    while (lst.removeFront())
    {
    }

    dbglog("after removing all manually, empty=%d\n", lst.empty());
    for (base::Iter<std::string> t; lst.forEach(t); )
    {
        dbglog("List entry: %d  is '%s'\n", t.pos(), t->c_str());
    }

    lst.addBack("Yasser");
    dbglog("after adding 1, empty=%d\n", lst.empty());
    for (base::Iter<std::string> t; lst.forEach(t); )
    {
        dbglog("List entry: %d  is '%s'\n", t.pos(), t->c_str());
    }

    dbgfnc();
    dbglog(" Leave\n");
}


struct ListEntry
{
    ~ListEntry()
    {
        dbgfnc();
    }
    int group;
    base::List<int> l;
};

base::List<ListEntry> ll;

void add(int group, int num)
{
    bool added = false;
    for (base::Iter<ListEntry> g; ll.forEach(g); )
    {
        if (g->group == group)
        {
            g->l.addBack(num);
            added = true;
            break;
        }
    }
    if (!added)
    {
        // ListEntry le;
        // le.group = group;
        // le.l.addBack(num);
        // ll.addBack(le);

        ListEntry* le = new (ll.allocBack()) ListEntry();
        le->group = group;
        le->l.addBack(num);
    }
}

void testList2()
{
    dbglog("enter testList2\n");
    add(100, 1);
    add(101, 2120);
    add(100, 2);
    add(101, 2121);
    add(101, 2122);
    add(101, 2123);
    add(100, 3);
    add(100, 4);
    add(102, 50000);
    add(102, 50001);
    add(100, 5);

    for (base::Iter<ListEntry> le; ll.forEach(le); )
    {
        printf("Group: %d\n", le->group);
        for (base::Iter<int> e; le->l.forEach(e); )
        {
            printf("  %d\n", *e);
        }
    }
    dbglog("leave testList2\n");
}

// Entities
#define Entity_Enum(_en_)               \
    _en_(None)                          \
        _en_(Unit)                      \
            _en_(Func)                  \
                _en_(FuncParamBlock)    \
                    _en_(FuncParam)     \
                        _en_(StmtBlock) \
                            _en_(Statement)

enummapdef(Entity_Enum, Entity, EntityMap, 0);

// Keywords
// NOTE: We need to have kw_ prefix because reserved words like for cannot be enum
// identifier. For strings, these are skipped due to 3 (skip 3 chars)
#define Keyword_Enum(_en_)            \
    _en_(kw_func)                     \
        _en_(kw_outer)                \
            _en_(kw_for)              \
                _en_(kw_while)        \
                    _en_(kw_if)       \
                        _en_(kw_true) \
                            _en_(kw_false)

enummapdef(Keyword_Enum, Keyword, KeywordMap, 3);

#define Primitive_Enum(_en_)                                      \
    _en_(p_int8)                                                  \
        _en_(p_int16)                                             \
            _en_(p_int32)                                         \
                _en_(p_int64)                                     \
                    _en_(p_int)                                   \
                        _en_(p_uint8)                             \
                            _en_(p_uint16)                        \
                                _en_(p_uint32)                    \
                                    _en_(p_uint64)                \
                                        _en_(p_uint)              \
                                            _en_(p_char)          \
                                                _en_(p_byte)      \
                                                    _en_(p_sizet) \
                                                        _en_(p_bool)

enummapdef(Primitive_Enum, Primitive, sPrimitiveMap, 2);

void testEnum()
{
    printf("exist Unit=%d\n", EntityMap.exists("Unit"));
    printf("exist Damn=%d\n", EntityMap.exists("Damn"));

    // auto x = EntityMap.toString(Entity::Func);
    // printf("v=%s e=%s\n", x.value.c_str(), x.error.message().c_str());
    // x = EntityMap.toString(Entity::Unit);
    // printf("v=%s e=%s\n", x.value.c_str(), x.error.message().c_str());
    // x = EntityMap.toString((Entity)20);
    // printf("v=%s e=%s\n", x.value.c_str(), x.error.message().c_str());

    Entity y;
    bool ret;
    ret = EntityMap.fromString("Unit", &y);
    printf("v=%d e=%d\n", (int)y, ret);
    ret = EntityMap.fromString("Shucks", &y);
    printf("v=%d e=%d\n", (int)y, ret);

    printf("KeywordMap:\n");
    for (uint i = 0; i < KeywordMap.count(); i++)
    {
        printf("%d - %s\n", i, KeywordMap.toStringI(i));
    }

    printf("EntityMap:\n");
    for (uint i = 0; i < EntityMap.count(); i++)
    {
        printf("%d - %s\n", i, EntityMap.toStringI(i));
    }

    printf("PrimitiveMap:\n");
    for (uint i = 0; i < sPrimitiveMap.count(); i++)
    {
        printf("%d - %s\n", i, sPrimitiveMap.toStringI(i));
    }
}

void testSplitter()
{
    const char* ex1 = "main[j].clas1.close.localenum";
    printf("\n%s with .\n", ex1);
    base::Splitter split(ex1, '.');
    while (!split.eof())
    {
        printf("tok=[%s]\n", split.get());
    }

    const char* ex2 = "fuck#this#'shit !'#   is#\"cool shit\"";
    printf("\n%s with #\n", ex2);
    base::Splitter split2(ex2, '#');
    while (!split2.eof())
    {
        printf("tok=[%s]\n", split2.get());
    }

    const char* ex3 = "F:\\src\\primal proj\\build\\Debug\\playg.exe";
    printf("\n%s with \\\n", ex3);
    base::Splitter split3(ex3, '\\');
    while (!split3.eof())
    {
        printf("tok=[%s]\n", split3.get());
    }

    const char* ex4 = "playg.exe";
    printf("\n%s with \\\n", ex4);
    base::Splitter split4(ex4, '\\');
    while (!split4.eof())
    {
        printf("tok=[%s]\n", split4.get());
    }

    const char* ex5 = "";
    printf("\n%s with \\\n", ex5);
    base::Splitter split5(ex5, '\\');
    while (!split5.eof())
    {
        printf("tok=[%s]\n", split5.get());
    }
}

class PlEntity
{
    // Fake
public:
    int fake = 0;
};
typedef PlEntity *EntityType;
typedef int PlUnit;
enum ETag
{
    test
};
constDef L_UNITWIDE = "unitwide";


class SymbolTable
{
public:
    SymbolTable()
    {
    }
    ~SymbolTable() = default;
    NOCOPY(SymbolTable)

    void init();
    void pushScope(const char* name);
    void popScope();
    EntityType find(const char* symbol);
    bool add(const char* symbol, EntityType entity, PlUnit* unit = nullptr);
    void dumpSymbols(base::StrBld& bld);
    void dbgDump()
    {
#ifdef _DEBUG
        base::StrBld bld;
        dumpSymbols(bld);
        dbglog("%s\n", bld.c_str());
#endif
    }

private:
    base::Variant mTbl;
    std::string mScope;

    std::string parent(std::string path)
    {
        size_t pos = path.find_last_of('.');
        if (pos == std::string::npos)
        {
            return std::string();
        }
        return path.substr(0, pos);
    }
};

void SymbolTable::init()
{
    mTbl.createObject();
    pushScope(L_UNITWIDE);
}

void SymbolTable::pushScope(const char* name)
{
    std::string keyname(name);
    keyname += '>';
    base::Variant& obj = mTbl.objPath(mScope, '.');
    if (!obj.isObject())
    {
        obj.createObject();
    }
    if (!obj.hasKey(keyname.c_str()))
    {
        obj.setProp(keyname.c_str());
        obj[keyname.c_str()].createObject();
    }
    if (!mScope.empty())
    {
        mScope.append(".");
    }
    mScope.append(keyname);

    dbglog("SymbolTable push scope, current: %s\n%s\n", mScope.c_str(), obj.toString().c_str());
}

void SymbolTable::popScope()
{
    mScope = parent(mScope);
    dbglog("SymbolTable pop scope: %s\n", mScope.c_str());
}

EntityType SymbolTable::find(const char* symbol)
{
    std::string sc = mScope;
    while (!sc.empty())
    {
        base::Variant& obj = mTbl.objPath(sc, '.');
        base::Variant v = obj[symbol];
        if (v.isPtrVal())
        {
            EntityType entity = (EntityType)v.getPtrVal();
            dbglog("find found '%s' in scope '%s'\n", symbol, sc.c_str());
            return entity;
        }
        sc = parent(sc);
    }
    dbglog("Symbol not found '%s' in scope '%s'\n", symbol, sc.c_str());
    return nullptr;
}

bool SymbolTable::add(const char* symbol, EntityType en, PlUnit* unit)
{
    dbglog("add '%s' in scope '%s' %llx (%s) symtbl=%llx\n", symbol, mScope.c_str(), (uint64)en, "unit->mName.c_str()", (uint64)this);

    base::Variant& obj = mTbl.objPath(mScope, '.');
    base::Variant v;
    v.setPtrVal(en);
    obj.setProp(symbol, v);

    // en->addAttrib(EAttribFlags::a_symbol);
    return true;
}

void SymbolTable::dumpSymbols(base::StrBld& bld)
{
    base::Buffer buf;
    mTbl.getYaml(buf);
    bld.append((const char*)buf.cptr(), buf.size());
}


void testScope()
{
    SymbolTable symbols;
    symbols.init();

    symbols.pushScope("math");

    symbols.add("sin", nullptr);
    symbols.add("cos", nullptr);
    symbols.add("tan", nullptr);
    symbols.add("fun1", nullptr);
    symbols.add("fun2", nullptr);

    symbols.pushScope("class1");
    symbols.add("_decl_", nullptr);
    symbols.add("open", nullptr);
    symbols.add("close", nullptr);
    symbols.popScope();

    symbols.add("x", nullptr);
    symbols.add("y", nullptr);
    symbols.popScope();

    symbols.dbgDump();

    symbols.pushScope("math");
    symbols.pushScope("class1");
    EntityType x = symbols.find("_decl_");
    if (x)
    {
        printf("symbol returned %s\n", (const char *)x);
    }
}

// enum class MyFlags : uint32_t
// {
//     ReplaceExisting = (1 << 0),
//     HardlinkSource = (1 << 1),
//     CopySource = (1 << 2),
//     MoveSource = (1 << 3),
// };

// template<class T>
// class Flags
// {

// public:
//     uint32_t value;
// };

void testFile()
{
    using namespace base;

    Path readfn = "c:\\x\\server.c";
    Path writefn;
    writefn.set(readfn.dirPart(), "out.c");

    printf("read=%s\n", readfn.c_str());
    printf("write=%s\n", writefn.c_str());

    // File f;
    // f.open(readfn, __f_open::read);
    // auto rr = f.readEntire(false);

    // rr.value.dbgDump("After readEntire call");

    // printf("Bytes read=%zu err=%x\n", rr.value.size(), (uint)rr.error);

    // File wf;
    // wf.open(writefn, __f_open::write);
    // auto wr = wf.write(rr.value);

    // printf("Bytes written=%d err=%x\n", wr.value, (uint)wr.error);

    copyDirectory("C:\\x\\test\\primal", "C:\\x\\test\\primal2");

    std::vector<base::Path> filenames;
    std::vector<base::Path> dirnames;

    listDirectory("c:\\x\\", &filenames, &dirnames);
    for (auto x : filenames)
    {
        printf("file: %s\n", x.c_str());
    }
    for (auto x : dirnames)
    {
        printf("dir: %s\n", x.c_str());
    }
}

void testYaml()
{
    base::Buffer buf;
    if (!buf.readFile("c:\\x\\test.json", true))
    {
        return;
    }

    base::Variant v;
    v.parseJson((const char *)buf.cptr());

    base::Variant yaml;
    base::Variant obj;
    obj.createObject();
    obj.setProp("name", "helloworld");
    obj.setProp("version", "0.1.0");
    obj.setProp("edition", "2022");
    yaml.createObject();
    yaml.setProp("unit", obj);
    // yaml.createArray();
    // yaml.push(obj);

    printf("yaml obj: %s\n", yaml.toString().c_str());

    printf("%s\n\n%s\n", yaml.toJsonString().c_str(), yaml.toYamlString().c_str());
    printf("%s\n", v.toYamlString().c_str());

    TEST("WriteYaml")
    yaml.getYaml(buf);
    RES = buf.writeFile("test.yaml");
    TESTEND()
}

//--------------------------------------------------------------------------------------------------------------------

// #define isFlagSet(value, flag)         ( ((value) & (flag)) != 0 )
// #define isFlagClear(value, flag)       ( ((value) & (flag)) == 0 )
// #define setFlag(value, flag)           { (value) |= (flag); }
// #define clearFlag(value, flag)         { (value) &= ~(flag); }

// void testBitmap()
// {
//     Bitmap bm(1000);
//     bm.alloc();
//     bm.setBit(5, true);
//     bm.setBit(10, true);
//     bm.setBit(31, true);
//     bm.setBit(90, true);
//     bm.setBit(997, true);
//     bm.setBit(0, true);

//     // for (uint i = 0; i < bm.bits(); i++)
//     // {
//     //     bm.setBit(i, true);
//     // }
//     bm.dbgDump();
// }

#if 0
class MemPool
{
    class PoolBlock;

public:
    MemPool() = delete;
    MemPool(size_t elemsize, size_t poolsize) : mElemSize(elemsize),
                                                mPoolSize(poolsize)
    {
    }

private:
    size_t mElemSize;
    size_t mPoolSize;
    ObjArray<PoolBlock> mPools;

    class PoolBlock
    {
    public:
        PoolBlock()
        {
        }
        void init()
        {
            //            mBuf.alloc(mElemSize * mPoolSize + bitmapsize)
        }

    private:
        base::Buffer mBuf;
    };
};
#endif
void testObjPool()
{
    printf("Testing arrays\n");
    // testArray();
    // testBitmap();
}

// SsoBuffer class template is used to declare a memory buffer which has enough
// space to store bytes a MAXFIXED size. For larger buf, heap allocation is used.
// 1Otherwise no memory is allocated.
template <size_t MAXFIXED>
class SsoBuffer
{
public:
    SsoBuffer()
    {
        mPtr = &mFixed;
    }
    ~SsoBuffer()
    {
        if (!isFixed())
        {
            free(mPtr);
        }
    }
    bool isFixed() const
    {
        return (mPtr == &mFixed);
    }
    void *ptr()
    {
        return mPtr;
    }
    size_t size()
    {
        return (isFixed() ? MAXFIXED : mDyn.mAllocSize);
    }
    void *ensure(size_t desiredsize)
    {
        if (size() < desiredsize)
        {
            expand(desiredsize);
        }
        return mPtr;
    }

private:
    void *mPtr;
    union
    {
        struct
        {
            size_t mAllocSize;
        } mDyn;
        unsigned char mFixed[MAXFIXED];
    };

    void expand(size_t desiredsize)
    {
        dbglog("Sso[: fixed=%d %p size=%zu\n", isFixed(), ptr(), size());
        // Need to increase size
        size_t newsize = size() * 2;
        if (newsize < desiredsize)
        {
            newsize = desiredsize;
        }
        if (isFixed())
        {
            // Currently fixed, allocate dync buffer, and copy fixed into it
            void *dp = malloc(newsize);
            memcpy(dp, mPtr, MAXFIXED);
            mPtr = dp;
        }
        else
        {
            // Already dynamic, realloc
            mPtr = ::realloc(mPtr, newsize);
        }
        mDyn.mAllocSize = newsize;
        dbglog("Sso]: fixed=%d %p size=%zu\n", isFixed(), ptr(), size());
    }
};

void testSsoBuf()
{
    SsoBuffer<8> buf;
    printf("sizeof(sso)=%zu\n", sizeof(buf));

    const char *pattern = "{yasser asmi}";
    for (int i = 0; i < 511; i++)
    {
        char *s = (char *)(buf.ensure(i + 2));
        s[i] = pattern[i % strlen(pattern)];
        s[i + 1] = '\0';
    }
    const char *s = (char *)(buf.ptr());
    printf("len=%zu\n%s\n", strlen(s), s);
}

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
// #include <sys/time.h>
#include <time.h>
#include <ctime>

using namespace std;

int Foo()
{
#pragma omp parallel
    int n = 0;
    {
        for (long i = 0; i < 50000; i++)
            for (long j = 0; j < 50000; j++)
                n++;
    }
    return n;
}

unsigned long long rdtsc()
{
    unsigned a, d;

    __asm__ volatile("rdtsc"
                     : "=a"(a), "=d"(d));

    return ((unsigned long long)a) | (((unsigned long long)d) << 32);
}

void Time()
{
    time_t t1, t2;

    time(&t1);
    Foo();
    time(&t2);

    cout << "time() : " << t2 - t1 << " s" << endl;
}

void Clock()
{
    clock_t c1 = clock();
    Foo();
    clock_t c2 = clock();
    cout << "clock() : " << (float)(c2 - c1) / (float)CLOCKS_PER_SEC << " s" << endl;
}

void RDTSC()
{
    unsigned long long t1, t2;

    t1 = rdtsc();
    Foo();
    t2 = rdtsc();

    cout << "rdtsc() : " << 1.0 * (t2 - t1) / 3199987.0 / 1000.0 << " s" << endl;
}

void TICKS()
{
    unsigned long long t1, t2;

    t1 = getTickMS();
    Foo();
    t2 = getTickMS();

    cout << "getTicks() : " << (t2 - t1) / 1000.0 << " s" << endl;
}

#ifdef _WIN32

void QPC()
{
    // Init
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER t1, t2;
    QueryPerformanceCounter(&t1);
    Foo();
    QueryPerformanceCounter(&t2);

    cout << "QPC() : " << (double)(t2.QuadPart - t1.QuadPart) / frequency.QuadPart << " s" << endl;
}
#endif

#ifndef _WIN32
void GetTimeOfDay()
{
    timeval t1, t2, t;
    gettimeofday(&t1, NULL);
    Foo();
    gettimeofday(&t2, NULL);
    timersub(&t2, &t1, &t);

    cout << "gettimeofday() : " << t.tv_sec + t.tv_usec / 1000000.0 << " s" << endl;
}

void ClockGettime()
{
    timespec res, t1, t2;
    clock_getres(CLOCK_REALTIME, &res);

    clock_gettime(CLOCK_REALTIME, &t1);
    Foo();
    clock_gettime(CLOCK_REALTIME, &t2);

    cout << "clock_gettime() : "
         << (t2.tv_sec - t1.tv_sec) + (float)(t2.tv_nsec - t1.tv_nsec) / 1000000000.0
         << " s" << endl;
}
#endif

void testClocks()
{
    // Windows:
    // time() : 4 s
    // clock() : 3.67500 s
    // rdtsc() : 3.33989 s
    // getTicks() : 3.67100 s
    // QPC() : 3.67284 s

    // Linux:
    // time() : 3 s
    // clock() : 3.68322 s
    // clock_gettime() : 3.74350 s
    // gettimeofday() : 3.63688 s
    // rdtsc() : 3.30714 s
    // getTicks() : 3.65300 s

    cout.setf(ios::fixed);
    cout.setf(ios::showpoint);
    cout.precision(5);

    Time();
    Clock();
#ifndef _WIN32

    ClockGettime();
    GetTimeOfDay();
#endif
    RDTSC();

    TICKS();
#ifdef _WIN32
    QPC();
#endif
}

#define AttribFlagList(_en_) \
    _en_(flag0, 0)            \
    _en_(flag1, 1)            \
    _en_(flag2, 2)            \
    _en_(flag20, 20)          \
    _en_(flag31, 31)
flagsdef32(AttribFlagList, AttribFlags);

#define AttribFlagList64(_en_) \
    _en_(flag0, 0)            \
    _en_(flag1, 1)            \
    _en_(flag2, 2)            \
    _en_(flag20, 20)          \
    _en_(flag63, 63)
flagsdef64(AttribFlagList64, AttribFlags64);

void testFlags()
{
    AttribFlags fl;
    printf("val=0x%08x\n", fl.value());
    TESTEXP("is 0 clear\n", fl.isClear(AttribFlags::flag0));
    fl.set(AttribFlags::flag0);
    TESTEXP("is 0 clear\n", !fl.isClear(AttribFlags::flag0));
    fl.set(AttribFlags::flag20);
    printf("val=0x%08x\n", fl.value());
    fl.set(AttribFlags::flag31);
    printf("val=0x%08x\n", fl.value());
    fl.clear(AttribFlags::flag20);
    TESTEXP("val=0x80000001\n", fl.value() == 0x80000001);

    AttribFlags64 fl64;
    printf("val=0x%llx\n", fl64.value());
    TESTEXP("is 0 clear\n", fl64.isClear(AttribFlags64::flag0));
    fl64.set(AttribFlags64::flag0);
    TESTEXP("is 0 clear\n", !fl64.isClear(AttribFlags64::flag0));
    printf("val=0x%llxx\n", fl64.value());
    printf("is 0 clear=%d\n", fl64.isClear(AttribFlags64::flag0));
    fl64.set(AttribFlags64::flag20);
    printf("val=0x%llxx\n", fl64.value());
    fl64.set(AttribFlags64::flag63);
    printf("val=0x%llxx\n", fl64.value());
    printf("is 20 set=%d\n", fl64.isSet(AttribFlags64::flag20));
    fl64.clear(AttribFlags64::flag20);
    printf("is 20 set=%d\n", fl64.isSet(AttribFlags64::flag20));
    printf("val=0x%llxx\n", fl64.value());
    TESTEXP("val=0x8000000000100001\n", fl64.value() == 0x8000000000000001ull);
}


void looptest(int fromval, int toval)
{
    printf("fwd: from=%d to=%d\n", fromval, toval);
    for (auto i = fromval; i < toval; i++)
    {
        printf("%d ", i);
    }
    printf("\n");

    printf("fwd inclusive: from=%d to=%d\n", fromval, toval);
    for (auto i = fromval; i <= toval; i++)
    {
        printf("%d ", i);
    }
    printf("\n");

    printf("rev: from=%d to=%d\n", fromval, toval);
    for (auto i = toval; i-- > fromval; )
    {
        printf("%d ", i);
    }
    printf("\n");

    printf("rev inclusive: from=%d to=%d\n", fromval, toval);
    for (auto i = toval + 1; i-- > fromval; )
    {
        printf("%d ", i);
    }
    printf("\n\n");
}

void testLoops()
{
    looptest(-5, 10);
    looptest(1, 5);
    looptest(4, 8);
    looptest(0, 0);
}

void readtextfile(strparam fn)
{
    base::Buffer buf;
    if (buf.readFile(fn.data(), false))
    {
        base::Splitter sp(buf, '\n');
        sp.ignore('\r');
        while (!sp.eof())
        {
            size_t l;
            const char* s = sp.get(&l);
            printf("[%s]\n", s);
        }
    }
}


int main(int argc, char **argv)
{
    setFlag(base::sBaseGFlags, base::GFLAG_VERBOSE_SHELL | base::GFLAG_VERBOSE_FILESYS);
    return RUNTESTS(argc, argv);
}