#include "primalc.h"

// PlToken

PlToken PlToken::sNilTok;

std::string PlToken::strNoQuotes(bool allowsingle) const
{
    std::string s;
    const char* buf = cstr();
    size_t len = strlen(buf);
    if (len >= 2)
    {
        char c1 = buf[0];
        char c2 = buf[len - 1];

        if ((c1 == '"' && c2 == '"') ||
            (allowsingle && c1 == '\'' && c2 == '\'') )
        {
            s.append(buf + 1, len - 2);
            return s;
        }
    }
    s.assign(buf, len);
    return s;
}


// PlTokenParser::

bool PlTokenParser::tokenize(const std::string& srcfn, bool removecomments)
{
    base::Buffer source;

    assert(mSrcFn.empty());

    // Read primal source (null terminate)
    if (!plRead("Primal source", source, srcfn, true))
    {
        return false;
    }
    mSrcFn = srcfn;

    mParser.initBase((const char*)source.cptr(), source.size());

    PlToken tok;
    while (mParser.readToken(tok))
    {
        if (removecomments && tok.isComment())
        {
            continue;
        }
        mTokens.emplace_back(tok);
    }
    if (mParser.failed())
    {
        dbgerr("Error: %s\n", mParser.errMsg().c_str());
        return false;
    }
    //dbglog("Parse: %zu tokens added\n", mTokens.size());
    // while (avail())
    // {
    //     size_t line = curLine();
    //     while (availForLine(line))
    //     {
    //         dbgTok(nullptr);
    //         advance();
    //     }
    //     dbglog("\n");
    // }
    // mCurIndex = 0;

    return true;
}

void PlTokenParser::expectNL()
{
    if (!mParser.failed())
    {
        if (!isAtNL())
        {
            dbgDumpTokens();
            pushErr("Expecting token to be on new line");
        }
    }
}

void PlTokenParser::expectLine(size_t lin)
{
    if (curLine() != lin)
    {
        pushErr("Need to be on same line (check line continuation)");
    }
}

bool PlTokenParser::advance(const char* tokstr)
{
    if (!mParser.failed())
    {
        if (token().equals(tokstr))
        {
            return advance();
        }
        else
        {
            pushErr("Expecting token [%s] but found", tokstr);
        }
    }
    return false;
}

void PlTokenParser::pushErr(const char* fmt, ...)
{
    base::StrBld str;
    if (!token().isEmpty())
    {
        // ex: filename:7:16: error: errmsg
        str.appendFmt("%s:%d:%d: error: ", mSrcFn.c_str(), token().mBegLine, token().mBegColumn);
    }

    va_list va;
    va_start(va, fmt);
    str.appendVFmt(fmt, va);
    va_end(va);

    str.appendFmt(" ('%s' #%zu)", token().cstr(), tokIndex());

    mParser.setError(str.c_str());
    plPrintErr(str.c_str());
}


// PrTokenParser::BaseParser::
bool PlTokenParser::BaseParser::readToken(PlToken& tok)
{
    if (eof())
    {
        // Reached the end of source file
        return false;
    }

    bool linecont = false;
    tok.clear();
    tok.mBegLine = tokenLine();
    tok.mBegColumn = tokenColumn() + 1;

    if (mNL)
    {
        setFlag(tok.mFlags, PlToken::FLAG_NL);
    }

    combinePunc();

    if (tokenEquals("//"))
    {
        const char* str;
        size_t len;
        tokenEOLPtrLen(&str, &len);
        tok.setStr(str, len);

        setFlag(tok.mFlags, PlToken::FLAG_EOL);
        setFlag(tok.mFlags, PlToken::FLAG_CPPCOMMENT);
    }
    else if (tokenEquals("/*"))
    {
        int begpos = tokStartPos();

        advance();
        combinePunc();
        while (!tokenEquals("*/"))
        {
            advance();
            combinePunc();
        }

        const char* str;
        size_t len;
        tokenSubstrPtrLen(&str, &len, begpos, tokEndPos());
        tok.setStr(str, len);

        setFlag(tok.mFlags, PlToken::FLAG_CCOMMENT);
    }
    else
    {
        // Further combine functionation for ::= and ...
        if (tokenEquals("::"))
        {
            tokInclude('=');
        }
        else if (tokenEquals(".."))
        {
            tokInclude('.');
        }


        if (tokenEquals(""))
        {
            return false;
        }
        linecont = (tokenEquals('\\'));
        const char* str;
        size_t len;
        tokenPtrLen(&str, &len);
        tok.setStr(str, len);
    }

    // Advance
    if (advanceSameLine())
    {
        setFlag(tok.mFlags, PlToken::FLAG_EOL);
        if (tokenEquals(""))
        {
            advance();
        }

        // If we found a line continuation and it is end of line,
        // remove clear EOL flag and skip the line continuation char.
        if (linecont)
        {
            clearFlag(tok.mFlags, PlToken::FLAG_EOL);
            advance();

            const char* str;
            size_t len;
            tokenPtrLen(&str, &len);
            tok.setStr(str, len);
            advance();
        }
    }

    mNL = (isFlagSet(tok.mFlags, PlToken::FLAG_EOL));
    return true;
}

void PlTokenParser::BaseParser::combinePunc()
{
    // Check for 2 char operators
    if (tokenLen() == 1)
    {
        switch (tokenFirst())
        {
        // Handle //, /* and /=
        case '/':
            if (!tokInclude('/'))
            {
                if (!tokInclude('*'))
                {
                    tokInclude('=');
                }
            }
            break;
        // Handle */ and *=
        case '*':
            if (!tokInclude('/'))
            {
                tokInclude('=');
            }
            break;
        // Handle ->, -= and --
        case '-':
            if (!tokInclude('>'))
            {
                if (!tokInclude('='))
                {
                    tokInclude('-');
                }
            }
            break;
        // Handle += and ++
        case '+':
            if (!tokInclude('='))
            {
                tokInclude('+');
            }
            break;
        // Handle ==
        case '=':
            tokInclude('=');
            break;
        // Handle %=
        case '%':
            tokInclude('=');
            break;
        // Handle >= and >>
        case '>':
            if (!tokInclude('>'))
            {
                tokInclude('=');
            }
            break;
        // Handle <= and <<
        case '<':
            if (!tokInclude('<'))
            {
                tokInclude('=');
            }
            break;
        // Handle ::
        case ':':
            tokInclude(':');
            break;
        // Handle ..
        case '.':
            tokInclude('.');
            break;
        }
    }
}

EntityType PlTokenParser::createEn(EntityType parent, EKind ekind, ETag etag)
{
    // Uses current parsed token
    return PlEntity::newEntity(parent, ekind, token(), etag);
}

EntityType PlTokenParser::createNilEn(EntityType parent, EKind ekind, ETag etag)
{
    // Uses a nil token
    EntityType en = PlEntity::newEntity(parent, ekind, PlToken::sNilTok, etag);
    return en;
}

EntityType PlTokenParser::createPubEn(EntityType parent, EKind ekind, ETag etag, bool pub)
{
    // Uses current parsed token
    EntityType en = PlEntity::newEntity(parent, ekind, token(), etag);
    if (pub)
    {
        en->addAttrib(EAttribFlags::a_public);
    }
    return en;
}

EntityType PlTokenParser::createNilPubEn(EntityType parent, EKind ekind, ETag etag, bool pub)
{
    // Uses nil token
    EntityType en = PlEntity::newEntity(parent, ekind, PlToken::sNilTok, etag);
    if (pub)
    {
        en->addAttrib(EAttribFlags::a_public);
    }
    return en;
}


EntityType PlTokenParser::createSingleEn(EntityType parent, EKind ekind, ETag etag)
{
    // Doesn't allow dups... finds existing one and create only if not found
    EntityType en = parent->getChild(ekind, etag);
    return (en) ? en : createEn(parent, ekind, etag);
}

void PlTokenParser::dumpTokens(base::StrBld& bld)
{
    size_t i = 0;
    for (auto& t : mTokens)
    {
        std::string ts(t.toString());
        base::padRight(ts, 20);
        bld.appendFmt("%3zu: %s %d:%d %s%s%s%s\n",
            i++,
            ts.c_str(),
            t.mBegLine,
            t.mBegColumn,
            isFlagSet(t.mFlags, PlToken::FLAG_EOL) ? "EOL " : "",
            isFlagSet(t.mFlags, PlToken::FLAG_NL) ? "NL " : "",
            isFlagSet(t.mFlags, PlToken::FLAG_CCOMMENT) ? "C " : "",
            isFlagSet(t.mFlags, PlToken::FLAG_CPPCOMMENT) ? "CC " : "");
    }
}

void PlTokenParser::dbgTok(const char* label)
{
    if (label)
    {
        dbglog("%s '%s'\n", label, token().toString().c_str());
    }
    else
    {
        dbglog("'%s' ", token().toString().c_str());
    }
}

void PlTokenParser::dbgDumpTokens()
{
#ifdef _DEBUG
    base::StrBld bld;
    dumpTokens(bld);
    dbglog("%s", bld.c_str());
#endif
}
