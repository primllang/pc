#pragma once
#include "plbase.h"
#include "plobj.h"
#include "plmem.h"
#include <string.h>

namespace primal
{

class MemArraySrt : public MemArray
{
public:
    typedef int (*CompareFunc)(const void* , const void* );

    MemArraySrt() = delete;
    MemArraySrt(usize elemsize, CompareFunc comp) :
        MemArray(elemsize),
        mComp(comp)
    {
    }
    ~MemArraySrt() = default;
    void* add(const void* elem)
    {
        return addOrModify(elem, false);
    }
    void* addOrModify(const void* elem, bool modifyfound = true);
    bool remove(const void* elem);
    void* find(const void* elem);
    bool findPos(const void* findelem, usize& pos)
    {
        return binSearch(findelem, pos);
    }

private:
    CompareFunc mComp;

protected:
    bool binSearch(const void* findelem, usize& pos);
};

template <class T>
class ObjArray : public MemArray
{
public:
    ObjArray() :
        MemArray(sizeof(T))
    {
    }
    ~ObjArray()
    {
        for (usize i = 0; i < MemArray::length(); i++)
        {
            T* obj = (T*)MemArray::get(i);
            obj->~T();
        }
    }

    // Basic functionality
    void* insertMem(usize pos)
    {
        return MemArray::insert(pos);
    }
    T* insert(usize pos)
    {
        T* obj = (T*)MemArray::insert(pos);
        return new (obj) T();
    }
    void* appendMem()
    {
        return MemArray::append();
    }
    void append(const T& a)
    {
        T* obj = (T*)MemArray::append();
        new (obj) T();
        *obj = a;
    }
    bool remove(usize pos)
    {
        T* obj = get(pos);
        if (obj == nullptr)
        {
            return false;
        }
        obj->~T();
        return MemArray::remove(pos);
    }
    void clear()
    {
        for (usize i = 0; i < MemArray::length(); i++)
        {
            T* obj = (T*)MemArray::get(i);
            obj->~T();
        }
        MemArray::clear();
    }
    T* get(usize pos)
    {
        #ifdef _DEBUG
        if (pos >= length())
        {
            dbglog("Invalid array access ", _D(pos));
            return nullptr;
        }
        #endif
        return (T*)MemArray::get(pos);
    }
};


template <class T>
class PlPoolAlloc : public PlObject, IMemAlloc
{
public:
    constMemb DEFAULTPOOLSIZE = 256;

    PlPoolAlloc() :
        mPool(sizeof(T), DEFAULTPOOLSIZE)
    {
    }
    ~PlPoolAlloc()
    {
    }
    IMemAlloc* getMemAlloc()
    {
        return (IMemAlloc*)this;
    }
    // IMemAlloc
    void* _malloc(usize size) override
    {
        assert(size = sizeof(T));
        return allocObj();
    }
    void* _zalloc(usize size) override
    {
        // Not supported on pool
        assert(false);
        return nullptr;
    }
    void* _realloc(void* p, usize newsize) override
    {
        // Not supported on pool
        assert(false);
        return nullptr;
    }
    void _free(void* p) override
    {
        freeObj(p);
    }

private:
    MemPool mPool;

    void* allocObj()
    {
        //dbgfnc();
        bool createdblock;
        void* ptr = mPool.allocElem(&createdblock);
        if (createdblock)
        {
            //dbglog("NewBlock bs=", _D(mPool.currentBlockSize()), " bcount=", _D(mPool.blockCount()));

            // When the first block is added, take a reference on self
            if (mPool.blockCount() == 1)
            {
                PlRef<PlPoolAlloc<T>>::incRef(this);
            }
        }
        return ptr;
    }
    void freeObj(void* p)
    {
        bool deletedblock;
        mPool.freeElem(p, &deletedblock);
        if (deletedblock)
        {
            // When all blocks go away, remove reference to self
            if (mPool.blockCount() == 0)
            {
                PlRef<PlPoolAlloc<T>>::decRef(this);
            }
        }
    }
};


// Iter class template is used to iterate over an array as follows:
//    for (Iter<Obj> i; arr.forEach(i); )
//    {
//        foo(i.obj->field);
//    }
template <class T>
class Iter
{
public:
    Iter() :
        mPos(0),
        mObj{0}
    {
    }
    Iter(usize p) :
        mPos(p),
        mObj{0}
    {
    }
    T operator->()
    {
        return mObj;
    }

public:
    usize mPos;
    T mObj;
};


template <class T>
class PlVector : public PlObject
{
public:
    PlVector() :
        mPool(_NEWOB(PlPoolAlloc<T>))
    {
    }
    ~PlVector() = default;

    usize length()
    {
        return mArr.length();
    }

    PlRef<T> get(usize pos)
    {
        // if (pos >= length())
        // {
        //     // nil
        // }
        return *mArr.get(pos);
    }

    PlRef<T> appendNew()
    {
        // Create the object with the mPool IMemAlloc interface which it allocate it in a pool block
        T* obj = PlRef<T>::createObject(mPool->getMemAlloc());

        // Append an item raw, in place new the PlRef with the obj, and return it
        return *(new (mArr.appendMem()) PlRef<T>(obj));
    }
    PlRef<T> insertNew(usize pos)
    {
        // Create the object with the mPool IMemAlloc interface which it allocate it in a pool block
        T* obj = PlRef<T>::createObject(mPool->getMemAlloc());

        // Insert item raw, in place new the PlRef with the obj, and return it
        return *(new (mArr.insertMem(pos)) PlRef<T>(obj));
    }

    void append(T o)
    {
        T* obj = PlRef<T>::createObject(mPool->getMemAlloc());
        PlRef<T> t = *(new (mArr.appendMem()) PlRef<T>(obj));
        t->mData = o.mData;
    }

    bool remove(usize pos)
    {

    }

    void tune()
    {
    }

    // Forward iterator
    Iter<PlRef<T>> iterFwd()
    {
        return Iter<PlRef<T>>();
    }
    bool iterFwdLoop(Iter<PlRef<T>>& iter)
    {
        if (iter.mPos < mArr.length())
        {
            iter.mObj = *mArr.get(iter.mPos++);
            return true;
        }
        return false;
    }

    // Reverse iterator
    Iter<PlRef<T>> iterRev()
    {
        return Iter<PlRef<T>>(length());
    }
    bool iterRevLoop(Iter<PlRef<T>>& iter)
    {
        if (iter.mPos > 0)
        {
            iter.mObj = *mArr.get(--iter.mPos);
            return true;
        }
        return false;
    }

private:
    PlRef<PlPoolAlloc<T>> mPool;
    ObjArray<PlRef<T>> mArr;
};


} // namespace primal

