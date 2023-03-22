
#include "parse.h"

namespace base
{

// DataSerParser::

void DataSerParser::parseNum(Variant& var)
{
    // number
    //    int
    //    int frac
    //    int exp
    //    int frac exp
    StrBld num;
    for (;;)
    {
        char c = token()[0];
        if (!isDigit(c) && c != '+' && c != '-' && c != 'e' && c != 'E' && c != '.')
        {
            break;
        }

        num.append(token());
        if (advanceSameLine())
        {
            if (tokenEquals(""))
            {
                advance();
            }
            break;
        }
    }
    const char* numstr = num.c_str();

    bool isint = num.length() < 20;
    if (isint)
    {
        const char* p = numstr;
        while (*p)
        {
            if (*p == 'e' || *p == 'E' || *p == '.')
            {
                isint = false;
                break;
            }
            p++;
        }
    }

    bool valid = false;
    if (isint)
    {
        if (num.length() >= 2 && *numstr == '0')
        {
            // In json, ints are not allowed to start with zero, invalid
        }
        else
        {
            char* ench;
            int64_t li = strtoull(numstr, &ench, 10);
            if (*ench == '\0')
            {
                valid = true;
                var = li;
            }
        }
    }
    else
    {
        char* ench;
        double dbl = strtod(numstr, &ench);
        if (*ench == '\0')
        {
            valid = true;
            var = dbl;
        }
    }

    if (!valid)
    {
        var = VNULL;
        std::string err;
        format(err, "Invalid number '%s'", numstr);
        setError(err.c_str());
    }
}

void DataSerParser::parseString(Variant& var)
{
    token().stripQuotes(isFlagSet(mFlags, FLAG_FLEXQUOTES));
    var = token().toString();
    advance();
}

bool DataSerParser::isString(StrBld& s, bool requirequotes)
{
    size_t l = s.length();
    bool flexquotes = isFlagSet(mFlags, FLAG_FLEXQUOTES);

    if (l >= 2)
    {
        // In Json, we require strings to have double quotes

        if (s[0] == '"' && s[l - 1] == '"')
        {
            return true;
        }
        else if (flexquotes)
        {
            // Allow single quotes

            if (s[0] == '\'' && s[l - 1] == '\'')
            {
                return true;
            }
        }
    }

    // If we get here that means there are no quotes (single or double) or string
    // is shorter than 2 chars.   If we are allowed to have flexible quotes and
    // quotes are not required, try to allow a "word" without quotes.
    if (flexquotes && !requirequotes)
    {
        //TODO: are there other rules for non-quoted string property names

        bool isword = true;
        for (int i = 0; i < l; i++)
        {
            if (!charWord(s[i]))
            {
                isword = false;
                break;
            }
        }
        if (isword)
        {
            return true;
        }
    }

    return false;
}

// JsonParser::

void JsonParser::parseInternal(Variant& outvar, const char* txt, size_t len, uint32_t flags, InternalState* state)
{
    initDataSer(txt, len, flags);

    if (state != nullptr)
    {
        restoreState(*state);
    }

    if (isFlagSet(mFlags, FLAG_ARRAYONLY))
    {
        parseArray(outvar);
    }
    else if (isFlagSet(mFlags, FLAG_OBJECTONLY))
    {
        parseObject(outvar);
    }
    else
    {
        if (tokenEquals('['))
        {
            parseArray(outvar);
        }
        else
        {
            parseObject(outvar);
        }
    }

    if (state != nullptr)
    {
        saveState(*state);
    }
    else
    {
        // Only check for extra input if no position handling is need (ie json is part of another document)
        advance();
        if (!failed() && !eof() && !token().empty())
        {
            std::string s;
            format(s, "Extra input '%s'", token().c_str());
            setError(s.c_str());
        }
    }

    if (failed())
    {
        dbglog("Json parsing failed: %s\n", errMsg().c_str());
    }
}


void JsonParser::parseObject(Variant& var)
{
    // object
    //    {}
    //    { members }
    advance('{');

    var.createObject();

    parseMembers(var);
    advance('}');
}

void JsonParser::parseMembers(Variant& var)
{
    // members
    //    pair
    //    pair, members
    // pair
    //    string : value
    StrBld key;
    while (!tokenEquals('}') && !failed())
    {
        // A property/key name can be a string.  Quotes can be single or double and
        // are optional in some cases.
        key.clear();
        if (isString(token(), false))
        {
            token().stripQuotes(isFlagSet(mFlags, FLAG_FLEXQUOTES));
            key.moveFrom(token());

            advance();
        }
        advance(':');

        // While JSON standard don't say what the right behavior is, this follows the
        // JavaScript behavior and the value is set to the very last value set.
        Variant& newprop = var.setProp(key.c_str());

        parseValue(newprop);
        if (tokenEquals(','))
        {
            advance();
            if (tokenEquals('}'))
            {
                setError("Found , followed by }");
            }
        }
    }
}

void JsonParser::parseArray(Variant& var)
{
    // array
    //    []
    //    [ elements ]
    advance('[');
    var.createArray();
    parseElements(var);
    advance(']');
}

void JsonParser::parseElements(Variant& var)
{
    // elements
    //    value
    //    value , elements
    while (!tokenEquals(']') && !failed())
    {
        Variant v;
        parseValue(v);
        var.append(v);

        if (tokenEquals(','))
        {
            advance();
            if (tokenEquals(']'))
            {
                setError("Found , followed by ]");
            }
        }
    }
}

void JsonParser::parseValue(Variant& var)
{
    // value
    //    string
    //    number
    //    object
    //    array
    //    true
    //    false
    //    null
    if (isNum(token()))
    {
        parseNum(var);
    }
    else if (isArray(token()))
    {
        parseArray(var);
    }
    else if (isObject(token()))
    {
        parseObject(var);
    }
    else if (tokenEquals("true"))
    {
        var = true;
        advance();
    }
    else if (tokenEquals("false"))
    {
        var = false;
        advance();
    }
    else if (tokenEquals("null"))
    {
        var = VNULL;
        advance();
    }
    else if (isString(token(), true))
    {
        parseString(var);
    }
    else
    {
        var = VNULL;
        std::string err;
        format(err, "Invalid value '%s'", token().c_str());
        setError(err.c_str());
    }
}

// YamlParser::

void YamlParser::parse(Variant& outvar, uint32_t flags)
{
    initDataSer((const char*)mTxtBuf.cptr(), mTxtBuf.size(), flags);

    enableHashComment(true);

    if (tokenEquals("---"))
    {
        advance();
    }

    parseYamlValue(outvar, -1);

    if (tokenEquals("..."))
    {
        advance();
    }

    advance();
    if (!failed() && !eof() && !token().empty())
    {
        std::string s;
        format(s, "Extra input '%s'", token().c_str());
        setError(s.c_str());
    }
    if (failed())
    {
        dbglog("Yaml parsing failed: %s\n", errMsg().c_str());
    }
}


void YamlParser::parseSequence(Variant& var, int parentcolumn)
{
    int startcol = tokenColumn();
    // if (startcol == parentcolumn)
    // {
    //     var.clear();
    //     return;
    // }

    var.createArray();
    for (;;)
    {
        if (!isSequence())
        {
            setError("Expecting a array elem");
            break;
        }
        advance();

        Variant v;
        parseYamlValue(v, startcol);
        var.append(v);

        if (eof())
        {
            break;
        }
        if (!isSequence())
        {
            break;
        }
        if (tokenColumn() < startcol)
        {
            break;
        }
    }
}

void YamlParser::parseYamlObject(Variant& var, int parentcolumn)
{
    // Object. If there is a : token on this line, it is a object map field name
    int startcol = tokenColumn();
    if (startcol == parentcolumn)
    {
        var.clear();
        return;
    }

    var.createObject();
    for (;;)
    {
        // At this point we are expecting a object key, key name is complex and we need to
        // read up to the a :
        if (!tokPeekInLine(":", true))
        {
            setError("Expecting a object field key name");
            break;
        }
        advance();

        std::string key = tokLineSoFarStr();
        if (startcol < key.length())
        {
            key = key.substr(startcol);
        }

        Variant v;
        parseYamlValue(v, startcol);

        var.setProp(key.c_str(), v);

        if (eof())
        {
            break;
        }
        if (tokenColumn() < startcol)
        {
            break;
        }
        if (!tokPeekInLine(":", false))
        {
            break;
        }
    }
}

void YamlParser::parseYamlValue(Variant& var, int parentcolumn)
{
    if (isSequence())
    {
        parseSequence(var, parentcolumn);
    }
    else if (isEmbeddedJson())
    {
        JsonParser json;

        // Skip back one char to before { or [ so json parser can process that
        movePos(-1);

        // Save state and parse to json parser
        Parser::InternalState state;
        Parser::saveState(state);
        json.parseInternal(var, getText(), getTextLen(),
            FLAG_FLEXQUOTES | (tokenEquals('[') ? FLAG_ARRAYONLY : FLAG_OBJECTONLY), &state);
        Parser::restoreState(state);
    }
    else if (tokPeekInLine(":", false))
    {
        parseYamlObject(var, parentcolumn);
    }
    // else if (isNum(token()))
    // {
    //     parseNum(var);
    // }
    else if (tokenEquals("true"))
    {
        var = true;
        advance();
    }
    else if (tokenEquals("false"))
    {
        var = false;
        advance();
    }
    else if (tokenEquals("null"))
    {
        var = VNULL;
        advance();
    }
    else if (tokenEquals('|'))
    {
        parseBlockLiteral(var, '|');
    }
    else if (tokenEquals('>'))
    {
        parseBlockLiteral(var, '>');
    }
    else if (isString(token(), true))
    {
        //parseString(var);
        token().stripQuotes(true);
        var = token().toString();
        advance();
    }
    else
    {
        // If none of the above, it must be a value without quotes. Read till eol.
        // TODO: The string is not processed as a quoted string by the parser so is not
        // full escaped.  Should escape characters.  Check for \r
        // TODO: Implement !!int, !!str !!float etc
        // TODO: Implement auto-detect of int, double
        std::string s = tokGetTillEOL();
        if (eof() && base::streql(s, ":"))
        {
            var = VNULL;
        }
        else
        {
            var = s;
        }
    }
}

void YamlParser::parseBlockLiteral(Variant& var, char blockstyle)
{
    char chomp = '\0';
    advance();
    if (tokenEquals('+'))
    {
        chomp = '+';
        advance();
    }
    else if (tokenEquals('-'))
    {
        chomp = '-';
        advance();
    }

    std::string value;
    int firstindent = 0;
    while (!eof())
    {
        InternalState chk;
        saveState(chk);

        std::string s = tokGetFullLine();

        // Calc indent and see if the string is empty
        size_t len = s.size();
        int ind = 0;
        while (ind < len && isspace(s[ind]))
        {
            ind++;
        }
        bool empty = (ind == len);

        if (!empty)
        {
            if (firstindent == 0)
            {
                firstindent = ind;
            }
            if (ind < firstindent)
            {
                // De-indented... restore to prev line. exit
                restoreState(chk);
                break;
            }
            else
            {
                s.erase(0, firstindent);
            }
        }

        if (value.length() != 0)
        {
            if (blockstyle == '|')
            {
                value += '\n';
            }
            else if (blockstyle == '>')
            {
                value += ' ';
            }
        }
        value += s;
    }

    var = value;
}


} // base