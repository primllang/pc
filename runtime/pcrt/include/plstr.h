#pragma once

#include "plbase.h"
#include "plmem.h"

namespace primal
{

class string
{
public:
    string() :
        mLen(0)
    {
    }
    string(czstr str)
    {
        if (str)
        {
            mLen = strlen(str);
            mBuf.point(str, mLen + 1);
        }
        else
        {
            clear();
        }
    }
    explicit string(czstr str, usize len)
    {
        // Point
        if (str)
        {
            mLen = len;
            mBuf.point(str, mLen + 1);
        }
        else
        {
            clear();
        }
    }
    string(const string& that) :
        mBuf(that.mBuf),
        mLen(that.mLen)
    {
        // Copy
    }
    string(string&& that) :
        mBuf(std::move(that.mBuf)),
        mLen(that.mLen)
    {
        // Move
    }
    ~string() = default;
    void append(char c)
    {
        zstr buf = ensureAlloc(mLen + 2);
        buf[mLen++] = c;
    }
    void append(usize count, char c)
    {
        zstr buf = ensureAlloc(mLen + count + 1);
        for (usize i = 0; i < count; i++)
        {
            buf[mLen++] = c;
        }
    }
    void append(czstr s, usize l)
    {
        zstr buf = ensureAlloc(mLen + l + 1);
        memcpy(buf + mLen, s, l);
        mLen += (int)l;
    }
    void append(czstr s)
    {
        append(s, strlen(s));
    }
    void append(const string& sb)
    {
        append(sb.cz(), sb.length());
    }
    void replaceLast(char c)
    {
        zstr buf = ensureAlloc(mLen + 1);
        if (mLen > 0)
        {
            buf[mLen - 1] = c;
        }
    }
    void eraseLast()
    {
        if (mLen > 0)
        {
            mLen--;
        }
    }
    char last()
    {
        czstr p = (czstr)mBuf.cptr();
        return mLen > 0 ? p[mLen - 1] : '\0';
    }
    char first()
    {
        czstr p = (czstr)mBuf.cptr();
        return mLen > 0 ? p[0] : '\0';
    }
    czstr cz() const
    {
        zstr buf = const_cast<string*>(this)->ensureAlloc(mLen + 1);
        // Lazy null termination
        buf[mLen] = '\0';
        return buf;
    }
    usize length() const
    {
        return mLen;
    }
    bool empty()
    {
        return mLen == 0;
    }
    void reserve(usize neededsize)
    {
        (void)ensureAlloc(neededsize);
    }
    usize cap()
    {
        return mBuf.size();
    }
    zstr wrbuf(usize neededsize = 32)
    {
        return ensureAlloc(neededsize);
    }

    int compare(czstr s) const
    {
        czstr cp = cz();
        if (cp)
        {
            return strcmp(cp, s);
        }
        else
        {
            return cp != s;
        }
    }
    int compare(const string& s) const
    {
        return compare(s.cz());
    }
    bool equals(czstr s)
    {
        return compare(s) == 0;
    }
    bool equals(char c)
    {
        zstr p = (zstr)mBuf.cptr();
        return (mLen == 1 && *p == c);
    }
    char operator[](usize i)
    {
        czstr p = (czstr)mBuf.cptr();
        return (i < mLen) ? p[i] : '\0';
    }
    bool exist(char c)
    {
        return (strchr(cz(), c) != NULL);
    }
    void assign(czstr str)
    {
        assign(str, strlen(str));
    }
    void assign(czstr str, usize len)
    {
        // Doesn't need to be zero terminated
        zstr buf = ensureAlloc(len + 1);
        memcpy(buf, str, len);
        mLen = len;
    }
    void clear()
    {
        mBuf.clear();
        mLen = 0;
    }
    bool empty() const
    {
        return mLen == 0;
    }

    string& operator=(const string& that)
    {
        if (this != &that)
        {
            mBuf.copy(that.mBuf);
            mLen = that.mLen;
        }
        return *this;
    }
    string& operator+=(const string& that)
    {
        append(that);
        return *this;
    }
    string& operator+=(char c)
    {
        append(c);
        return *this;
    }
    string operator+(const string& that) const
    {
        string s(*this);
        s.append(that);
        return s;
    }
    bool operator==(const string& that) const
    {
        return compare(that) == 0;
    }
    bool operator!=(const string& that) const
    {
        return compare(that) != 0;
    }

    static string uint64Str(uint64 num, int8 base, bool minus = false);
    static string int64Str(int64 num);
    static string float64Str(float64 num);

private:
    SsoBuffer<32> mBuf;
    usize mLen;

    zstr ensureAlloc(usize neededsize)
    {
        return (zstr)mBuf.ensure(neededsize);
    }
};

inline primal::string Fmt(uint64 num, usize base = 10)
{
    return string::uint64Str(num, base);
}
inline primal::string Fmt(uint32 num)
{
    return string::uint64Str(num, 10);
}
inline primal::string Fmt(uint16 num)
{
    return string::uint64Str(num, 10);
}
inline primal::string Fmt(uint8 num)
{
    return string::uint64Str(num, 10);
}
inline primal::string Fmt(int64 num)
{
    return string::int64Str(num);
}
inline primal::string Fmt(int32 num)
{
    return string::int64Str(num);
}
inline primal::string Fmt(int16 num)
{
    return string::int64Str(num);
}
inline primal::string Fmt(int8 num)
{
    return string::int64Str(num);
}
inline primal::string Fmt(float64 num)
{
    return string::float64Str(num);
}
inline primal::string Fmt(float32 num)
{
    return string::float64Str(num);
}
inline primal::string Fmt(primal::string s)
{
    return s;
}

} // namespace primal

void printva_(vararg(primal::string) list);
#define printv(...)  printva_({__VA_ARGS__})

void prints(const char* str);

usize argcount_();
primal::string argstr_(usize n);


#ifdef _MSC_VER
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
#endif


// Debug sensitive functions

#ifdef _DEBUG
// This is a debug build

#define dbglog(...) \
    do { printva_({__VA_ARGS__}); } while (0)

#define dbgerr(...) \
    do { printva_({"Error: ", __FILE__, _D(__LINE__), __VA_ARGS__}); } while (0)

#ifdef _MSC_VER
void __cdecl __debugbreak(void);
#define dbgbreak() __debugbreak()
#define dbgfnc()  dbglog(__FUNCSIG__);
#else
#define dbgbreak() __builtin_trap()
#define dbgfnc()  dbglog("[", __PRETTY_FUNCTION__, "]");
#endif

#else
// This is not a debug build

#define dbglog(...)

#define dbgerr(...) \
    do { printva_({"Error: ", ## __VA_ARGS__}); } while (0)

#define dbgbreak() ((void)0)
#define dbgfnc() ((void)0)

#endif

