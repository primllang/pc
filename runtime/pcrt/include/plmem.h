#pragma once

#include "plbase.h"
#include "pldef.h"

// Placement new
inline void* operator new(usize size, void* where) noexcept
{
    (void)size;
    return where;
}
#define __PLACEMENT_NEW_INLINE

#include <utility>

namespace primal
{

// Memory allocation interface
class IMemAlloc
{
public:
    virtual void* _malloc(usize size) = 0;
    virtual void* _zalloc(usize size) = 0;
    virtual void* _realloc(void* p, usize newsize) = 0;
    virtual void _free(void* p) = 0;
};

extern IMemAlloc* sDefaultMemAlloc;
#ifdef _DEBUG
extern usize sAllocCnt;
extern usize sFreeCnt;
#endif

void initDefaultMemAlloc();

// Buffer class maintains an allocated chunk of memory.  It takes care of freeing the memory
// when the object goes out of scope.  It uses malloc/free/realloc. It can also read a file into the buffer.
class Buffer
{
public:
    Buffer() :
        mMemory(nullptr),
        mSize(0)
    {
    }
    explicit Buffer(usize size) :
        mMemory(nullptr),
        mSize(0)
    {
        allocMem(size);
    }
    // Copy constructor
    Buffer(const Buffer& src) :
        mMemory(nullptr),
        mSize(0)
    {
        copyFrom(src);
    }
    // Move constructor
    Buffer(Buffer&& src) :
        mMemory(nullptr),
        mSize(0)
    {
        moveFrom(src);
    }

    ~Buffer()
    {
        freeMem();
    }
    void* ptr()
    {
        return mMemory;
    }
    const void* cptr() const
    {
        return mMemory;
    }
    usize size() const
    {
        return mSize;
    }

    void allocMem(usize size);
    void reAllocMem(usize size);
    void dblMem(usize neededsize);
    void freeMem();

    // Copies the memory from the provided buffer into this buffer
    void copyFrom(const Buffer& src);

    // Moves the memory from the provided buffer into this buffer.  The provided
    // buffer is emptied.  No actual memcpy is done.
    void moveFrom(Buffer& src);

    void zero()
    {
        if (mMemory)
        {
            memset(mMemory, 0, mSize);
        }
    }
private:
    void* mMemory;
    usize mSize;
};


// SsoBuffer class template is used to declare a memory buffer which has enough
// space to store bytes a MAXFIXED size. For larger buf, heap allocation is used.
// For smaller buffers, no memory is allocated.  It can also point to a readonly
// outside buffer and can do copy-on-write when needed.
template <usize MAXFIXED>
class SsoBuffer
{
public:
    SsoBuffer()
    {
        mPtr = &mInternalBuf;
    }
    explicit SsoBuffer(const void* ptr, usize siz)
    {
        // Point
        mPtr = const_cast<void*>(ptr);
        mData.mPtrMemSize = siz;
        mData.mExtAllocSize = 0;
    }
    SsoBuffer(const SsoBuffer& that) :
        SsoBuffer()
    {
        // Copy constructor
        if (that.isInternal())
        {
            memcpy(mPtr, that.mPtr, MAXFIXED);
        }
        else if (that.isPtrMem())
        {
            mPtr = that.mPtr;
            mData = that.mData;
        }
        else
        {
            copy(that);
        }
    }
    SsoBuffer(SsoBuffer&& that) :
        SsoBuffer()
    {
        // Move constructor
        if (that.isInternal())
        {
            memcpy(mPtr, that.mPtr, MAXFIXED);
        }
        else
        {
            mPtr = that.mPtr;
            mData = that.mData;
            that.mPtr = &that.mInternalBuf;
        }
    }
    ~SsoBuffer()
    {
        clear();
    }
    usize size() const
    {
        // NOTE: size is buffer size which may be larger than the size of contents
        return (isInternal() ? MAXFIXED : (isPtrMem() ? mData.mPtrMemSize : mData.mExtAllocSize));
    }
    const void* cptr() const
    {
        return mPtr;
    }
    void* ensure(usize writesize)
    {
        if (isPtrMem())
        {
            copyOnWrite(writesize);
        }
        else if (size() < writesize)
        {
            expand(writesize);
        }
        return mPtr;
    }
    void point(const void* ptr, usize siz)
    {
        clear();
        mPtr = const_cast<void*>(ptr);
        mData.mPtrMemSize = siz;
        mData.mExtAllocSize = 0;
    }
    void copy(const void* ptr, usize siz)
    {
        clear();
        void* p = ensure(siz);
        memcpy(p, ptr, siz);
    }
    void copy(const SsoBuffer& that)
    {
        copy(that.cptr(), that.size());
    }
    void clear()
    {
        if (!isInternal() && mData.mExtAllocSize)
        {
            sDefaultMemAlloc->_free(mPtr);
        }
        mPtr = &mInternalBuf;
    }

private:
    void* mPtr;
    union
    {
        struct
        {
            usize mExtAllocSize;
            usize mPtrMemSize;
        } mData;
        uint8 mInternalBuf[MAXFIXED];
    };

    bool isInternal() const
    {
        return (mPtr == &mInternalBuf);
    }
    bool isExtAlloc() const
    {
        return (mPtr != &mInternalBuf) && mData.mExtAllocSize;
    }
    bool isPtrMem() const
    {
        return (mPtr != &mInternalBuf) && mData.mPtrMemSize;
    }
    void expand(usize desiredsize)
    {
        // Need to increase size
        usize newsize = size() * 2;
        if (newsize < desiredsize)
        {
            newsize = desiredsize;
        }
        if (isInternal())
        {
            // Currently internal, allocate external buffer, and copy fixed into it
            void* dp = sDefaultMemAlloc->_malloc(newsize);
            memcpy(dp, mPtr, MAXFIXED);
            mPtr = dp;
        }
        else
        {
            // Already external, realloc
            mPtr = sDefaultMemAlloc->_realloc(mPtr, newsize);
        }
        mData.mExtAllocSize = newsize;
        mData.mPtrMemSize = 0;
    }
    void copyOnWrite(usize desiredsize)
    {
        void* ptr = mPtr;
        usize siz = mData.mPtrMemSize;
        usize newsize = desiredsize > siz ? desiredsize : siz;
        if (newsize < MAXFIXED)
        {
            mPtr = &mInternalBuf;
        }
        else
        {
            mPtr = sDefaultMemAlloc->_malloc(newsize);
            mData.mExtAllocSize = newsize;
            mData.mPtrMemSize = 0;
        }
        memcpy(mPtr, ptr, siz);
    }
};

class MemArray
{
public:
    MemArray() = delete;
    MemArray(usize elemsize) :
        mElemSize(elemsize),
        mElemCount(0)
    {
    }
    ~MemArray() = default;
    void* insert(usize pos);
    void* append()
    {
        return insert(mElemCount);
    }
    bool remove(usize pos);
    void* get(usize pos)
    {
        return (char*)(mBuf.ptr()) + (pos * mElemSize);
    }
    usize length()
    {
        return mElemCount;
    }
    usize capacity()
    {
        return mBuf.size() / mElemSize;
    }
    void reserve(usize elemcount)
    {
        if (elemcount > capacity())
        {
            ensureAlloc(elemcount);
        }
    }
    void clear()
    {
        mBuf.freeMem();
        mElemCount = 0;
    }

protected:
    usize mElemSize;
    usize mElemCount;
    Buffer mBuf;

    void ensureAlloc(usize desiredlen)
    {
        usize newcap = desiredlen * mElemSize;
        if (newcap != mBuf.size())
        {
            mBuf.reAllocMem(newcap);
        }
    }
};

class Bitmap
{
public:
    Bitmap() = delete;
    Bitmap(usize bitcount) :
        mBitCount(bitcount),
        mWordMem(nullptr),
        mAlloc(nullptr)
    {
        // Must call either alloc() or setMem() to point to bitmap memory
    }
    ~Bitmap()
    {
        clear();
    }
    usize bits()
    {
        return mBitCount;
    }
    usize bytes()
    {
        return words() * WORDBITS / 8;
    }
    usize words()
    {
        return (mBitCount + WORDBITS - 1) / WORDBITS;
    }
    usize bitsPerWord()
    {
        return WORDBITS;
    }
    void allocMem();
    void useMem(void *ptr, usize size);

    bool getBit(usize index)
    {
        #ifdef _DEBUG
        if (!rangeCheck(index))
        {
            return false;
        }
        #endif
        return (mWordMem[index / WORDBITS] & (1ull << (index % WORDBITS))) != 0;
    }
    void setBit(usize index, bool val)
    {
        #ifdef _DEBUG
        if (!rangeCheck(index))
        {
            return;
        }
        #endif
        if (val)
        {
            mWordMem[index / WORDBITS] |= (1ull << (index % WORDBITS));
        }
        else
        {
            mWordMem[index / WORDBITS] &= ~(1ull << (index % WORDBITS));
        }
    }
    uint64 getWord(usize windex)
    {
        #ifdef _DEBUG
        if (!rangeCheck(windex * WORDBITS))
        {
            return 0;
        }
        #endif
        return mWordMem[windex];
    }
    void setWord(usize windex, uint64 val)
    {
        #ifdef _DEBUG
        if (!rangeCheck(windex * WORDBITS))
        {
            return;
        }
        #endif
        mWordMem[windex] = val;
    }
    void zeroAll()
    {
        for (usize i = 0; i < words(); i++)
        {
            setWord(i, 0);
        }
    }
    void oneAll()
    {
        for (usize i = 0; i < words(); i++)
        {
            setWord(i, MAXWORD);
        }
    }
    bool findFirstZero(usize* bit);
    void dbgDump();

private:
    constMemb WORDBITS = 64;
    constMemb_(uint64) MAXWORD = 0xFFFFFFFFFFFFFFFF;

    usize mBitCount;
    uint64* mWordMem;
    void* mAlloc;

    void clear()
    {
        if (mAlloc)
        {
            sDefaultMemAlloc->_free(mAlloc);
            mAlloc = nullptr;
        }
        mWordMem = nullptr;
    }
    bool rangeCheck(usize index);
};


class MemPool
{
    class Block
    {
    public:
        Block() = delete;
        Block(usize elemsize, usize blocksize);
        ~Block() = default;
        void* allocAddr();
        bool freeAddr(void* addr);

        bool inRange(void* addr)
        {
            return ((char*)addr >= (char*)calcElemAddr(0)) &&
                   ((char*)addr < (char*)calcElemAddr(mBmp.bits()));
        }
        bool hasRoom()
        {
            return mAllocCnt < mBmp.bits();
        }
        bool isEmpty()
        {
            return mAllocCnt == 0;
        }

    private:
        usize mElemSize;
        usize mAllocCnt;
        usize mNextFree;
        primal::Buffer mBuf;
        primal::Bitmap mBmp;

        void* calcElemAddr(usize ofs)
        {
            return ((char*)mBuf.ptr()) + (mElemSize * ofs);
        }
    };

    class BlockArray : public MemArray
    {
    public:
        BlockArray() :
            MemArray(sizeof(Block))
        {
        }
        ~BlockArray()
        {
            for (usize i = 0; i < MemArray::length(); i++)
            {
                Block* obj = (Block*)MemArray::get(i);
                obj->~Block();
            }
        }
        void* insertMem(int pos)
        {
            return MemArray::insert(pos);
        }
        void* appendMem()
        {
            return MemArray::append();
        }
        bool remove(usize pos)
        {
            Block* obj = get(pos);
            if (obj == nullptr)
            {
                return false;
            }
            obj->~Block();
            return MemArray::remove(pos);
        }
        Block* get(usize pos)
        {
            if (pos >= length())
            {
                return nullptr;
            }
            return (Block*)MemArray::get(pos);
        }
    };

public:
    MemPool() = delete;
    MemPool(usize elemsize, usize initblocksize, usize incinterval = 0) :
        mElemSize(elemsize),
        mInitBlockSize(initblocksize),
        mIncInterval(incinterval)
    {
    }
    ~MemPool() = default;
    void* allocElem(bool* createdblock);
    bool freeElem(void* addr, bool* deletedblock);
    usize blockCount()
    {
        return mPoolBlocks.length();
    }
    usize currentBlockSize()
    {
        usize bs = ((mIncInterval == 0) ? 1 : ((mPoolBlocks.length() / mIncInterval) + 1)) * mInitBlockSize;
        return bs;
    }
    void config(usize initblocksize, usize incinterval)
    {
        mInitBlockSize = initblocksize;
        mIncInterval = incinterval;
    }

private:
    usize mElemSize;
    uint32 mInitBlockSize;
    uint32 mIncInterval;
    BlockArray mPoolBlocks;

    Block* newBlock()
    {
        return new (mPoolBlocks.appendMem()) Block(mElemSize, currentBlockSize());
    }
};

} // namespace primal

