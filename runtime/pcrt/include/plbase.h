#pragma once


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WIN32
#include <stddef.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <initializer_list>

// Basic types

using int8 = int8_t;
using uint8 = uint8_t;

using int16 = int16_t;
using uint16 = uint16_t;

using int32 = int32_t;
using uint32 = uint32_t;

using int64 = int64_t;
using uint64 = uint64_t;

using usize = size_t;

using czstr = const char*;
using zstr = char*;

using float32 = float;
using float64 = double;


#ifndef NDEBUG
#ifndef _DEBUG
    #define _DEBUG
#endif
#else
    #undef _DEBUG
#endif

// TODO: define flag limits
#define isFlagSet(value, flag)         ( ((value) & (flag)) != 0 )
#define isFlagClear(value, flag)       ( ((value) & (flag)) == 0 )
#define setFlag(value, flag)           { (value) |= (flag); }
#define clearFlag(value, flag)         { (value) &= ~(flag); }

#define countof(x)  ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// Main entry point
extern void pcrtmain();

// TODO: Remove these and replace with proper API
extern int sProcArgc;
extern char** sProcArgv;


// Basic atomics.  Uses either Microsoft's intrinsic or gcc builtins

#ifdef _MSC_VER
extern "C" long _InterlockedIncrement(long volatile*);
extern "C" long _InterlockedDecrement(long volatile*);
#ifdef __x86_64__
extern "C" __int64 _InterlockedIncrement64(__int64 volatile*);
extern "C" __int64 _InterlockedDecrement64(__int64 volatile*);
#endif
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#ifdef __x86_64__
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#endif
#endif

#ifndef ALLOW_CRT_MALLOC
// Disallow calls to malloc, free etc
#define calloc(c, s)       _NOT_ALLOWED_
#define free(p)            _NOT_ALLOWED_
#define malloc(s)          _NOT_ALLOWED_
#define _msize(p)          _NOT_ALLOWED_
#define realloc(p, s)      _NOT_ALLOWED_
#endif

#define printf              _NOT_ALLOWED_

inline int32 atomicIncrement(volatile int32* v)
{
#ifdef _MSC_VER
    return _InterlockedIncrement((long volatile*)v);
#else
    return __sync_add_and_fetch(v, 1);
#endif
}

inline int64 atomicIncrement(volatile int64* v)
{
#ifdef _MSC_VER
    return _InterlockedIncrement64((__int64 volatile*)v);
#else
    return __sync_add_and_fetch(v, 1);
#endif
}

inline int32 atomicDecrement(volatile int32* v)
{
#ifdef _MSC_VER
    return _InterlockedDecrement((long volatile*)v);
#else
    return __sync_add_and_fetch(v, -1);
#endif
}

inline int64 atomicDecrement(volatile int64* v)
{
#ifdef _MSC_VER
    return _InterlockedDecrement64((__int64 volatile*)v);
#else
    return __sync_add_and_fetch(v, -1);
#endif
}

// Basic OS output
void osPrint(const char* str, usize len);

// Basic OS specific mutex.
class Mutex
{
public:
    Mutex();
    ~Mutex();
    void lock();
    bool tryLock();
    void unlock();
private:
// Size of OS dependant mutex data (CRITICAL_SECTION or pthread counterpart).
// Done this way so Windows.h or pthread.h doesn't need to be included here
#ifdef _WIN32
#if INTPTR_MAX == INT64_MAX
#define OS_MUTEX_SIZE_64    5
#else
#define OS_MUTEX_SIZE_64    3
#endif
#else
#if INTPTR_MAX == INT64_MAX
#define OS_MUTEX_SIZE_64    5
#else
#define OS_MUTEX_SIZE_64    3
#endif
#endif
    uint64 mMutData[OS_MUTEX_SIZE_64];
};

class AutoLock
{
public:
    explicit AutoLock(Mutex& mutex) :
        mMut(mutex)
    {
        mMut.lock();
    }
    ~AutoLock()
    {
        mMut.unlock();
    }
private:
    Mutex& mMut;
};

// Variable argument type
#define vararg(T)  std::initializer_list<T>


// TODO: Reconsolile this with outher debug routines... move all at this level
void printeno(int eno, const char* func);

#define dbgeno(eno) \
   do { if (eno) {printeno(eno, __PRETTY_FUNCTION__);} } while (0)

