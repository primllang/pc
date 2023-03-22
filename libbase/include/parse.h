#pragma once

#include "var.h"

namespace base
{

// Data serialization base parser
class DataSerParser : public Parser
{
public:
    enum
    {
        FLAG_FLEXQUOTES = 0x1,
        FLAG_OBJECTONLY = 0x2,
        FLAG_ARRAYONLY = 0x4
    };

    bool readFile(const char* filename)
    {
        // Read the text file into the buffer but don't append a null terminator.
        // The buffer is used by the parser which uses the buffer size and doesn't expect
        // a null terminated string.
        return mTxtBuf.readFile(filename, false);
    }

protected:
    void initDataSer(const char* txt, size_t len, uint32_t flags = 0)
    {
        mFlags = flags;
        Parser::initParser(txt, len);
    }
    void parseNum(Variant& var);
    void parseString(Variant& var);
    bool isString(StrBld& s, bool requirequotes);
    bool isArray(StrBld& s)
    {
        return s.equals('[');
    }
    bool isObject(StrBld& s)
    {
        return s.equals('{');
    }
    bool isNum(StrBld& s)
    {
        char c = s[0];
        if (isDigit(c) || c == '-')
        {
            return true;
        }
        return false;
    }
    bool isDigit(char c)
    {
        return (c >='0' && c <= '9');
    }

protected:
    Buffer mTxtBuf;
    uint32_t mFlags;
};

// JsonParser class parses a json string into a Variant structure.
class JsonParser : public DataSerParser
{
public:
    JsonParser() = default;
    void parse(Variant& outvar, uint32_t flags = 0)
    {
        parseInternal(outvar, (const char*)mTxtBuf.cptr(), mTxtBuf.size(), flags, nullptr);
    }

    void parseObject(Variant& var);
    void parseMembers(Variant& var);
    void parseArray(Variant& var);
    void parseElements(Variant& var);
    void parseValue(Variant& var);

    void parseInternal(Variant& outvar, const char* txt, size_t len, uint32_t flags, InternalState* state);
};

// YamlParser class
class YamlParser : public DataSerParser
{
public:
    YamlParser() = default;
    void parse(Variant& outvar, uint32_t flags = 0);

protected:
    bool isEmpty(std::string& str);
    bool isSequence()
    {
        if (tokenEquals(""))
        {
            advance();
        }
        bool minus = tokenEquals('-');
        char nc = tokNextChar();
        return (minus && (nc == ' ' || nc == '\r' || nc == '\n'));
    }
    bool isEmbeddedJson()
    {
        return (tokenEquals('{') || tokenEquals('['));
    }

    void parseYamlValue(Variant& var, int parentcolumn);
    void parseBlockLiteral(Variant& var, char blockstyle);
    void parseSequence(Variant& var, int parentcolumn);
    void parseYamlObject(Variant& var, int parentcolumn);
};


} // base

