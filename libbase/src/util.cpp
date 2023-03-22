#include "util.h"
#include "sys.h"
#include <sys/stat.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <dirent.h>
#endif

using namespace std;

namespace base
{

uint64 sBaseGFlags = 0;

// Buffer::

void Buffer::alloc(size_t size)
{
    free();
    mMemory = ::malloc(size);
    if (mMemory == NULL)
    {
        dbgerr("failed to allocate %zu bytes\n", size);
        return;
    }
    mSize = size;
    //dbgDump("Alloc");
}

void Buffer::reAlloc(size_t size)
{
    if (size == 0)
    {
        free();
        return;
    }
    void* p = ::realloc(mMemory, size);
    if (p == NULL)
    {
        dbgerr("failed to allocate %zu bytes\n", size);
        return;
    }

    mMemory = p;
    mSize = size;
    //dbgDump("Realloc");
}

void Buffer::dblOr(size_t neededsize)
{
    size_t newsize = mSize;

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
    reAlloc(newsize);
}

void Buffer::free()
{
    if (mMemory != NULL)
    {
        //dbgDump("Free");
        ::free(mMemory);
        mMemory = NULL;
        mSize = 0;
    }
}

void Buffer::copyFrom(const Buffer& src)
{
    reAlloc(src.size());
    memcpy(ptr(), src.cptr(), src.size());
}

void Buffer::appendFrom(const Buffer& src)
{
    size_t oldsize = size();
    reAlloc(oldsize + src.size());
    memcpy(((unsigned char*)ptr()) + oldsize, src.cptr(), src.size());
}


void Buffer::moveFrom(Buffer& src)
{
    free();
    mMemory = src.mMemory;
    mSize = src.mSize;
    src.mMemory = NULL;
    src.mSize = 0;
}

bool Buffer::readFile(const std::string& filename, bool nullterm)
{
    // Read a file into memory buffer. If asked, a zero is appended at the end
    // so the buffer can be used as a null terminated string.

    bool ret = false;
    struct stat st;
    if (stat(filename.data(), &st) == 0)
    {
        FILE* f = fopen(filename.c_str(), "rb");

        reAlloc(st.st_size + (nullterm ? 1 : 0));
        char* buf = (char*)ptr();

        if (buf != NULL)
        {
            if (fread(buf, 1, st.st_size, f) == (size_t)st.st_size)
            {
                if (nullterm)
                {
                    buf[st.st_size] = '\0';
                }
                ret = true;
            }
        }
        fclose(f);
    }
    if (!ret)
    {
        // Failed to read, free the buffer.
        free();
        dbgerr("Failed to read file '%s' into buffer\n", filename.c_str());
    }
    return ret;
}

bool Buffer::writeFile(const std::string& filename, size_t size) const
{
    //dbgDump(filename.c_str());
    if (!mMemory)
    {
        // writing an empty buffer is not considered an error (is a noop)
        return true;
    }
    bool ret = false;
    FILE* f = fopen(filename.c_str(), "wb");
    if (f != NULL)
    {
        if (fwrite(mMemory, 1, size, f) == size)
        {
            ret = true;
        }
        fclose(f);
    }
    if (!ret)
    {
        perror("");
        dbgerr("Failed to write buffer to '%s'\n", filename.c_str());
    }
    return ret;
}


// StrBld::
char* StrBld::ensureAlloc(size_t neededsize)
{
    if (neededsize > mBuf.size())
    {
        mBuf.dblOr(neededsize);
    }
    return (char*)mBuf.ptr();
}

void StrBld::copyToBuffer(base::Buffer& buf)
{
    // If this buffer is empty, dest buf is made emptied by freeing what it has
    if (mLen == 0)
    {
        buf.free();
    }
    else
    {
        buf.reAlloc(mLen);
        memcpy(buf.ptr(), mBuf.ptr(), mLen);
    }
}

void StrBld::append(size_t count, char c)
{
    char* buf = ensureAlloc(mLen + count + 1);
    for (size_t i = 0; i < count; i++)
    {
        buf[mLen++] = c;
    }
}

void StrBld::append(const char* s, size_t l)
{
    char* buf = ensureAlloc(mLen + l + 1);
    memcpy(buf + mLen, s, l);
    mLen += (int)l;
}

void StrBld::moveToBuffer(base::Buffer& buf)
{
    // If this buffer is empty, dest buf is made emptied by freeing what it has
    if (mLen == 0)
    {
        buf.free();
    }
    else
    {
        mBuf.reAlloc(mLen);
        buf.moveFrom(mBuf);
        mLen = 0;
    }
}

bool StrBld::appendFmt(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    bool ret = appendVFmt(fmt, va);
    va_end(va);
    return ret;
}

bool StrBld::iequals(const char* s)
{
    return base::strieql(c_str(), s);
}

bool StrBld::appendVFmt(const char* fmt, va_list varg)
{
    const int stackbufsize = 256;
    char stackbuf[stackbufsize];
    int count;
    va_list orgvarg;

    va_copy(orgvarg, varg);

    // Try to format using the small stack buffer.
    count = vsnprintf(stackbuf, stackbufsize, fmt, varg);
    if (count < 0)
    {
        // Negative number means formatting error.
        return false;
    }

    if (count < stackbufsize)
    {
        // Non-negative and less than the given size so string was completely written.
        append(stackbuf);
    }
    else
    {
        // Format the string with a sufficiently large heap buffer.
        char* buf = ensureAlloc(mLen + count + 1);
        buf += mLen;
        int fincount = vsnprintf((char*)buf, count + 1, fmt, orgvarg);
        if (fincount < 0)
        {
            return false;
        }
        mLen += fincount;
    }
    return true;
}

void StrBld::stripQuotes(bool allowsingle)
{
    char c1;
    char c2;

    if (mLen >= 2)
    {
        char* buf = (char*)mBuf.ptr();

        c1 = buf[0];
        c2 = buf[mLen - 1];

        if ((c1 == '"' && c2 == '"') ||
            (allowsingle && c1 == '\'' && c2 == '\'') )
        {
            mLen--;

            if (mLen >= 1)
            {
                memmove(buf, buf + 1, mLen);
                mLen--;
            }
        }
    }
}

// Parser::

void Parser::initParser(const char* txt, size_t len)
{
    clearState();
    mTxt = txt;
    if (txt)
    {
        mTxtLen = (int)len;
        newLine(0);
    }
    else
    {
        setError("NULL string");
    }
}

void Parser::clearState()
{
    mToken.clear();
    mTokParsed = false;
    mPos = 0;
    mLineNum = 1;
    mErr = false;
    mTokStartPos = 0;
    mTokEndPos = 0;
    mLineStartPos = 0;
    mSinglePunc = false;
    mSkipHashComment = false;
    mLineIncPos = 0;
}

bool Parser::internalParse(bool exitonnewline)
{
    TokenStateEnum state = NullTok;
    bool done = false;
    char lastc = '\0';
    char quotec = '\0';
    bool exitednl = false;

    mToken.clear();
    while (!eof())
    {
        char c = mTxt[mPos];

        // Skip white space if not inside quotes.  It will also finish a token.

        if ((state != QuoteTok) && (state != HashComment) && isspace(c))
        {
            if (state != NullTok)
            {
                done = true;
            }
            else if (c == '\n')
            {
                newLine(1);
                if (exitonnewline)
                {
                    done = true;
                    exitednl = true;
                }
            }
        }
        else
        {
            // If a state has not been established, determine which state
            // to get into, otherwise read the rest of the token

            if (state == NullTok)
            {
                state = detState(c);
                if (state == QuoteTok)
                {
                    quotec = c;
                }

                mTokStartPos = mPos;

                append(c);
            }
            else
            {
                switch (state)
                {
                    case QuoteTok:
                    {
                        if (lastc == '\\')
                        {
                            // Escape char.
                            char escch = c;
                            size_t pos;
                            if (strfind(ESCAPE_CODES, escch, &pos))
                            {
                                escch = ESCAPE_CHARS[pos];

                                replaceLast(escch);
                                c = '\0';
                            }
                            else
                            {
                                // If the failure was due to a json hex code, read it now.
                                eraseLast();
                                if (c == 'u')
                                {
                                    string hex;
                                    for (int h = 0; h < 4; h++)
                                    {
                                        if (!eof())
                                        {
                                            mPos++;
                                            hex += mTxt[mPos];
                                        }
                                    }

                                    // If found 4 hex chars, convert to int, and the UTF8
                                    if (hex.length() == 4)
                                    {
                                        bool valid;
                                        uint32_t cp = (uint32_t)str2baseint(hex, 16, &valid);
                                        if (valid)
                                        {
                                            append(makeUTF8(cp));
                                        }
                                    }
                                }
                                else
                                {
                                    //TODO: Handle failure
                                    //dbglog ("Illegal escape char %c\n", c);
                                    //setError("Illegal escape char");
                                    //done = true;
                                }
                            }
                        }
                        else
                        {
                            if (c != quotec)
                            {
                                append(c);
                            }
                            else
                            {
                                append(c);
                                if (!eof())
                                {
                                    lastc = c;
                                    mPos++;
                                }
                                done = true;
                            }
                        }
                    }
                    break;

                    case WordTok:
                    {
                        if (charWord(c))
                        {
                            append(c);
                        }
                        else
                        {
                            done = true;
                        }
                    }
                    break;

                    case BracTok:
                    {
                        done = true;
                    }
                    break;

                    case PuncTok:
                    {
                        bool besingle = (lastc == ':') || (lastc == ',');
                        if (!mSinglePunc && !besingle && charPunc(c))
                        {
                            append(c);
                        }
                        else
                        {
                            done = true;
                        }
                    }
                    break;

                    case HashComment:
                    {
                        eraseLast();
                        mPos = findEol();
                        newLine(1);
                        state = NullTok;
                        if (exitonnewline)
                        {
                            done = true;
                            exitednl = true;
                        }
                    }
                    break;

                    default:
                    // TODO: what should happen here.
                    break;
                }
            }
        }

        if (done)
        {
            break;
        }
        lastc = c;
        mPos++;
    }

    mTokEndPos = mPos;
    mTokParsed = true;

    return exitednl;
}

bool Parser::advanceSameLine()
{
    // Advance to next token.. return false if moved to next line

    mTokParsed = false;
    if (mErr)
    {
        mToken.clear();
        return false;
    }
    return internalParse(true);
}


int Parser::findEol()
{
    int eol = mLineStartPos;
    while (eol < mTxtLen && mTxt[eol] != '\n')
    {
        eol++;
    }
    return eol;
}

// Returns true if the 'tok' exists as a token in the current line
// Position is not changed unless found and stayiffound is true.
bool Parser::tokPeekInLine(const char* tok, bool stayiffound)
{
    // TODO: This code is very hacky. Redo.

    bool found = tokenEquals(tok);
    if (found)
    {
        return found;
    }

    if (tokenEquals(""))
    {
        advance();
    }

    InternalState chk;
    saveState(chk);
    while (!eof())
    {
        if (tokenEquals(tok))
        {
            found = true;
            break;
        }
        if (advanceSameLine())
        {
            break;
        }
    }
    if (tokenEquals(tok))
    {
        found = true;
    }

    if (stayiffound)
    {
        if (!found)
        {
            restoreState(chk);
        }
    }
    else
    {
        restoreState(chk);
    }
    return found;
}

void Parser::setError(const char* msg, bool fmt)
{
    // TODO: Avoid multiple formatting in this function
    if (!mErr)
    {
        if (fmt)
        {
            format(mErrMsg, "Parser error: %s at line %d\n", msg, mLineNum);
        }
        else
        {
            mErrMsg.assign(msg);
        }
        mErr = true;
    }
}

std::string Parser::tokLineSoFarStr()
{
    std::string s;
    int l = mTokStartPos - mLineStartPos;
    if (mLineStartPos + l <= mTxtLen)
    {
        s = std::string(mTxt + mLineStartPos, l);
    }
    mTokParsed = false;
    return s;
}

std::string Parser::tokGetFullLine()
{
    // Returns the entire line--hash comments are not excluded
    // after the call current pos is pointing at next line

    int eol = findEol();
    std::string s;
    if (eol > mLineStartPos)
    {
        s = std::string(mTxt + mLineStartPos, eol - mLineStartPos - 1);
    }

    mPos = eol;
    newLine(1);
    mTokParsed = false;
    return s;
}

std::string Parser::tokGetTillEOL()
{
    // TODO: Remove this function
    // Returns the line from start of current token, until EOL, hash comments are excluded
    // after the call current pos is pointing at next line
    std::string s;
    int l = 0;
    mPos = mTokStartPos;
    bool commentfound = false;
    while (!eof())
    {
        if (mTxt[mPos] == '\n')
        {
            mPos++;
            break;
        }
        else if (mSkipHashComment && !commentfound && mTxt[mPos] == '#')
        {
            commentfound = true;
        }

        if (!commentfound && mTxt[mPos] != '\r')
        {
            l++;
        }
        mPos++;
    }
    while (l > 1 && (mTxt[mTokStartPos + l - 1] == '\r' || isspace(mTxt[mTokStartPos + l - 1])))
    {
        l--;
    }
    if (mTokStartPos + l <= mTxtLen)
    {
        s = std::string(mTxt + mTokStartPos, l);
    }
    newLine(0);
    mTokParsed = false;
    return s;
}

void Parser::copySubstr(StrBld& bld, int startpos, int endpos)
{
    if (endpos > startpos)
    {
        int l = endpos - startpos;
        if ((startpos + l) <= mTxtLen)
        {
            bld.assignSubstr(mTxt + startpos, l);
        }
    }
}

void Parser::tokenSubstrPtrLen(const char** ptr, size_t* len, int startpos, int endpos)
{
    if (endpos > startpos)
    {
        int l = endpos - startpos;
        if ((startpos + l) <= mTxtLen)
        {
            *len = l;
            *ptr = mTxt + startpos;
        }
    }
}

void Parser::tokenEOLPtrLen(const char** ptr, size_t* len)
{
    // TODO: Similar to tokGetTillEOL() which should call this, and verify functionality
    // Skips the line from start of current token, until EOL, hash comments are excluded
    // after the call current pos is pointing at next line
    int l = 0;
    mPos = mTokStartPos;
    bool commentfound = false;
    while (!eof())
    {
        if (mTxt[mPos] == '\n')
        {
            mPos++;
            break;
        }
        else if (mSkipHashComment && !commentfound && mTxt[mPos] == '#')
        {
            commentfound = true;
        }

        if (!commentfound && mTxt[mPos] != '\r')
        {
            l++;
        }
        mPos++;
    }
    while (l > 1 && (mTxt[mTokStartPos + l - 1] == '\r' || isspace(mTxt[mTokStartPos + l - 1])))
    {
        l--;
    }
    if (mTokStartPos + l <= mTxtLen)
    {
        *len = l;
        *ptr = mTxt + mTokStartPos;
    }
    newLine(0);
    mTokParsed = false;
}


int Parser::getIndent()
{
    // Returns the indent level of currentline
    int i = mLineStartPos;
    while (i < mTxtLen && isspace(mTxt[i]))
    {
        i++;
    }
    return i - mLineStartPos;
}

void Parser::expectErr(char c, const char* str)
{
    char tmp[2];
    std::string s;
    parseToken();
    if (c)
    {
        tmp[0] = c;
        tmp[1] = '\0';
        str = tmp;
    }
    format(s, "Expecting [%s] but found [%s] ", str, tokenStr());
    setError(s.c_str());
}

Parser::TokenStateEnum Parser::tokType()
{
    parseToken();

    char c = mToken[0];
    if (c != '\0')
    {
        return detState(c);
    }
    return NullTok;
}

void Parser::saveState(InternalState& chk)
{
    chk.pos = mPos;
    chk.tokStartPos = mTokStartPos;
    chk.tokEndPos = mTokEndPos;
    chk.lineStartPos = mLineStartPos;
    chk.parsed = mTokParsed;
    chk.linenum = mLineNum;
    chk.lineIncPos = mLineIncPos;
    mToken.copyToStr(chk.tok);
}

void Parser::restoreState(InternalState& chk)
{
    mPos = chk.pos;
    mTokStartPos = chk.tokStartPos;
    mTokEndPos = chk.tokEndPos;
    mLineStartPos = chk.lineStartPos;
    mTokParsed = chk.parsed;
    mLineNum = chk.linenum;
    mLineIncPos = chk.lineIncPos;
    mToken.clear();
    mToken.append(chk.tok);
}

void Parser::movePos(int delta)
{
    int np = mPos + delta;
    if (np >= 0 && np < mTxtLen)
    {
        mPos = np;
        mTokParsed = false;
    }
}

// errort::
std::string errort::message()
{
    if (number != 0)
    {
        switch (facility)
        {
            case FACILITY_PRIMAL:
            {
                // TODO:
                std::string msg("Primal error #");
                msg += std::to_string(number);
                return msg;
            }
            break;

            case FACILITY_ERRNO:
            {
                return std::string(strerror(number));
            }
            break;
 #ifdef _WIN32
            case FACILITY_WIN32:
            {
                LPSTR msgbuf = nullptr;
                size_t size = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    number,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR)&msgbuf,
                    0,
                    NULL);

                std::string msg(msgbuf, size);
                LocalFree(msgbuf);
                return msg;
            }
#endif
            break;
        }
    }
    return std::string();
}


// Utility functions

uint strHash(strparam msg)
{
    // Robert Sedgewick hash function
    uint b = 378551;
    uint a = 63689;
    uint hash = 0;

    const char* str = msg.data();
    size_t len = msg.length();
    for (size_t i = 0; i < len; i++)
    {
        hash = hash * a + str[i];
        a = a * b;
    }

    return hash;
}

int randInt()
{
    static bool srcalled = false;
    if (!srcalled)
    {
        time_t t;
        srand((unsigned)time(&t));
        srcalled = true;
    }
    return rand();
}

void upperCase(std::string& str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        str[i] = (char)toupper(str[i]);
    }
}

void lowerCase(std::string& str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        str[i] = (char)tolower(str[i]);
    }
}

void trimLeft(std::string& str)
{
    size_t len = str.size();
    size_t i = 0;
    while (i < len && isspace(str[i]))
    {
        i++;
    }
    str.erase(0, i);
}

void trimRight(std::string& str)
{
    size_t i = str.size();
    while (i > 0 && isspace(str[i - 1]))
    {
        i--;
    }
    str.erase(i, str.size());
}

void padRight(std::string& str, size_t width)
{
    if (str.size() < width)
    {
        str.append(width - str.size(), ' ');
    }
}

bool vformat(string& outstr, const char* fmt, va_list varg)
{
    const int stackbufsize = 256;
    char stackbuf[stackbufsize];
    int count;
    va_list orgvarg;

    va_copy(orgvarg, varg);

    // Try to format using the small stack buffer.

    count = vsnprintf(stackbuf, stackbufsize, fmt, varg);
    if (count < 0)
    {
        // Negative number means formatting error.

        return false;
    }

    if (count < stackbufsize)
    {
        // Non-negative and less than the given size so string was completely written.

        outstr.assign(stackbuf);
    }
    else
    {
        // Format the string with a suffiently large heap buffer.

        void* buf = malloc(count + 1);
        if (buf == NULL)
        {
            return false;
        }

        vsnprintf((char*)buf, count + 1, fmt, orgvarg);

        outstr.assign((const char*)buf);
        free(buf);
    }
    return true;
}

void format(string& outstr, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    bool ret = vformat(outstr, fmt, va);
    va_end(va);

    if (!ret)
    {
        dbgerr("String format failed\n");
    }
}

string formatr(const char* fmt, ...)
{
    string outstr;

    va_list va;
    va_start(va, fmt);
    bool ret = vformat(outstr, fmt, va);
    va_end(va);

    if (!ret)
    {
        dbgerr("String format failed\n");
    }
    return outstr;
}

int64_t str2baseint(strparam str, int base, bool* valid /* = NULL */)
{
    char* end;
    bool res = true;

    errno = 0;
    int64_t value = strtoull(str.data(), &end, base);
    if ((errno != 0 && value == 0) || (*end != '\0'))
    {
        value = 0;
        res = false;
    }

    if (valid != NULL)
    {
        *valid = res;
    }
    return value;
}

double str2dbl(strparam str, bool* valid  /* = NULL */)
{
    char* end;
    bool res = true;

    double value = strtod(str.data(), &end);
    if ((*end != '\0'))
    {
        value = 0.0;
        res = false;
    }

    if (valid != NULL)
    {
        *valid = res;
    }
    return value;
}

bool strfind(const char* s, char c, size_t* pos)
{
    const char* foundp = strchr(s, c);
    if (foundp)
    {
        if (pos)
        {
            *pos = (foundp - s);
        }
        return true;
    }
    return false;
}


// Splitter::

Splitter::Splitter(const char* str, size_t len, char delim)
{
    mDelim = delim;
    mIgnore = '\0';
    mTxt = str;
    mLen = len;
    mPos = 0;
}

const char* Splitter::get()
{
    mCur.clear();
    while (!eof())
    {
        char c = mTxt[mPos++];
        if (c == mDelim)
        {
            break;
        }
        if (c != mIgnore)
        {
            mCur.appendc(c);
        }
    }
    return mCur.c_str();
}

// List:: (and ListP::)

void* ListP::allocFront()
{
    Node* newnode = allocNode();

    // Add at head
    newnode->mNext = mFront;
    if (mFront != nullptr)
    {
        mFront->mPrev = newnode;
    }
    mFront = newnode;

    // If this is the first node added, tail points here too
    if (mBack == nullptr)
    {
        mBack = newnode;
    }

    return newnode->data();
}

void* ListP::allocBack()
{
    Node* newnode = allocNode();

    // Add at tail.
    newnode->mPrev = mBack;
    if (mBack != nullptr)
    {
        mBack->mNext = newnode;
    }
    mBack = newnode;

    // If this is the first node added, head points here too.
    if (mFront == nullptr)
    {
        mFront = newnode;
    }

    return newnode->data();
}

void ListP::linkBack(void* data)
{
    if (data)
    {
        Node* newnode = Node::nodeFromData(data);
        // Add at tail.
        newnode->mPrev = mBack;
        if (mBack != nullptr)
        {
            mBack->mNext = newnode;
        }
        mBack = newnode;

        // If this is the first node added, head points here too.
        if (mFront == nullptr)
        {
            mFront = newnode;
        }
    }
}

void ListP::freeUnlinked(void* data)
{
    if (data)
    {
        Node* node = Node::nodeFromData(data);
        freeNode(node);
    }
}

bool ListP::removeP(void* data)
{
    if (data)
    {
        Node* node = Node::nodeFromData(data);
        unlinkP(node);
        return true;
    }
    return false;
}

void* ListP::next(void* data)
{
    if (data)
    {
        Node* node = Node::nodeFromData(data);
        if (node->mNext)
        {
            return node->mNext->data();
        }
    }
    return nullptr;
}

void* ListP::prev(void* data)
{
    if (data)
    {
        Node* node = Node::nodeFromData(data);
        if (node->mPrev)
        {
            return node->mPrev->data();
        }
    }
    return nullptr;
}

void ListP::clearP()
{
    Node* n = mFront;
    while (n)
    {
        Node* tmp = n;
        n = n->mNext;
        freeNode(tmp);
    }
    mFront = nullptr;
    mBack = nullptr;
}

void ListP::unlinkP(Node* node)
{
    // Unlink and delete it.
    Node* tmp = node;
    Node* prevnode = node->mPrev;
    Node* nextnode = node->mNext;

    if (prevnode != nullptr)
    {
        prevnode->mNext = nextnode;
    }
    else
    {
        mFront = nextnode;
    }
    if (nextnode != nullptr)
    {
        nextnode->mPrev = prevnode;
    }
    else
    {
        mBack = prevnode;
    }
    freeNode(tmp);
}


} // base
