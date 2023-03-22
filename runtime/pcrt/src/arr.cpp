#include "plmem.h"
#include "plstr.h"
#include "plarr.h"

namespace primal
{

void* MemArraySrt::addOrModify(const void* elem, bool modifyfound /*= true */)
{
    usize pos;
    void* elemptr;

    if (binSearch(elem, pos))
    {
        if (modifyfound)
        {
            elemptr = get(pos);
        }
        else
        {
            elemptr = nullptr;
        }
    }
    else
    {
        elemptr = insert(pos);
    }
    if (elem)
    {
        memcpy(get(pos), elem, mElemSize);
    }
    return elemptr;
}

bool MemArraySrt::remove(const void* elem)
{
    usize pos;
    if (!binSearch(elem, pos))
    {
        return false;
    }
    return MemArray::remove(pos);
}

void* MemArraySrt::find(const void* elem)
{
    usize pos;
    if (!binSearch(elem, pos))
    {
        return nullptr;
    }
    return get(pos);
}

bool MemArraySrt::binSearch(const void* findelem, usize &pos)
{
    assert(mComp);
    if (length() == 0 || mComp == nullptr)
    {
        pos = 0;
        return false;
    }

    usize low = 0;
    usize high = length() - 1;
    while (low <= high)
    {
        usize mid = (low + high) / 2;
        usize res = (*mComp)(get(mid), findelem);

        if (res == 0)
        {
            pos = mid;
            return true;
        }
        else if (res < 0)
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }

    pos = low;
    return false;
}

} // namespace primal