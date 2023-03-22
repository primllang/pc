#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4521)
#pragma warning(disable: 4018)
#endif

#ifdef __CYGWIN__
// This define blocks some functions such as strptime() that are required
#undef __STRICT_ANSI__
#endif

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <math.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <sys/timeb.h>
#endif

#ifdef _MSC_VER
#include <time.h>
#endif

#include <cstdint>
#include <string>
#include <string_view>

// const helpers
#define constMemb static constexpr auto
#define constMemb_(x) static constexpr x
#define constDef constexpr auto
#define constDef_(x) constexpr x

// Delete copy (also move) and assignment
#define NOCOPY(CLASSNAME) \
    CLASSNAME (const CLASSNAME&) = delete; \
    CLASSNAME& operator= (const CLASSNAME&) = delete;

// String type helper
using strparam = std::string_view;

// Bit flag helpers
#define isFlagSet(value, flag)         ( ((value) & (flag)) != 0 )
#define isFlagClear(value, flag)       ( ((value) & (flag)) == 0 )
#define setFlag(value, flag)           { (value) |= (flag); }
#define clearFlag(value, flag)         { (value) &= ~(flag); }

#define countof(x)  ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// Windows specific
#ifdef _MSC_VER
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
#ifndef va_copy
    #define va_copy(dest, src) ((dest) = (src))
#endif
#endif

// Fundamental integer types (already defined: int char)
typedef unsigned char       byte;
typedef signed char         int8;
typedef short               int16;
typedef int                 int32;
typedef long long           int64;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;
typedef unsigned int        uint;

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

#ifdef _WIN32
    #define LINE_END "\r\n"
#else
    #define LINE_END "\n"
#endif

// Debug sensitive defines
#ifndef NDEBUG
     // We are debugging
#ifndef _DEBUG
    #define _DEBUG
#endif
    #define dbglog(fmt, ...) \
        do { fprintf(stderr, fmt, ## __VA_ARGS__); } while (0)

    #define dbgerr(fmt, ...) \
        do { fprintf(stderr, ESC_RED "Error: " fmt ESC_DEFAULT, ## __VA_ARGS__); } while (0)

#ifdef _MSC_VER
    #define dbgfnc() \
        do { fprintf(stderr, ESC_YELLOW "%s\n" ESC_DEFAULT, __FUNCSIG__); } while (0)
#else
    #define dbgfnc() \
        do { fprintf(stderr, ESC_YELLOW "%s\n" ESC_DEFAULT, __PRETTY_FUNCTION__); } while (0)
#endif

#else
    #undef _DEBUG

    #define dbglog(fmt, ...)

    #define dbgerr(fmt, ...) \
        do { fprintf(stderr, "Error: " fmt, ## __VA_ARGS__); } while (0)

    #define dbgfnc() ((void)0)
#endif

namespace base
{

enum
{
    GFLAG_VERBOSE_SHELL = 0x1,
    GFLAG_VERBOSE_FILESYS = 0x2
};
extern uint64 sBaseGFlags;

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
    explicit Buffer(size_t size) :
        mMemory(NULL),
        mSize(0)
    {
        alloc(size);
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
        free();
    }
    void* ptr()
    {
        return mMemory;
    }
    const void* cptr() const
    {
        return mMemory;
    }
    size_t size() const
    {
        return mSize;
    }
    bool empty() const
    {
        return mSize == 0;
    }

    void alloc(size_t size);
    void reAlloc(size_t size);
    void dblOr(size_t neededsize);
    void free();

    // Copies the memory from the provided buffer into this buffer
    void copyFrom(const Buffer& src);

    // Appends the memory from the provided buffer into this buffer
    void appendFrom(const Buffer& src);

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

    bool readFile(const std::string& filename, bool nullterm);
    bool writeFile(const std::string& filename) const
    {
        return writeFile(filename, mSize);
    }
    bool writeFile(const std::string& filename, size_t size) const;

    void dbgDump(const char* label)
    {
        dbglog("Buffer @ %s: mem:%p siz:%zu\n", label, mMemory, mSize);
    }

private:
    void* mMemory;
    size_t mSize;
};


// Iter class template is used to iterate over an array as follows:
//    for (Iter<Obj> i; arr.forEach(i); )
//    {
//        foo(i->field);
//    }
template <class T>
class Iter
{
public:
    Iter() :
        mPos(-1),
        mObj(nullptr),
        mKey(nullptr)
    {
    }
    T* operator->()
    {
        return mObj;
    }
    T& operator*()
    {
        return *mObj;
    }
    T* operator&()
    {
        return mObj;
    }
    int pos()
    {
        return mPos;
    }
    const char* key()
    {
        return mKey;
    }

public:
    int64 mPos;
    T* mObj;
    const char* mKey;
};


// FixedStr class template is used to declare a string object which has enough
// space to store a string of MAXFIXED size.  If the string being assigned is
// longer, heap memory is allocated.  Otherwise no memory is allocated.
template <int MAXFIXED>
class FixedStr
{
public:
    FixedStr()
    {
        mFixed[0] = '\0';
        mFixed[1] = '\0';
    }

    FixedStr(const FixedStr& src)
    {
        mFixed[0] = '\0';
        mFixed[1] = '\0';
        set(src.get());
    }

    FixedStr(FixedStr& src)
    {
        mFixed[0] = '\0';
        mFixed[1] = '\0';
        set(src.get());
    }
    ~FixedStr()
    {
        if (mFixed[0] == '\1')
        {
            if (mDyn.ptr)
            {
                free(mDyn.ptr);
            }
        }
    }
    FixedStr& operator=(const FixedStr& src)
    {
        set(src.get());
        return *this;
    }
    void set(const char* val)
    {
        set(val, strlen(val));
    }
    void set(const char* val, size_t len)
    {
        char* dest;

        if (len > (MAXFIXED - 2))
        {
            // Cannot fit in fixed
            if (isFixed())
            {
                mFixed[0] = '\1';

                mDyn.ptr = NULL;
                mDyn.size = 0;
            }
            ensureDyn(len + 1);
            dest = mDyn.ptr;
        }
        else
        {
            // Can fit in fixed
            if (isFixed())
            {
                dest = &(mFixed[1]);
            }
            else
            {
                ensureDyn(len + 1);
                dest = mDyn.ptr;
            }
        }
        memcpy(dest, val, len);
        dest[len] = '\0';
    }

    // Get the string pointer fixed or dynamic.
    const char* get() const
    {
        if (isFixed())
        {
            return &(mFixed[1]);
        }
        else
        {
            return mDyn.ptr;
        }
    }

    void clear()
    {
        if (mFixed[0] == '\1')
        {
            if (mDyn.ptr)
            {
                free(mDyn.ptr);
            }
        }
        mFixed[0] = '\0';
        mFixed[1] = '\0';
    }

private:
    union
    {
        struct
        {
            int tag;
            char* ptr;
            int size;
        } mDyn;
        char mFixed[MAXFIXED];
    };
    bool isFixed() const
    {
        return mFixed[0] == '\0';
    }
    void ensureDyn(int size)
    {
        if (mDyn.ptr != NULL && mDyn.size >= size)
        {
            return;
        }

        void* p = ::realloc(mDyn.ptr, size);
        if (p == NULL)
        {
            dbgerr("FixedStr failed to allocate %d bytes\n", size);
            return;
        }
        mDyn.ptr = (char*)p;
        mDyn.size = size;
   }
};

// Class used to effiently build a string
class StrBld
{
public:
    StrBld(const char* str) :
        mLen(0)
    {
        size_t l = strlen(str);
        mBuf.alloc(l + 1);
        append(str, l);
    }
    StrBld() :
        mLen(0)
    {
        mBuf.alloc(64);
    }
    StrBld(size_t minsize) :
        mLen(0)
    {
        mBuf.alloc(minsize);
    }
    StrBld(const StrBld& src) :
        mBuf(src.mBuf),
        mLen(src.mLen)
    {
    }
    StrBld(StrBld& src) :
        mBuf(src.mBuf),
        mLen(src.mLen)
    {
    }
    void appendc(char c)
    {
        char* buf = ensureAlloc(mLen + 2);
        buf[mLen++] = c;
    }
    void append(size_t count, char c);
    void append(const char* s, size_t l);
    void append(const char* s)
    {
        append(s, strlen(s));
    }
    void append(std::string_view s)
    {
        append(s.data(), s.length());
    }
    // void append(const std::string& s)
    // {
    //     append(s.c_str(), s.length());
    // }
    void append(StrBld& sb)
    {
        append(sb.c_str(), sb.length());
    }
    void replaceLast(char c)
    {
        char* buf = ensureAlloc(mLen + 1);
        if (mLen > 0)
        {
            buf[mLen - 1] = c;
        }
    }
    void eraseLast()
    {
        if (mLen > 0)
        {
            mLen--;
        }
    }
    char last()
    {
        char* buf = (char*)mBuf.ptr();
        return mLen > 0 ? buf[mLen - 1] : '\0';
    }
    char first()
    {
        char* buf = (char*)mBuf.ptr();
        return mLen > 0 ? buf[0] : '\0';
    }
    void clear()
    {
        mLen = 0;
    }
    const char* c_str()
    {
        char* buf = (char*)mBuf.ptr();
        if (buf)
        {
            // Lazy null termination
            buf[mLen] = '\0';
        }
        return buf;
    }
    std::string toString()
    {
        char* buf = (char*)mBuf.ptr();
        return std::string(buf, mLen);
    }
    size_t length()
    {
        return mLen;
    }
    bool empty()
    {
        return mLen == 0;
    }

    bool equals(const char* s)
    {
        const char* cp = c_str();
        if (cp)
        {
            return strcmp(cp, s) == 0;
        }
        else
        {
            return cp != s;
        }
    }
    bool iequals(const char* s);
    bool equals(char c)
    {
        return (mLen == 1 && *(char*)mBuf.ptr() == c);
    }
    char operator[](size_t i)
    {
        char* buf = (char*)mBuf.ptr();
        return (i < mLen) ? buf[i] : '\0';
    }
    bool existCh(char c)
    {
        return (strchr(c_str(), c) != NULL);
    }

    bool appendVFmt(const char* fmt, va_list varg);
    bool appendFmt(const char* fmt, ...);

    // Strip quotes from the current token string if found
    //   allowsingle If true, also allows stripping single quotes
    void stripQuotes(bool allowsingle);
    void copyToStr(std::string& s)
    {
        s.assign((char*)mBuf.ptr(), mLen);
    }
    void assignSubstr(const char* str, size_t len)
    {
        // Doesn't need to be zero terminated
        char* buf = ensureAlloc(len + 1);
        memcpy(buf, str, len);
        mLen = len;
    }
    void moveFrom(StrBld& sb)
    {
        mBuf.moveFrom(sb.mBuf);
        mLen = sb.mLen;
        sb.mLen = 0;
    }
    void copyFrom(const StrBld& sb)
    {
        mBuf.copyFrom(sb.mBuf);
        mLen = sb.mLen;
    }
    void copyToBuffer(base::Buffer& buf);
    void moveToBuffer(base::Buffer& buf);

private:
    char* ensureAlloc(size_t neededsize);

protected:
    base::Buffer mBuf;
    size_t mLen;
};

// Parser class implements a text parser which follows simple rules to build tokens.

constexpr auto PUNC_CHARS = "&!|/:;=+*-.$@^%?`,\\";
constexpr auto BRAC_CHARS = "<([{}])>";
constexpr auto ESCAPE_CODES = "nrtbf\\\"/";
constexpr auto ESCAPE_CHARS = "\n\r\t\b\f\\\"/";

class Parser
{
public:
    Parser() :
        mToken(128)
    {
        clearState();
    }

    void initParser(const char* txt, size_t len);

	// Enables or disables single punctuation mode.  In single punc mode, a token
    // consists of only a single char.  When this mode is disabled (default), multiple
    // adjacent chars are combined into a single token.
    void enableSinglePunc(bool enable)
    {
        mSinglePunc = enable;
    }
    // Disabled by default.  When enabled, automatically skips # comments till EOL
    void enableHashComment(bool enable)
    {
        mSkipHashComment = enable;
    }
    // returns true if at the end of text has reached
    bool eof()
    {
        return (mPos >= mTxtLen) || mErr;
    }
    // returns true if there was an error, false otherwise
    bool failed()
    {
        return mErr;
    }
    const std::string& errMsg()
    {
        return mErrMsg;
    }
    // Returns the current token
    const char* tokenStr()
    {
        parseToken();
        return mToken.c_str();
    }
    StrBld& token()
    {
        parseToken();
        return mToken;
    }
    size_t tokenLen()
    {
        parseToken();
        return mToken.length();
    }
    void tokenPtrLen(const char** ptr, size_t* len)
    {
        size_t l = tokenLen();
        *len = l;
        *ptr = mTxt + (mPos - l);
    }
    void tokenEOLPtrLen(const char** ptr, size_t* len);
    void tokenSubstrPtrLen(const char** ptr, size_t* len, int startpos, int endpos);

    // Compares the current token with a string
    bool tokenEquals(const char* val)
    {
        parseToken();
        return mToken.equals(val);
    }
    bool tokenEquals(char c)
    {
        parseToken();
        return mToken.equals(c);
    }
    char tokenFirst()
    {
        parseToken();
        return mToken.first();
    }
    int tokenColumn()
    {
        // Returns starting column in the line (zero based)
        parseToken();
        int col = mTokStartPos - mLineStartPos;
        return col < 0 ? 0 : col;
    }
    int tokenLine()
    {
        parseToken();
        return lineNum();
    }

    std::string tokLineSoFarStr();
    std::string tokGetFullLine();
    std::string tokGetTillEOL();
    bool tokPeekInLine(const char* tok, bool stayiffound);
    int tokEndPos()
    {
        return mTokEndPos;
    }
    int tokStartPos()
    {
        return mTokStartPos;
    }
    char tokNextChar()
    {
        if (!eof())
        {
            return mTxt[mPos];
        }
        else
        {
            return '\0';
        }
    }
    bool tokInclude(char c)
    {
        if (tokNextChar() == c)
        {
            append(c);
            mPos++;
            mTokEndPos = mPos;
            return true;
        }
        return false;
    }

    int getIndent();

    // Advances to the next token by parsing it
    void advance()
    {
        mTokParsed = false;
    }
    void advance(const char* match)
    {
        if (mErr)
        {
            return;
        }
        if (tokenEquals(match))
        {
            advance();
        }
        else
        {
            expectErr(0, match);
        }
    }
    void advance(char matchc)
    {
        if (mErr)
        {
            return;
        }

        if (tokenEquals(matchc))
        {
            advance();
        }
        else
        {
            expectErr(matchc, NULL);
        }
    }
    // Advance to next token.. return false if moved to next line
    bool advanceSameLine();
    int findEol();
    void setError(const char* msg, bool fmt = true);

    void copySubstr(StrBld& bld, int startpos, int endpos);

protected:
    struct InternalState
    {
        int pos;
        int tokStartPos;
        int tokEndPos;
        int lineStartPos;
        bool parsed;
        int linenum;
        int lineIncPos;
        std::string tok;
    };

    void saveState(InternalState& chk);
    void restoreState(InternalState& chk);
    void movePos(int delta);

    const char* getText()
    {
        return mTxt;
    }
    size_t getTextLen()
    {
        return (size_t)mTxtLen;
    }
    int lineNum()
    {
        return mLineNum;
    }
    bool charWord(char c)
    {
        return (isalnum(c) || (c == '_') || (c == '\''));
    }
    bool charPunc(char c)
    {
        return (strchr(PUNC_CHARS, c) != NULL);
    }
    bool charBrac(char c)
    {
        return (strchr(BRAC_CHARS, c) != NULL);
    }

protected:
    bool mErr;
    std::string mErrMsg;

private:
    enum TokenStateEnum
    {
        NullTok, WordTok, PuncTok, BracTok, QuoteTok, HashComment
    };

    const char* mTxt;
    StrBld mToken;
    bool mTokParsed;
    int mPos;
    int mTxtLen;
    int mLineNum;
    int mTokStartPos;
    int mTokEndPos;
    int mLineStartPos;
    bool mSinglePunc;
    bool mSkipHashComment;
    int mLineIncPos;

    void clearState();

    void newLine(int delta)
    {
        //TODO: Line numbers are completely broken
        if (mPos != mLineIncPos)
        {
            mLineNum++;
            mLineIncPos = mPos;
        }
        mLineStartPos = mPos + delta;
    }

    void parseToken()
    {
        if (mErr)
        {
            mToken.clear();
            return;
        }
        if (mTokParsed)
        {
            return;
        }
        (void)internalParse(false);
    }

    bool internalParse(bool exitonnewline);
    void expectErr(char c, const char* str);

    void append(char c)
    {
        mToken.appendc(c);
    }
    void append(const std::string& s)
    {
        mToken.append(s);
    }
    void replaceLast(char c)
    {
        mToken.replaceLast(c);
    }
    void eraseLast()
    {
        mToken.eraseLast();
    }
    TokenStateEnum detState(char c)
    {
        if (c == '"' || c == '\'')
        {
            return QuoteTok;
        }
        if (charWord(c))
        {
            return WordTok;
        }
        if (charBrac(c))
        {
            return BracTok;
        }
        if (c == '#' && mSkipHashComment)
        {
            return HashComment;
        }

        return PuncTok;
    }

    TokenStateEnum tokType();
};

// Internal class. Do not use directly.
class ListP
{
public:
    ListP() = delete;
    ListP(size_t objsize) :
        mFront(nullptr),
        mBack(nullptr),
        mObjSize(objsize)
    {
    }
    ~ListP()
    {
        clearP();
    }

    // Returns a pointer to the newly allocated node memory at head where data can be stored
    void* allocFront();

    // Returns a pointer to the newly allocated node memory at tail where data can be stored
    void* allocBack();

    // Returns a pointer to the newly allocated node memory which is unlinked
    // it must either be added to the list using linkBack() or freed using freeUnlinked()
    // Not doing either will cause memory leak. freeUnlinked doesn't call the destructor.
    void* allocUnlinked()
    {
        Node* newnode = allocNode();
        return newnode->data();
    }
    void linkBack(void* data);
    void freeUnlinked(void* data);

    // Returns a pointer to the node data at the front (or nullptr)
    void* front()
    {
        return mFront ? mFront->data() : nullptr;
    }

    // Returns a pointer to the node data at the back (or nullptr)
    void* back()
    {
        return mBack ? mBack->data() : nullptr;
    }
    bool empty()
    {
        return mFront == nullptr && mBack == nullptr;
    }

    // The data parameter must be a node from list
    // If not, memory will be corrupted
    static void* next(void* data);

    // The data parameter must be a node from list
    // If not, memory will be corrupted
    static void* prev(void* data);

protected:
    class Node
    {
    public:
        Node() = delete;
        ~Node() = delete;
        void init()
        {
            mNext = nullptr;
            mPrev = nullptr;
        }
        void* data()
        {
            return (void*)((unsigned char*)this + sizeof(Node));
        }
        static Node* nodeFromData(void* data)
        {
            return (Node*)((unsigned char*)data - sizeof(Node));
        }
    public:
        Node* mNext;
        Node* mPrev;
        // Data is stored after the two pointers above
    };


protected:
    Node* mFront;
    Node* mBack;
    size_t mObjSize;

    Node* allocNode()
    {
        Node* node = (Node*)malloc(sizeof(Node) + mObjSize);
        node->init();
        return node;
    }
    void freeNode(Node* node)
    {
        free(node);
    }
    // Remove all items
    void clearP();

    // Remove the given data node which must be a node from list
    // If not, memory will be corrupted
    bool removeP(void* data);

    // Unlink the node
    void unlinkP(Node* node);

};


template <class T>
class List : public ListP
{
public:
    List() :
        ListP(sizeof(T))
    {
    }
    NOCOPY(List);
    ~List()
    {
        clear();
    }
    void addFront(const T& elem)
    {
        new (allocFront()) T(elem);
    }
    void addBack(const T& elem)
    {
        new (allocBack()) T(elem);
    }
    bool findAndRemove(const T& elem)
    {
        void* data = front();
        while (data)
        {
            if (elem == *(T*)data)
            {
                break;
            }
            data = next(data);
        }
        if (data)
        {
            T* obj = (T*)data;
            obj->~T();
            removeP(data);
            return true;
        }
        return false;
    }
    void clear()
    {
        Node* n = mFront;
        while (n)
        {
            Node* tmp = n;
            n = n->mNext;
            T* obj = (T*)tmp->data();
            obj->~T();
            freeNode(tmp);
        }
        mFront = nullptr;
        mBack = nullptr;
    }
    bool removeFront()
    {
        T* obj = (T*)front();
        if (obj)
        {
            obj->~T();
        }
        return ListP::removeP(front());
    }
    bool removeBack()
    {
        T* obj = (T*)back();
        if (obj)
        {
            obj->~T();
        }
        return ListP::removeP(back());
    }
    void deleteUnlinked(T* obj)
    {
        if (obj)
        {
            obj->~T();
        }
        freeUnlinked(obj);
    }
    bool forEachRev(base::Iter<T>& Iter)
    {
        if (Iter.mPos == -1)
        {
            Iter.mObj = (T*)back();
            Iter.mPos++;
        }
        else
        {
            Iter.mObj = (T*)prev(Iter.mObj);
            Iter.mPos++;
        }
        return Iter.mObj != nullptr;
    }
    bool forEach(base::Iter<T>& Iter)
    {
        if (Iter.mPos == -1)
        {
            Iter.mObj = (T*)front();
            Iter.mPos++;
        }
        else
        {
            Iter.mObj = (T*)next(Iter.mObj);
            Iter.mPos++;
        }
        return Iter.mObj != nullptr;
    }

};

class Splitter
{
public:
    Splitter() = delete;
    Splitter(const char* str, size_t len, char delim);
    Splitter(const char* str, char delim) :
        Splitter(str, strlen(str), delim)
    {
    }
    Splitter(const base::Buffer& buf, char delim) :
        Splitter((const char*)buf.cptr(), buf.size(), delim)
    {
    }
    Splitter(strparam strp, char delim) :
        Splitter(strp.data(), strp.length(), delim)
    {
    }
    void ignore(char c)
    {
        mIgnore = c;
    }
    bool eof()
    {
        return mPos >= mLen;
    }
    const char* get();
    const char* get(size_t* len)
    {
        const char* p = get();
        *len = mCur.length();
        return p;
    }
    std::string getStr()
    {
        size_t l;
        const char* s = get(&l);
        return std::string(s, l);
    }

private:
    const char* mTxt;
    size_t mLen;
    size_t mPos;
    base::StrBld mCur;
    char mDelim;
    char mIgnore;
};


#define FACILITY_PRIMAL     1776
#define FACILITY_ERRNO      3001
#define FACILITY_WIN32      7

union errort
{
    uint32 errorcode;
    struct
    {
        uint reserved : 4;
        uint facility : 12;
        uint number : 16;
    };
    errort() :
        errorcode(0)
    {
    }
    constexpr errort(uint errfacility, uint errnumber) :
        reserved(0),
        facility(errfacility),
        number(errnumber)
    {
    }
    operator bool() const
    {
        return errorcode != 0;
    }
    std::string message();
};

// Primal errors
constexpr errort errorNone = errort(0, 0);
constexpr errort errorNotFound = errort(FACILITY_PRIMAL, 2);

// Enum and string map support
template<typename T>
class EnumStringMap
{
public:
    constexpr EnumStringMap(uint count, const char** arr, uint skipchars) :
        mStrArr(arr),
        mStrCnt(count),
        mSkip(skipchars)
    {
    }
    bool exists(strparam str) const
    {
        return fromString(str, nullptr);
    }
    uint count() const
    {
        return mStrCnt;
    }
    bool fromString(strparam str, T* ret) const
    {
        for (uint i = 0; i < mStrCnt; i++)
        {
            if (str.compare(mStrArr[i] + mSkip) == 0)
            {
                if (ret)
                {
                    *ret = (T)i;
                }
                return true;
            }
        }
        return false;
    }
    const char* toString(T v) const
    {
        uint i = (uint)v;
        return getStr(i, mSkip);
    }
    const char* toStringI(uint index) const
    {
        return getStr(index, mSkip);
    }

private:
    const char** mStrArr;
    uint mStrCnt;
    uint mSkip;

    const char* getStr(uint index, uint skip) const
    {
        if (index < mStrCnt)
        {
            return mStrArr[index] + skip;
        }
        return nullptr;
    }
};

#define ENUM_ENTRY(name) name,
#define ENUM_STRENTRY(name) #name,

#define enummapdef(enumlist, enumtype, mapname, skip) \
    enum class enumtype : uint32 \
    { \
        enumlist(ENUM_ENTRY) \
    }; \
    inline const char* enumtype##Strings[] = \
    { \
        enumlist(ENUM_STRENTRY) \
    }; \
    constexpr base::EnumStringMap<enumtype> mapname(countof(enumtype##Strings), enumtype##Strings, skip)

#define strmapdef(strarray, mapname) \
    constexpr base::EnumStringMap<uint> mapname(countof(strarray), strarray, 0)

// Flags

#define FLAGS32_ENTRY(flagname, bitno) flagname = 1ul << bitno,
#define FLAGS64_ENTRY(flagname, bitno) flagname = 1ull << bitno,

#define flagsdef32(flagslist, flagstypename) \
class flagstypename   \
{ \
    uint32 mVal = 0; \
public: \
    enum etype : uint32 \
    { \
        flagslist(FLAGS32_ENTRY) \
    }; \
    uint32 value() { return mVal; }  \
    void setValue(uint32 val) { mVal = val; }  \
    bool isSet(etype flag) { return (mVal & flag) != 0; } \
    bool isClear(etype flag) { return (mVal & flag) == 0; } \
    bool isBitNoSet(uint32 bitno) { return (mVal & (1ul << bitno)) != 0;} \
    void set(etype flag) { mVal |= flag; } \
    void clear(etype flag) { mVal &= ~flag; } \
};

#define flagsdef64(flagslist, flagstypename) \
class flagstypename   \
{ \
    uint64 mVal = 0; \
public: \
    enum etype : uint64 \
    { \
        flagslist(FLAGS64_ENTRY) \
    }; \
    uint64 value() { return mVal; }  \
    void setValue(uint64 val) { mVal = val; }  \
    bool isSet(etype flag) { return (mVal & flag) != 0; } \
    bool isClear(etype flag) { return (mVal & flag) == 0; } \
    bool isBitNoSet(uint64 bitno) { return (mVal & (1ull << bitno)) != 0;} \
    void set(etype flag) { mVal |= flag; } \
    void clear(etype flag) { mVal &= ~flag; } \
};

int randInt();

// Formats a string using printf style parameters
std::string formatr(const char* fmt, ...);
void format(std::string& outstr, const char* fmt, ...);
bool vformat(std::string& outstr, const char* fmt, va_list varg);

// String
void upperCase(std::string& str);
void lowerCase(std::string& str);
void trimLeft(std::string& str);
void trimRight(std::string& str);
void padRight(std::string& str, size_t width);
uint strHash(strparam msg);

// Compares strings ('i' means case-insensitive)
inline bool strieql(strparam a, strparam b)
{
    return strcasecmp(a.data(), b.data()) == 0;
}
inline bool streql(strparam a, strparam b)
{
    return strcmp(a.data(), b.data()) == 0;
}
int64_t str2baseint(strparam str, int base, bool* valid = NULL);
inline int64_t str2int(strparam str, bool* valid = NULL)
{
    return str2baseint(str, 10, valid);
}
double str2dbl(strparam str, bool* valid = NULL);

// Finds the index of a char in a string
//   s   String to search in
//   c   Char to search
//   pos If not null and match found, returns the position of the match
// returns True if found, false otherwise
bool strfind(const char* s, char c, size_t* pos = NULL);

} // base


