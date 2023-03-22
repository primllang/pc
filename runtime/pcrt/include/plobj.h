#pragma once

#include "plbase.h"
#include "plmem.h"

namespace primal
{

// PlObject is what every Primal object (not value types) is derived from.
// PlObject only has a default constuctor

class PlObject
{
public:
    PlObject() :
        mStrongRC(0),
        mWeakRC(0),
        mMemAlloc(sDefaultMemAlloc)
    {
        //dbgfnc();
    }
    PlObject(const PlObject &) = delete;
    ~PlObject() = default;
    PlObject& operator=(const PlObject &) = delete;

private:
template <class> friend class PlRef;
template <class> friend class PlWeakRef;

    int32 mStrongRC;
    int32 mWeakRC;
    IMemAlloc* mMemAlloc;
};


template <class InterfType>
class PlInterfRef;

// PlRef (strong) and PlWeakRef (weak) classes are references to an object derived from PlObject.
// The class must be derived from base PlObject and objects must be created with a call to PlRef<objtype>::createObject()
// Two reference counters (strong/weak) are maintained by these classes inside the PlObject.
// The object is destroyed when the last strong reference is released. Memory is deallocated when all
// references (strong and weak) are released.

// Strong Ref
template <class ObjType>
class PlRef
{
public:
    PlRef(ObjType* obj) :
        mObj(obj)
    {
        //dbglog("   PlRef(objptr*)--> ", _D((int64)obj, 16));
        incRef(mObj);
    }
    PlRef(PlRef& orig)
    {
        //TODO: Should param be const
        mObj = orig.mObj;
        incRef(mObj);
    }
    ~PlRef()
    {
        //dbgfnc();
        decRef(mObj);
    }
    ObjType* operator->()
    {
        //dbgfnc();
        return mObj;
    }
    const ObjType* operator->() const
    {
        return mObj;
    }
    bool operator==(const PlRef& right) const
    {
        return mObj == right.mObj;
    }
    PlRef& operator=(PlRef& right)
    {
        if (this == &right)
        {
            return *this;
        }
        incRef(right.mObj);
        decRef(mObj);
        mObj = right.mObj;
        return *this;
    }
    operator bool()
    {
        return mObj != nullptr;
    }

    static ObjType* createObject(IMemAlloc* memalloc = nullptr)
    {
        //dbgfnc();
        ObjType* obj;
        if (memalloc == nullptr)
        {
            memalloc = sDefaultMemAlloc;
        }

        obj = (ObjType*) (memalloc->_malloc(sizeof(ObjType)));

        // Call the constuctor (only the default constructor)
        new(obj) ObjType();
        ((PlObject*)obj)->mMemAlloc = memalloc;

        return obj;
    }

    template<class IntfType>
    operator PlInterfRef<IntfType>()
    {
        // TODO: Handle exceptions
        return PlInterfRef<IntfType>(mObj, dynamic_cast<IntfType*>(mObj));
    }

    // template<class IntfType>
    // PlInterfRef<IntfType> getIntf()
    // {
    //     // TODO: Handle exceptions
    //     return PlInterfRef<IntfType>(mObj, dynamic_cast<IntfType*>(mObj));
    // }

    static void incRef(ObjType* obj)
    {
        if (obj)
        {
            //dbglog(__PRETTY_FUNCTION__, " [Strong] ref:", _D(obj->mStrongRC), "->", _D(obj->mStrongRC + 1));
            atomicIncrement(&obj->mStrongRC);
        }
    }
    static void decRef(ObjType* obj)
    {
        if (obj)
        {
            assert(obj->mStrongRC > 0);
            //dbglog(__PRETTY_FUNCTION__, " [Strong] ref:", _D(obj->mStrongRC), "->", _D(obj->mStrongRC - 1));
            if (atomicDecrement(&obj->mStrongRC) == 0)
            {
                // Destroy object by calling the derived objects' destructor
                obj->~ObjType();

                // If the weak reference count is also 0, release the memory
                if (obj->mWeakRC == 0)
                {
                    //dbglog("   freeing memory--> ", _D((int64)obj, 16));
                    obj->mMemAlloc->_free((void*)obj);
                }
            }
        }
    }

private:
template <class> friend class PlWeakRef;

    ObjType* mObj;
};


// Weak Ref
template <class ObjType>
class PlWeakRef
{
public:
    PlWeakRef() :
        mObj(nullptr)
    {
    }
    PlWeakRef(ObjType* obj) :
        mObj(obj)
    {
        incWeakRef(mObj);
    }
    PlWeakRef(PlWeakRef& orig)
    {
        mObj = orig.mObj;
        incWeakRef(mObj);
    }
    ~PlWeakRef()
    {
        decWeakRef();
    }
    ObjType* operator->()
    {
        return mObj;
    }
    const ObjType* operator->() const
    {
        return mObj;
    }
    bool operator==(const PlWeakRef& right)
    {
        return mObj == right.mObj;
    }
    PlWeakRef& operator=(PlWeakRef& right)
    {
        if (this == &right)
        {
            return *this;
        }
        incWeakRef(right.mObj);
        decWeakRef();
        mObj = right.mObj;
        return *this;
    }
    PlWeakRef& operator=(PlRef<ObjType>& right)
    {
        incWeakRef(right.mObj);
        decWeakRef();
        mObj = right.mObj;
        return *this;
    }
    bool isAlive()
    {
        // TODO: validate and test this
        return (mObj->mStrongRC > 0);
    }
    operator bool()
    {
        // TODO: validate and test this
        return mObj != nullptr && isAlive();
    }

private:
template <class> friend class PlRef;

    ObjType* mObj;

    void incWeakRef(ObjType* obj)
    {
        if (obj)
        {
            dbglog("incWeakRef:", _D(obj->mWeakRC), _D(obj->mWeakRC + 1));
            atomicIncrement(&obj->mWeakRC);
        }
    }
    void decWeakRef()
    {
        if (mObj)
        {
            dbglog("decWeakRef:", _D(mObj->mWeakRC), _D(mObj->mWeakRC - 1));
            if (atomicDecrement(&mObj->mWeakRC) == 0)
            {
                if (mObj->mStrongRC == 0)
                {
                    dbglog("   decWeakRef()--> ", _D((int64)mObj, 16));
                    mObj->mMemAlloc->_free((void*)mObj);
                }
            }
        }
    }
};


template <class InterfType>
class PlInterfRef
{
public:
    using ObjType = primal::PlObject;

    PlInterfRef()  = delete;
    PlInterfRef(ObjType* obj, InterfType* intf) :
        mObj((ObjType*)obj)
    {
        PlRef<ObjType>::incRef(mObj);
        mIntf = intf;
    }
    // PlInterfRef(PlRef<ObjType>& orig)
    // {
    //     mObj = orig.mObj;
    //     PlRef<ObjType>::incRef(mObj);

    //     // TODO: Handle exceptions
    //     mIntf = dynamic_cast<InterfType*>(mObj);
    // }
    ~PlInterfRef()
    {
        PlRef<ObjType>::decRef(mObj);
    }
    InterfType* operator->()
    {
        return mIntf;
    }
    const InterfType* operator->() const
    {
        return mIntf;
    }
    // PlInterfRef& operator=(PlRef<ObjType>& right)
    // {
    //     PlRef<ObjType>::decRef(mObj);

    //     mObj = orig.mObj;
    //     PlRef<ObjType>::incRef(mObj);

    //     // TODO: Handle exceptions
    //     mIntf = dynamic_cast<InterfType*>(mObj);

    //     return *this;
    // }

private:
template <class> friend class PlRef;

    ObjType* mObj;
    InterfType* mIntf;
};

template <typename T>
class PlBoxObj : public PlObject
{
public:
    PlBoxObj() = default;
    PlBoxObj(T data) :
        mData(data)
    {
    }
    T mData;
};

} // namespace primal
