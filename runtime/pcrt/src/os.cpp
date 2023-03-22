#include "plmem.h"
#include <stdio.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <pthread.h>
#endif

void printeno(int eno, const char* func)
{
#ifdef _DEBUG
    fprintf(stderr, "Error: '%s(%d)' %s \n", strerror(eno), eno, func == NULL ? "" : func);
#endif
// Use perror()?
}


// Mutex::
Mutex::Mutex()
{
#ifdef _WIN32
    static_assert(sizeof(mMutData) >= sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(&(CRITICAL_SECTION&)mMutData);
#else
    // Recursive to match critical section
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int ret = pthread_mutex_init((pthread_mutex_t*)mMutData, &attr);
    pthread_mutexattr_destroy(&attr);
    dbgeno(ret);
#endif
}

Mutex::~Mutex()
{
#ifdef _WIN32
    DeleteCriticalSection(&(CRITICAL_SECTION&)mMutData);
#else
    pthread_mutex_destroy((pthread_mutex_t*)mMutData);
#endif
}

void Mutex::lock()
{
#ifdef _WIN32
    EnterCriticalSection(&(CRITICAL_SECTION&)mMutData);
#else
    pthread_mutex_lock((pthread_mutex_t*)mMutData);
#endif
}

bool Mutex::tryLock()
{
#ifdef _WIN32
    return TryEnterCriticalSection(&(CRITICAL_SECTION&)mMutData) == TRUE;
#else
    return (pthread_mutex_trylock((pthread_mutex_t*)mMutData) == 0) ? true : false;
#endif
}

void Mutex::unlock()
{
#ifdef _WIN32
    LeaveCriticalSection(&(CRITICAL_SECTION&)mMutData);
#else
    pthread_mutex_unlock((pthread_mutex_t*)mMutData);
#endif
}

void osPrint(const char* str, usize len)
{
    if (str)
    {
#ifdef _WIN32
        if (len == 0)
        {
            len = strlen(str);
        }
        static HANDLE han = INVALID_HANDLE_VALUE;
        if (han == INVALID_HANDLE_VALUE)
        {
            han = GetStdHandle(STD_OUTPUT_HANDLE);
        }
        WriteFile(han, str, (DWORD)len, NULL, NULL);
#else
        fputs(str, stdout);
#endif
    }
}

int sProcArgc;
char** sProcArgv;

int main(int argc, char** argv)
{
    int ret = 0;

    // TODO: add API to access params and set return value
    sProcArgc = argc;
    sProcArgv = argv;

    primal::initDefaultMemAlloc();

    // Call main entry point
    pcrtmain();

    return ret;
}

