#include "var.h"
#include "primalc.h"

PlConfig sConfig;
PlCppProp sCppProp;
bool sVerbose = false;

int plLogShell(const std::string& cmd, base::Buffer& log, base::Buffer& errlog)
{
    constDef EXTRALINES = 2;
    int ret = 0;

    base::Path tempfn;
    base::getTempFilename(tempfn);

    ret = base::shellCmd(cmd, false, &tempfn);

    // read the log buffer from temp file
    base::Buffer buf;
    if (buf.readFile(tempfn, false))
    {
        // Parse out error messages from log buffer
        base::StrBld errs;
        base::Splitter sp(buf, '\n');
        sp.ignore('\r');
        while (!sp.eof())
        {
            size_t l;
            const char* s = sp.get(&l);
            if (strstr(s, "error:") || strstr(s, "warning:"))
            {
                errs.append(s, l);
                errs.appendc('\n');
                for (int i = 0; i < EXTRALINES; i++)
                {
                    if (!sp.eof())
                    {
                        s = sp.get(&l);
                        errs.append(s, l);
                        errs.appendc('\n');
                    }
                }
            }
        }

        // Print the error strings
        fprintf(stderr, "%s%s%s", ESC_RED, errs.c_str(), ESC_DEFAULT);

        // Append the full log to the log buffer for accumulation
        log.appendFrom(buf);

        // Append the found errors strings to the error log
        errs.moveToBuffer(buf);
        errlog.appendFrom(buf);
    }

    base::deleteFile(tempfn);

    return ret;
}

bool plReadYaml(const char* content, const base::Path fn, base::Variant& var)
{
    if (isFlagSet(base::sBaseGFlags, base::GFLAG_VERBOSE_FILESYS))
    {
        dbglog("Loading '%s' yaml from file %s\n", content, fn.c_str());
    }
    if (!var.readYamlFile(fn.c_str()))
    {
        dbgerr("Failed to load '%s' from %s\n", content, fn.c_str());
        return false;
    }
    //dbglog("%s\n", var.toJsonString().c_str());
    return true;
}

bool plReadJson(const char* content, const base::Path fn, base::Variant& var)
{
    if (isFlagSet(base::sBaseGFlags, base::GFLAG_VERBOSE_FILESYS))
    {
        dbglog("Loading '%s' json from file %s\n", content, fn.c_str());
    }
    if (!var.readJsonFile(fn.c_str()))
    {
        dbgerr("Failed to load '%s' from %s\n", content, fn.c_str());
        return false;
    }
    return true;
}

bool plWrite(const char* content, const base::Buffer& buf, const base::Path fn, bool onlywhendiff)
{
    // NOTE: if onlywhendiff is true, the function performs a binary compare
    // and only writes the file if the files are different. And when this
    // option is true, the return value of the function will be false if the
    // file was not written (and it doesn't indicate an error)

    bool shouldwr = true;
    if (onlywhendiff)
    {
        // Assume different
        bool dif = true;
        size_t osize;
        base::getFileSize(fn, osize);
        if (osize == buf.size())
        {
            base::Buffer obuf;
            if (obuf.readFile(fn, false))
            {
                byte* b = (byte*)buf.cptr();
                byte* ob = (byte*)obuf.cptr();
                dif = false;
                for (size_t i = 0; i < buf.size(); i++)
                {
                    if (b[i] != ob[i])
                    {
                        dif = true;
                        break;
                    }
                }
            }
        }
        shouldwr = dif;
    }

    if (isFlagSet(base::sBaseGFlags, base::GFLAG_VERBOSE_FILESYS))
    {
        if (shouldwr)
        {
            dbglog("Writing '%s' to file %s\n", content, fn.c_str());
        }
        else
        {
            dbglog("'%s' hasn't changed. Not updating file %s\n", content, fn.c_str());
        }
    }
    // NOTE: If shouldwr is false, it is due to 'write only when different' and
    // in this case, the function returns false if the file is not written.
    bool ret = shouldwr;
    if (shouldwr)
    {
        ret = buf.writeFile(fn);
    }
    return ret;
}

bool plRead(const char* content, base::Buffer& buf, const base::Path fn, bool nullterm)
{
    if (isFlagSet(base::sBaseGFlags, base::GFLAG_VERBOSE_FILESYS))
    {
        dbglog("Reading '%s' to file %s, nullterm=%d\n", content, fn.c_str(), nullterm);
    }
    return buf.readFile(fn, nullterm);
}


bool plIsIdent(strparam str)
{
    const char* s = str.data();
    size_t len = str.length();
    // Basic checks
    if (len == 0)
    {
        return false;
    }
    if (!isalpha(s[0]) && s[0] != '_')
    {
        return false;
    }
    for (int i = 1; i < len; i++)
    {
        char c = s[i];
        if (!isalnum(c) && c != '_')
        {
            return false;
        }
    }
    // TODO: disallow __ if it becomes reserved
    // Check it is not a language keyword
    return !sKeywordMap.exists(str);
}


// PlCppProp::
PlCppProp::PlCppProp()
{
    mProp.createObject();

    // Set defaults
    setStr(CppPropType::unitwidehdrfn, "unitwide.h");
}

std::string PlCppProp::getStr(CppPropType keyenum)
{
    base::Variant value;
    const char* keyname = sCppPropTypeMap.toString(keyenum);
    value = mProp[keyname];

    return value.toString();
}

void PlCppProp::setStr(CppPropType keyenum, std::string value)
{
    const char* keyname = sCppPropTypeMap.toString(keyenum);
    setStr(keyname, value);
}

void PlCppProp::setStr(std::string key, std::string value)
{
    base::Variant v(value);
    mProp.setProp(key.c_str(), v);
}


bool PlCppProp::convValidNum(std::string& num, DataType& dtype)
{
    bool valid = false;

    // Copy, remove underscores, conv to uppercase
    std::string s;
    for (int i = 0; i < num.length(); i++)
    {
        char c = (char)toupper(num[i]);
        if (c == '_')
        {
            continue;
        }
        s += c;
    }

    size_t base = 10;
    std::string beg = s.substr(0, 2);
    // If there is a negative sign, base 10 is applied
    if (beg == "0X")
    {
        base = 16;
        s.erase(0, 2);
    }
    else if (beg == "0B")
    {
        base = 2;
        s.erase(0, 2);
    }

    bool sign = (s.length() > 1) && s[0] == '-';
    bool floating = false;
    size_t bits = 64;
    std::string end = (s.length() > 2 ? s.substr(s.length() - 2) : "");
    if (end == "U8" || end == "I8")
    {
        sign = end[0] == 'I';
        bits = 8;
        s.erase(s.length() - 2);
    }
    else if (s.length() > 3)
    {
        end = s.substr(s.length() - 3);
        auto n = strtol(end.substr(1).c_str(), nullptr, 10);
        if (end[0] == 'U' || end[0] == 'I')
        {
            // Integer
            if (n == 16 || n == 32 || n == 64)
            {
                sign = end[0] == 'I';
                bits = n;
                s.erase(s.length() - 3);
            }
        }
        else if (end[0] == 'F' && base == 10)
        {
            // Floating
            if (n == 32 || n == 64)
            {
                floating = true;
                bits = n;
                s.erase(s.length() - 3);
            }
        }
    }

    char* ench;
    if (!s.empty())
    {
        int64_t li = strtoull(s.c_str(), &ench, base);
        if (*ench == '\0')
        {
            valid = true;
        }

        if (!valid)
        {
            double dbl = strtod(s.c_str(), &ench);
            if (*ench == '\0')
            {
                floating = true;
                valid = true;
            }
        }
    }

    // For valid numbers, convert to CPP format string and determine type
    if (valid)
    {
        num.clear();
        if (base == 16)
        {
            num += "0X";
        }
        else if (base == 2)
        {
            num += "0B";
        }

        num += s;

        if (floating)
        {
            if (bits == 32)
            {
                dtype = DataType::d_float32;
                num += "F";
            }
            else
            {
                dtype = DataType::d_float64;
            }
        }
        else
        {
            if (sign)
            {
                switch (bits)
                {
                    case 8:
                    dtype = DataType::d_int8;
                    break;

                    case 16:
                    dtype = DataType::d_int16;
                    break;

                    case 32:
                    dtype = DataType::d_int32;
                    break;

                    case 64:
                    dtype = DataType::d_int64;
                    num += "LL";
                    break;
                }
            }
            else
            {
                switch (bits)
                {
                    case 8:
                    dtype = DataType::d_uint8;
                    break;

                    case 16:
                    dtype = DataType::d_uint16;
                    break;

                    case 32:
                    dtype = DataType::d_uint32;
                    num += "U";
                    break;

                    case 64:
                    dtype = DataType::d_uint64;
                    num += "ULL";
                    break;
                }
            }
        }
    }

    return valid;
}


// PlConfig::
bool PlConfig::init(base::Path configfn)
{
    if (configfn.empty())
    {
        // Look in the directory where the executable was launched from
        base::Path bindir = base::getProcPath(base::getPid()).dirPart();

        // ..\templates (if running from deployed instance)
        // ..\..\primalc\templates (if running from where it was built)
        configfn.set(bindir.dirPart(), "templates");
        configfn.add(CONFIGFN);
        if (!base::existFile(configfn))
        {
            configfn.set(bindir.dirPart().dirPart(), "primalc");
            configfn.add("templates");
            configfn.add(CONFIGFN);
        }
    }

    if (!plReadYaml("config", configfn, mCfg))
    {
        return false;
    }
    mCfgFn = configfn;

    #ifdef _WIN32
        mPlat = WIN32PLAT;
    #else
        mPlat = LINUXPLAT;
    #endif

    return true;
}

std::string PlConfig::getStr(Config keyenum)
{
    base::Variant value;

    const char* keyname = sConfigMap.toString(keyenum);

    // Check platform specific
    value = mCfg[mPlat][keyname];
    if (value.empty())
    {
        // Check common
        value = mCfg[COMMON][keyname];
    }
    if (value.empty())
    {
        // Check default value
        for (size_t i = 0; i < countof(sCfgDefaultMap); i++)
        {
            if (keyenum == sCfgDefaultMap[i].cfg)
            {
                value = sCfgDefaultMap[i].defvalue;
                break;
            }
        }
    }
    return value.toString();
}

base::Path PlConfig::getPath(Config keyenum)
{
    // If relative path, the path is considered from location of config file
    base::Path p = fixPath(getStr(keyenum));
    if (p.isAbs())
    {
        return p;
    }
    return base::Path(mCfgFn.dirPart(), p);
}

base::Path PlConfig::fixPath(std::string path)
{
    base::Path p(path);
    p.ensureNativeSlash();
    return p;
}

// PlDiscern::

bool PlDiscern::isCompoundIdent(strparam str)
{
    bool ret = isSimpleIdent(str);
    if (!ret)
    {
        ret = true;
        base::Splitter sp(str.data(), '.');
        while (!sp.eof())
        {
            if (!isSimpleIdent(sp.get()))
            {
                ret = false;
                break;
            }
        }
    }
    return ret;
}

bool PlDiscern::isSimpleIdent(strparam str)
{
    return plIsIdent(str);
}

bool PlDiscern::isNumPart(const std::string& s)
{
    // Allows _, E, e u, i, f, and hex digits inside number parts
    // Doesn't fully validate it is a valid numeric literal

    if (s.empty())
    {
        return false;
    }
    else if (!isDigit(s[0]))
    {
        return false;
    }
    else
    {
        bool ret = true;
        for (int i = 0; i < s.length(); i++)
        {
            if (strchr("0123456789_xXuUiaAbBcCdDeEfF", s[i]) != NULL)
            {
                continue;
            }
            ret = false;
            break;
        }
        return ret;
    }
}

bool PlDiscern::isStrLit(const std::string& s)
{
    size_t l = s.length();
    if (l >= 2)
    {
        if (s[0] == '"' && s[l - 1] == '"')
        {
            return true;
        }
        else if (s[0] == '\'' && s[l - 1] == '\'')
        {
            return true;
        }
    }
    return false;
}


// PlErrs::

void PlErrs::add(EntityType en, std::string msgstr)
{
    std::string msg;
    if (en)
    {
        if (!mCtxFn.empty())
        {
            // ex: filename:7:16: error: errmsg
            msg = base::formatr("%s:%d:%d: error: ", mCtxFn.c_str(), en->mToken.mBegLine, en->mToken.mBegColumn);
            mCtxErrCnt++;
        }
        else
        {
            msg = base::formatr("At %d:%d: ", en->mToken.mBegLine, en->mToken.mBegColumn);
        }
    }
    msg += msgstr;
    mErrs.addBack(msg);
}

void PlErrs::add(EntityType en, const char* fmt, ...)
{
    std::string usermsg;
    va_list va;
    va_start(va, fmt);
    bool ret = base::vformat(usermsg, fmt, va);
    va_end(va);

    add(en, usermsg);
}

void PlErrs::print()
{
    for (base::Iter<std::string> e; mErrs.forEach(e); )
    {
        plPrintErr(e->c_str());
    }
}

void plPrintErr(strparam str)
{
    fprintf(stderr, "%s%s%s\n", ESC_RED, str.data(), ESC_DEFAULT);
}

