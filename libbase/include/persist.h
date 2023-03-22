#pragma once

#include "util.h"
#include "sys.h"

namespace base
{

enum class PersistType : byte
{
    d_none,
    d_begfile,
    d_done,
    d_str,
    d_int64,
    d_uint64,
    d_int32,
    d_uint32,
    d_float,
    d_char,
    d_bool,
    d_dbl,
    d_arr,
    d_endelem,
    d_obj,
    d_endfield
};

class PersistWr
{
public:
    PersistWr() = delete;
    PersistWr(const char* sig) :
        mLen(0)
    {
        mBuf.alloc(64);
        wrData(PersistType::d_none, (void*)sig, strlen(sig));
        wrType(PersistType::d_begfile);
    }
    void clear()
    {
        mLen = 0;
    }
    size_t length()
    {
        return mLen;
    }
    // Write
    void wrStr(const char* str)
    {
        uint32 len = (uint32)strlen(str);
        wrData(PersistType::d_str, (void*)str, len + 1);
    }
    void wrInt64(int64 x)
    {
        wrData(PersistType::d_int64,(void*)&x, sizeof(x));
    }
    void wrUInt64(uint64 x)
    {
        wrData(PersistType::d_uint64,(void*)&x, sizeof(x));
    }
    void wrDbl(double x)
    {
        wrData(PersistType::d_dbl,(void*)&x, sizeof(x));
    }
    void wrInt32(int32 x)
    {
        wrData(PersistType::d_int32,(void*)&x, sizeof(x));
    }
    void wrUInt32(uint32 x)
    {
        wrData(PersistType::d_uint32,(void*)&x, sizeof(x));
    }
    void wrBool(bool x)
    {
        wrData(PersistType::d_bool,(void*)&x, sizeof(x));
    }
    void wrChar(char x)
    {
        wrData(PersistType::d_char,(void*)&x, sizeof(x));
    }
    size_t wrArr()
    {
        size_t ret = mLen;
        uint32 len = 0;
        wrData(PersistType::d_arr, (void*)&len, sizeof(len));
        return ret;
    }
    void endElem(size_t arrloc)
    {
        //dbglog("endElem(%llx)\n", mLen);
        incCount(arrloc, PersistType::d_arr);
        wrType(PersistType::d_endelem);
    }
    size_t wrObj()
    {
        size_t ret = mLen;
        uint32 len = 0;
        wrData(PersistType::d_obj, (void*)&len, sizeof(len));
        return ret;
    }
    void endField(size_t objloc)
    {
        incCount(objloc, PersistType::d_obj);
        wrType(PersistType::d_endfield);
    }
    bool save(const base::Path& fn)
    {
        if (isFlagSet(base::sBaseGFlags, base::GFLAG_VERBOSE_FILESYS))
        {
            dbglog("Persisting to file %s\n", fn.c_str());
        }
        return mBuf.writeFile(fn, mLen);
    }
    void moveToBuffer(base::Buffer& buf)
    {
        if (mLen == 0)
        {
            buf.free();
        }
        else
        {
            mBuf.reAlloc(mLen);
            buf.moveFrom(mBuf);
            mLen = 0;
        }
    }
private:
    void wrType(PersistType dt)
    {
        char* buf = ensureAlloc(mLen + 1);
        buf[mLen++] = (char)dt;
    }
    void wrData(PersistType dt, const void* p, size_t l);
    void incCount(size_t loc, PersistType dt);
    char* ensureAlloc(size_t neededsize);

protected:
    base::Buffer mBuf;
    size_t mLen;
};

class PersistRd
{
public:
    PersistRd() = delete;
    PersistRd(const char* sig) :
        mPos(0),
        mErr(false)
    {
        if (sig)
        {
            mSig = sig;
        }
    }
    bool load(const base::Path& fn);

    PersistType peek()
    {
        char* buf = (char*)mBuf.ptr();
        return boundsCheck(mPos) ? (PersistType)(buf[mPos]) : PersistType::d_done;
    }
    bool inErr()
    {
        return mErr;
    }
    void setErr(const char* msg)
    {
        dbgerr("Persist read error: %s\n", msg);
        mErr = true;
    }
    int64 rdInt64()
    {
        int64 x;
        (void)rdData((void*)&x, sizeof(int64), PersistType::d_int64);
        return x;
    }
    uint64 rdUInt64()
    {
        uint64 x;
        (void)rdData((void*)&x, sizeof(uint64), PersistType::d_uint64);
        return x;
    }
    double rdDbl()
    {
        double x;
        (void)rdData((void*)&x, sizeof(double), PersistType::d_dbl);
        return x;
    }
    int32 rdInt32()
    {
        int32 x;
        (void)rdData((void*)&x, sizeof(int32), PersistType::d_int32);
        return x;
    }
    uint32 rdUInt32()
    {
        uint32 x;
        (void)rdData((void*)&x, sizeof(uint32), PersistType::d_uint32);
        return x;
    }
    bool rdBool()
    {
        bool x;
        (void)rdData((void*)&x, sizeof(bool), PersistType::d_bool);
        return x;
    }
    char rdChar()
    {
        char x;
        (void)rdData((void*)&x, sizeof(char), PersistType::d_char);
        return x;
    }
    std::string rdStr();

    uint32 rdArr()
    {
        uint32 len = 0;
        rdData(&len, sizeof(len), PersistType::d_arr);
        return len;
    }
    bool endElem()
    {
        //dbglog("endElem(%llx)\n", mPos);
        if (peek() == PersistType::d_endelem)
        {
            mPos++;
            return true;
        }
        mErr = true;
        return false;
    }

private:
    bool boundsCheck(size_t pos);
    bool rdData(void* p, size_t l, PersistType checkdt = PersistType::d_none);

protected:
    std::string mSig;
    base::Buffer mBuf;
    size_t mPos;
    bool mErr;
};


} // base
