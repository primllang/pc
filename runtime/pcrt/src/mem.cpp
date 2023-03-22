#define ALLOW_CRT_MALLOC
#include "plmem.h"
#include "plstr.h"

#ifdef _WIN32
// Windows
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace primal
{

IMemAlloc* sDefaultMemAlloc;
#ifdef _DEBUG
usize sAllocCnt;
usize sFreeCnt;
#endif

#ifdef _WIN32

class HeapMemAlloc : public IMemAlloc
{
public:
    HeapMemAlloc()
    {
        mHeap = GetProcessHeap();
    }
    ~HeapMemAlloc() = default;

    void* _malloc(usize size) override
    {
#ifdef _DEBUG
        sAllocCnt++;
#endif
        void* p = HeapAlloc(mHeap, 0, size);
        //dbglog("_malloc:", _D((int64)size), "ptr", _D((int64)p, 16));
        return p;
    }
    void* _zalloc(usize size) override
    {
#ifdef _DEBUG
        sAllocCnt++;
#endif
        return HeapAlloc(mHeap, HEAP_ZERO_MEMORY, size);
    }
    void* _realloc(void* p, usize newsize) override
    {
        if (p)
        {
            return HeapReAlloc(mHeap, 0, p, newsize);
        }
        else
        {
#ifdef _DEBUG
        sAllocCnt++;
#endif
            return HeapAlloc(mHeap, 0, newsize);
        }
    }
    void _free(void* p) override
    {
#ifdef _DEBUG
        sFreeCnt++;
#endif
        HeapFree(mHeap, 0, p);
    }

private:
    HANDLE mHeap;
} sHeapMemAlloc;

void initDefaultMemAlloc()
{
    sDefaultMemAlloc = &sHeapMemAlloc;
}

#else
// Linux

class CrtMemAlloc : public IMemAlloc
{
public:
    CrtMemAlloc() = default;
    ~CrtMemAlloc() = default;

    void* _malloc(usize size) override
    {
        void* ptr = malloc(size);
        //dbglog("_malloc:", _D((int64)size), " ptr=", _D((int64)ptr, 16));
        return ptr;
        //return malloc(size);
    }
    void* _zalloc(usize size) override
    {
        return calloc(size, 1);
    }
    void* _realloc(void* p, usize newsize) override
    {
        return realloc(p, newsize);
    }
    void _free(void* p) override
    {
        //dbglog("_free: ptr ", _D((int64)p, 16));
        free(p);
    }
} sCrtMemAlloc;


void initDefaultMemAlloc()
{
    sDefaultMemAlloc = &sCrtMemAlloc;
}

#endif // Linux


// Buffer::

void Buffer::allocMem(usize size)
{
    freeMem();
    mMemory = sDefaultMemAlloc->_malloc(size);
    if (mMemory == NULL)
    {
        //dbgerr("failed to allocate ", _D(size));
        return;
    }
    mSize = size;
    //dbgDump("Alloc");
}

void Buffer::reAllocMem(usize size)
{
    if (size == 0)
    {
        freeMem();
        return;
    }
    void* p = sDefaultMemAlloc->_realloc(mMemory, size);
    if (p == NULL)
    {
        //dbgerr("failed to allocate \n", _D(size));
        return;
    }

    mMemory = p;
    mSize = size;
    //dbgDump("Realloc");
}

void Buffer::dblMem(usize neededsize)
{
    usize newsize = mSize;

    if (newsize == 0)
    {
        newsize = 64;
    }
    else
    {
        newsize *= 2;
    }
    if (newsize < neededsize)
    {
        newsize = neededsize;
    }
    reAllocMem(newsize);
}

void Buffer::freeMem()
{
    if (mMemory != NULL)
    {
        //dbgDump("Free");
        sDefaultMemAlloc->_free(mMemory);
        mMemory = NULL;
        mSize = 0;
    }
}

void Buffer::copyFrom(const Buffer& src)
{
    reAllocMem(src.size());
    memcpy(ptr(), src.cptr(), src.size());
}

void Buffer::moveFrom(Buffer& src)
{
    freeMem();
    mMemory = src.mMemory;
    mSize = src.mSize;
    src.mMemory = NULL;
    src.mSize = 0;
}

// Bitmap::

bool Bitmap::rangeCheck(usize index)
{
    if (index >= mBitCount || mWordMem == nullptr)
    {
        dbglog("Bitmap index out of range ", _D(index));
        return false;
    }
    return true;
}

void Bitmap::allocMem()
{
    clear();
    mAlloc = sDefaultMemAlloc->_malloc(bytes());
    mWordMem = (uint64 *)mAlloc;
    zeroAll();
}

void Bitmap::useMem(void *ptr, usize size)
{
    if (size != bytes())
    {
        dbglog("Incorrect bitmap memory size");
        return;
    }
    clear();
    mWordMem = (uint64 *)ptr;
}

bool Bitmap::findFirstZero(usize* bit)
{
    usize ofs = 0;
    for (usize i = 0; i < words(); i++)
    {
        // If found a word that is not all ones
        if (getWord(i) != MAXWORD)
        {
            for (usize j = 0; j < WORDBITS; j++, ofs++)
            {
                if (!getBit(ofs))
                {
                    *bit = ofs;
                    return true;
                }
            }
        }
        ofs += WORDBITS;
    }
    return false;
}

void Bitmap::dbgDump()
{
#ifdef _DEBUG
    usize cnt = 0;
    for (usize i = 0; i < words(); i++)
    {
        for (usize j = 0; j < bitsPerWord(); j++)
        {
            if (cnt >= bits())
            {
                break;
            }
            usize w = getWord(i);
            prints(((w >> j) & 1ull) ? "1 " : "0 ");
            cnt++;
        }
        prints("\n");
        if (cnt >= bits())
        {
            break;
        }
    }
#endif
}


// MemPool::PoolBlock::

MemPool::Block::Block(usize elemsize, usize blocksize) :
    mElemSize(elemsize),
    mAllocCnt(0),
    mNextFree(0),
    mBmp(blocksize)
{
    // Pool block layout:
    //    Elem 0
    //    Elem 1
    //    ...
    //    Elem (blocksize - 1) where blocksize = bmp.bits()
    //    Bitmap where 0=free, 1=alloc
    mBuf.allocMem(mElemSize * blocksize + mBmp.bytes());
    mBmp.useMem(calcElemAddr(mBmp.bits()), mBmp.bytes());

    mBmp.zeroAll();
}

void* MemPool::Block::allocAddr()
{
    if (mNextFree >= mBmp.bits())
    {
        if (hasRoom())
        {
            if (!mBmp.findFirstZero(&mNextFree))
            {
                 return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }

    mBmp.setBit(mNextFree, true);
    mAllocCnt++;
    return calcElemAddr(mNextFree++);
}

bool MemPool::Block::freeAddr(void* addr)
{
    if (inRange(addr))
    {
        usize bitno = ((char*)addr - (char*)calcElemAddr(0)) / mElemSize;
        mBmp.setBit(bitno, false);
        mAllocCnt--;
        mNextFree = bitno;
        return true;
    }
    return false;
}

// MemPool::

void* MemPool::allocElem(bool* createdblock)
{
    // Try to allocate a block, beginning at the last and moving back
    usize i = mPoolBlocks.length();
    while (i--)
    {
        Block* bp = mPoolBlocks.get(i);
        if (bp->hasRoom())
        {
            *createdblock = false;
            return bp->allocAddr();
        }
    }
    // Couldn't find room, allocate address on a new block
    *createdblock = true;
    return newBlock()->allocAddr();
}

bool MemPool::freeElem(void* addr, bool* deletedblock)
{
    *deletedblock = false;
    // Find the block where the address being free is in range of the pool start/end address
    for (usize i = 0; i < mPoolBlocks.length(); i++)
    {
        Block* bp = mPoolBlocks.get(i);

        // Try to ask block to delete the address which it will do only if the addr exists inside the block
        if (bp->freeAddr(addr))
        {
            // If there are no more addresses allocated in the block, delete the block.
            if (bp->isEmpty())
            {
                mPoolBlocks.remove(i);
                *deletedblock = true;
            }
            return true;
        }
    }
    return false;
}

// MemArray::

void* MemArray::insert(usize pos)
{
    if (pos > mElemCount)
    {
        return nullptr;
    }

    usize cap = capacity();
    if (mElemCount >= cap)
    {
        // Expand
        ensureAlloc((cap == 0) ? 4 : (cap * 2));
    }

    void* elemptr = get(pos);
    usize shift = (mElemCount - pos);
    // dbglog("insert memmove(%d, %d, %d)\n", pos + 1, pos, shift);
    if (shift > 0)
    {
        memmove(get(pos + 1), elemptr, mElemSize * shift);
    }
    mElemCount++;

    return elemptr;
}

bool MemArray::remove(usize pos)
{
    if (pos >= length())
    {
        return false;
    }

    usize shift = (length() - pos - 1);

    // dbglog("delete memmove(%d, %d, %d)\n", pos, pos + 1, shift);
    if (shift > 0)
    {
        memmove(get(pos), get(pos + 1), mElemSize * shift);
    }
    mElemCount--;

    // Shrink
    if (length() <= (capacity() / 2))
    {
        ensureAlloc(length());
    }

    return true;
}


} // namespace primal