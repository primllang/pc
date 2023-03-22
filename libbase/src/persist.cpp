#include "util.h"
#include "persist.h"

namespace base
{

// PersistWr::

void PersistWr::wrData(PersistType dt, const void* p, size_t l)
{
    //dbglog("wrData(%llx)%s dt=%d l=%zu\n", mLen, (dt == PersistType::d_str ? "[str]" : ""), (uint)dt, l);
    char* buf = ensureAlloc(mLen + l + 1);
    if (dt != PersistType::d_none)
    {
        buf[mLen++] = (char)dt;
    }
    memcpy(buf + mLen, p, l);
    mLen += l;
}

void PersistWr::incCount(size_t loc, PersistType dt)
{
    char* buf = (char*)mBuf.ptr();
    if ((loc + sizeof(uint32)) >= mLen)
    {
        dbgerr("Invalid location for PersistWr array/obj\n");
        return;
    }
    if ((PersistType)buf[loc] != dt)
    {
        dbgerr("Invalid elem/field call\n");
        return;
    }
    (*((uint32*)(buf + loc + 1)))++;
}

char* PersistWr::ensureAlloc(size_t neededsize)
{
    if (neededsize > mBuf.size())
    {
        mBuf.dblOr(neededsize);
    }
    return (char*)mBuf.ptr();
}


// PersistRd::
bool PersistRd::load(const base::Path& fn)
{
    bool ret = mBuf.readFile(fn, false);
    mPos = 0;
    if (ret && !mSig.empty())
    {
        ret = (mBuf.size() >= mSig.length());
        if (ret)
        {
            std::string sig((const char*)mBuf.cptr(), mSig.length());
            ret = base::streql(mSig, sig);
            if (ret)
            {
                mPos += sig.length();
            }
        }
    }
    if (ret)
    {
        ret = (peek() == PersistType::d_begfile);
    }
    if (!ret)
    {
        dbgerr("Can't load %s or is invalid\n", fn.c_str());
        mBuf.free();
    }
    mPos++;
    return ret;
}

std::string PersistRd::rdStr()
{
    std::string s;
    if (!boundsCheck(mPos))
    {
        return s;
    }

    char* buf = (char*)mBuf.ptr();
    if ((PersistType)buf[mPos] != PersistType::d_str)
    {
        dbgerr("Data is not a string\n");
        mErr = true;
        return s;
    }
    // Skip past the type, and calculate the length of the string
    auto pos = mPos++;
    while (boundsCheck(pos) && buf[pos] != '\0')
    {
        pos++;
    }
    if (buf[pos] != '\0' || mErr)
    {
        dbgerr("Invalid string data\n");
        mErr = true;
        return s;
    }
   
    size_t lennz = pos - mPos;
    s.append(buf + mPos, lennz);
    mPos += lennz + 1;

    return s;
}

bool PersistRd::boundsCheck(size_t pos)
{
    if (mErr)
    {
        dbgerr("PersistRd in error\n");
        return false;
    }
    if (pos >= mBuf.size())
    {
        dbgerr("PersistRd out of bounds\n");
        mErr = true;
        return false;
    }
    return true;
}

bool PersistRd::rdData(void* p, size_t l, PersistType checkdt)
{
    if (!boundsCheck(mPos + l) || p == nullptr || l == 0)
    {
        mErr = true;
        return false;
    }

    char* buf = (char*)mBuf.ptr();

    //dbglog("rdData(%llx) l=%zu chkdt=%d actdt=%d\n", mPos, l, (uint)checkdt, buf[mPos]);

    // Current position is at the type, if asked,
    // check the type is right before proceeding
    if (checkdt != PersistType::d_none)
    {
        if ((PersistType)buf[mPos] != checkdt)
        {
            dbgerr("Specified type doesn't match data\n");
            mErr = true;
            return false;
        }
    }
    // Skip past the type
    mPos++;

    memcpy(p, buf + mPos, l);
    mPos += l;
    return true;
}    


} // base