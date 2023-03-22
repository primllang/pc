
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
#include "persist.h"

#include "tests.h"

class stringparam
{
public:
    stringparam()
    {
        clear();
    }
    stringparam(const char* str)
    {
        mStr = str;
        mLen = strlen(str);
    }
    explicit stringparam(const char* str, size_t len)
    {
        mStr = str;
        mLen = len;
    }
    stringparam(const stringparam& that)
    {
        mStr = that.mStr;
        mLen = that.mLen;
    }
    stringparam(stringparam&& that) noexcept
    {
        mStr = that.mStr;
        mLen = that.mLen;
        that.clear();
    }
    ~stringparam() = default;
    void clear()
    {
        static char blank[] = "";
        mStr = blank;
        mLen = 0;
    }
    const char* cstr() const
    {
        return mStr;
    }
    const size_t length() const
    {
        return mLen;
    }
    char last() const
    {
        return mLen > 0 ? mStr[mLen - 1] : '\0';
    }
    char first() const
    {
        return mLen > 0 ? mStr[0] : '\0';
    }
    bool isEmpty() const
    {
        return mLen == 0;
    }
private:
    const char* mStr;
    size_t mLen;
};


class Fmt
{
public:
    Fmt()
    {
        mLen = 0;
    }
    explicit Fmt(uint64 num, int base)  
    {
        mLen = uint64Str(num, mBuf, base);
    }
    explicit Fmt(uint64 num)  
    {
        mLen = uint64Str(num, mBuf, 10);
    }
    explicit Fmt(uint32 num)  
    {
        mLen = uint64Str(num, mBuf, 10);
    }
    explicit Fmt(uint16 num)  
    {
        mLen = uint64Str(num, mBuf, 10);
    }
    explicit Fmt(uint8 num)  
    {
        mLen = uint64Str(num, mBuf, 10);
    }
    explicit Fmt(int64 num)  
    {
        mLen = int64Str(num, mBuf);
    }
    explicit Fmt(int32 num)  
    {
        mLen = int64Str(num, mBuf);
    }
    explicit Fmt(int16 num)  
    {
        mLen = int64Str(num, mBuf);
    }
    explicit Fmt(int8 num)  
    {
        mLen = int64Str(num, mBuf);
    }
    
    ~Fmt() = default;

    stringparam sp()
    {
        return stringparam(mBuf, mLen);
    }
    operator stringparam()
    {
        return sp();
    }
    Fmt& str(uint64 num, int base = 10)
    {
        mLen = uint64Str(num, mBuf, base);
        return *this;
    }
    Fmt& lower()
    {
        for (int i = 0; i < mLen; i++)
        {
            char c = mBuf[i];
            if (c >= 'A' && c <= 'Z')
            {
                mBuf[i] = c + 'a' - 'A';
            }
        }
        return *this;
    }
    Fmt& upper()
    {
        for (int i = 0; i < mLen; i++)
        {
            char c = mBuf[i];
            if (c >= 'a' && c <= 'z')
            {
                mBuf[i] = c - 'a' + 'A';
            }
        }
        return *this;
    }

private:
    char mBuf[256];
    size_t mLen;

    // TBD: Support functions (TODO: make private) and consolidate with the static function
    size_t uint64Str(uint64 num, char* str, uint64 base = 10)
    {
        // NOTE: This code assumes the underlying str buffer is large enough 
        // to store the string version of a number of upto 64 bits

        // Convert to string (but chars will be in reverse order)
        size_t i = 0;
        do
        {
            char digit = (char)(num % base);
            digit += (digit < 0xA) ? '0' : ('A' - 0xA);
            str[i++] = digit;

            num /= base;
        
        } while (num);

        str[i] = '\0';
        size_t slen = i;

        // Reverse the string
        i--;
        for (size_t j = 0; j < i; j++, i--)
        {
            char tmp = str[i];
            str[i] = str[j];
            str[j] = tmp;
        }
        return slen;
    }

    size_t int64Str(int64 num, char* str)
    {
        if (num < 0)
        {
            str[0] = '-';
            return (1 + uint64Str(-1 * num, str, 10));
        }
        else
        {
            return uint64Str(num, str, 10);
        }
    }    
};


void printva_(std::initializer_list<stringparam> list);
#define print(...)  outputV({__VA_ARGS__})

void printva_(std::initializer_list<stringparam> list)
{
    bool appendspace = false;
    for (auto& s : list)
    {
        if (appendspace)
        {
            printf(" ");
            appendspace = false;
        }
        if (s.length() != 0)
        {
            appendspace = (s.last() != ' ');
            printf("%s", s.cstr());
        }
    }
    printf("\n");
}

/*
// Fmt ::
size_t Fmt::uint64Str(uint64 num, char* str, size_t strsize, uint64 base)
{
    // Convert to string (but chars will be in reverse order)
    strsize--;
    size_t i = 0;
    do
    {
        char digit = (char)(num % base);
        if (digit < 0xA)
        {
            str[i++] = '0' + digit;
        }
        else
        {
            str[i++] = 'A' + digit - 0xA;
        }
        num /= base;
    } while (num && (i < strsize));

    str[i] = '\0';
    if (i == strsize && num)
    {
        return 0;
    }

    // Reverse the string
    size_t slen = i;
    i--;
    for (size_t j = 0; j < i; j++, i--)
    {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
    return slen;
}
*/


stringparam funcret()
{
    return stringparam("Return World");
}

void func1(stringparam p)
{
    printf("func1 pass-by-value: ");
    printf("stringparam %s\n", p.cstr());
}

void func2(stringparam &p)
{
    printf("func1 pass-by-ref: ");
    printf("stringparam %s\n", p.cstr());
}

void func3(const stringparam &p)
{
    printf("func1 pass-by-const-ref: ");
    printf("stringparam %s\n", p.cstr());
}
void testStringParam()
{
    stringparam A;
    stringparam B("Hello world");
    func1(B);
    func2(B);
    func3(B);
    func1(std::move(B));
    func1(stringparam("New World"));
    func1("Hi there");
    func1(funcret());
}

void testFmt()
{
    int64 i = 0xDEADBEEF;
    //  $( 2021, 16) .trim().lower()
    //  $i
    //  $(i, hex)
    //  $(i).padRight(20)
    //  $getValue()
    //  $(getValue()).upper()
    //  prepend, append
    //  ltrim, rtrim

    //printf("%s\n", Fmt(i, 16).lower().sp().cstr());
    TESTEXP("NOT IMPLEMENTED", false);
}



void testPersist()
{
    base::PersistWr pwr("ASMI");
    base::PersistRd prd("ASMI");

    // write
    pwr.wrBool(true);
    pwr.wrInt32(20);
    pwr.wrInt64(12309812309812038);
    pwr.wrDbl(1.3);

    auto ar = pwr.wrArr();
    for (int i = 0; i < 10; i++)
    {
        pwr.wrInt32(i);
        pwr.wrStr(base::formatr("Array element %d", i).c_str());
        pwr.wrUInt64(20);
        pwr.endElem(ar);
    }
    pwr.save("testpersist.bin");

    // read
    TEST("PersistRd load()")
    RES = prd.load("testpersist.bin");
    TESTEND()

    bool b = prd.rdBool();
    TESTEXP("rdBool", b == true && !prd.inErr());

    int32 i32 = prd.rdInt32();
    TESTEXP("rdInt32", i32 == 20 && !prd.inErr());

    int64 i64 = prd.rdInt64();
    TESTEXP("rdInt64", i64 == 12309812309812038 && !prd.inErr());

    double db = prd.rdDbl();
    TESTEXP("rdDbl", db == 1.3 && !prd.inErr());

    
    std::string s;
    uint64 ui64;
    uint32 arrlen = prd.rdArr();
    TESTEXP("rdArr", arrlen == 10);
    for (uint32 i = 0; i < arrlen; i++)
    {
        i32 = prd.rdInt32();
        s = prd.rdStr();
        ui64 = prd.rdUInt64();

        prd.endElem();

        printf("%d %lld '%s'\n", i32, i64, s.c_str());
    }

    TESTEXP("End read",  !prd.inErr());

    printf("Done\n");
}


void testPath()
{
    base::Path p("D:\\src\\playg\\base\\test1\\test2");
    base::Path np("D:\\src\\playg\\yasser");
    dbglog("fwd slash %s => %s\n", p.c_str(), p.withFwdSlash().c_str());
 
    np.makeRelTo(p);
    dbglog("relpath: %s\n", np.c_str());

    np.makeAbs();
    dbglog("abspath: %s\n", np.c_str());
}
