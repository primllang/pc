#pragma once


// Console esc codes
#ifndef CONSOLE_ESC_CODES
#define CONSOLE_ESC_CODES
#define ESC_DEFAULT "\033[0m"
#define ESC_BLACK "\033[30m"
#define ESC_RED "\033[31m"
#define ESC_GREEN "\033[32m"
#define ESC_YELLOW "\033[33m"
#define ESC_BLUE "\033[34m"
#define ESC_MAGENTA "\033[35m"
#define ESC_CYAN "\033[36m"
#define ESC_WHITE "\033[37m"
#define ESC_CLEAR_LINE "\033[2K\r"
#define ESC_CLEAR_SCREEN "\033[2J"
#endif

#ifdef PRIMALC
inline void ytprint(const char* s0, const char* s1 = "", const char* s2 = "", const char* s3 = "")
{
    prints(s0);
    prints(s1);
    prints(s2);
    prints(s3);
}
inline void ytprint(long i)
{
    prints(_D((int64)i).cz());
}
#else
#include <stdio.h>
#include <stdlib.h>

inline void ytprint(const char* s0, const char* s1 = "", const char* s2 = "", const char* s3 = "")
{
    fputs(s0, stdout);
    fputs(s1, stdout);
    fputs(s2, stdout);
    fputs(s3, stdout);
}
inline void ytprint(long i)
{
    char buf[128];
    sprintf(buf, "%ld", i);
    ytprint(buf);
}
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <time.h>
#endif


inline int sTestsSucceeded = 0;
inline int sTestsFailed = 0;

#undef TEST
#define TEST(name)         \
    do                            \
    {                             \
        ytprint(ESC_WHITE, "Test: ", name, "...\n"); \
        ytprint(ESC_DEFAULT);     \
        bool RES = true;          \
        do                        \
        {

#undef TESTEND
#define TESTEND() \
        } while (false);          \
        if (!(RES))               \
        {                         \
            sTestsFailed++;       \
            ytprint(ESC_RED, "FAIL: ", __FILE__, ": "); \
            ytprint(__LINE__);    \
            ytprint(ESC_DEFAULT, "\n"); \
        }                         \
        else                      \
        {                         \
            sTestsSucceeded++;    \
            ytprint(ESC_GREEN, "OK\n", ESC_DEFAULT); \
        }                         \
    } while (false);

#define TESTEXP(name, expr) \
    TEST(name)              \
    RES = (expr);           \
    TESTEND()


inline unsigned long long getTickMS()
{
#ifdef _WIN32
    return GetTickCount64();
#else
    // TODO: This has issues can mislead due to kernel (ex: printf throws it off)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts))
    {
        return 0;
    }
    return (ts.tv_sec * 1000) + ts.tv_nsec / 1000000L;
#endif
}

class YTimer
{
public:
    YTimer() :
        mLabel("task")
    {
        start();
    }
    YTimer(const char* label) :
        mLabel(label)
    {
        // NOTE: string passed in for label must stick around for lifetime of this object
        start();
    }
    void start()
    {
        mVal = getTickMS();
    }
    void stop()
    {
        mVal = getTickMS() - mVal;
    }
    void report(const char* color)
    {
        ytprint(color, "Time (ms) for ", mLabel, ": ");
        ytprint(mVal);
        ytprint(ESC_DEFAULT, "\n");
    }

private:
    unsigned long long mVal;
    const char* mLabel;
};

#undef TIME
#define TIME(name) \
    do                    \
    {                     \
        YTimer ti(name);  \
        do                \
        {

#undef TIMEEND
#define TIMEEND() \
        } while (false);  \
        ti.stop();        \
        ti.report(ESC_YELLOW);  \
    } while (false);


struct TestEntry
{
    const char* name;
    void (*func)();
};


#define FUNCDECL(NAME)     void NAME();
#define FUNCENTRY(NAME)   {#NAME, NAME},

#define DECLTESTS() \
    TESTLIST(FUNCDECL) \
    inline static TestEntry sTestEntries[] = \
    { \
        TESTLIST(FUNCENTRY) \
    };

inline int reportTestResults()
{
    ytprint("\n\n---------------------------------------------\n", ESC_GREEN, "Tests succeeded: ");
    ytprint(sTestsSucceeded);
    ytprint(ESC_RED, "\nTests failed:    ");
    ytprint(sTestsFailed);
    ytprint(ESC_DEFAULT, "\n");
    return sTestsFailed;
}


inline int runArgTests(int argc, char** argv, int entc, TestEntry* entv)
{
    if (argc == 1)
    {
        ytprint("Usage: Provide 1 or more tests from below or * to run all tests.\n");
        for (int j = 0; j < entc; j++)
        {
            ytprint("    ", entv[j].name, "\n");
        }
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        for (int j = 0; j < entc; j++)
        {
            if (strcasecmp(argv[i], "*") == 0 || strcasecmp(argv[i], entv[j].name) == 0)
            {
                ytprint(ESC_CYAN, "Running test function: ", entv[j].name, "\n");
                YTimer ti(entv[j].name);
                entv[j].func();
                ti.stop();
                ti.report(ESC_CYAN);
            }
        }
    }
    return reportTestResults();
}

#ifdef PRIMALC
#define RUNTESTS() runArgTests(sProcArgc, sProcArgv, countof(sTestEntries), sTestEntries)
#else
#define RUNTESTS(argc, argv) runArgTests(argc, argv, countof(sTestEntries), sTestEntries)
#endif

