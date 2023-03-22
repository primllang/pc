#include "var.h"
#include "sys.h"
#include "parse.h"

namespace base
{

Variant Variant::sEmpty;
Variant Variant::sNull(Variant::V_NULL);

// Variant::

bool Variant::parseJson(const char* jsontxt)
{
    if (mData.type == V_NULL)
    {
        return false;
    }
    JsonParser json;
    json.parseInternal(*this, jsontxt, strlen(jsontxt), 0, nullptr);
    bool err = json.failed();
    if (err)
    {
        clear();
    }
    return !err;
}

bool Variant::format(const char* fmt, ...)
{
    va_list va;
    std::string outstr;

    if (mData.type == V_NULL)
    {
        return false;
    }

    va_start(va, fmt);
    bool ret = vformat(outstr, fmt, va);
    va_end(va);
    if (!ret)
    {
        return false;
    }
    assignStr(outstr);
    return true;
}

int64_t Variant::makeInt() const
{
    switch (mData.type)
    {
        case V_INT:
            return mData.intData;

        case V_BOOL:
            return (int64_t)mData.boolData;

        case V_DOUBLE:
            return (int64_t)mData.dblData;

        case V_STRING:
        {
            std::string* strdata = mData.strData();
            char* end;
            errno = 0;
            int64_t value = strtoull(strdata->c_str(), &end, 10);
            if ((errno != 0 && value == 0) || (*end != '\0'))
            {
                value = 0;
            }
            return value;
        }

        default:
            return 0;
    }
}

double Variant::makeDbl() const
{
    switch (mData.type)
    {
        case V_INT:
            return (double)mData.intData;

        case V_BOOL:
            return (double)mData.boolData;

        case V_DOUBLE:
            return mData.dblData;

        case V_STRING:
        {
            std::string* strdata = mData.strData();
            char* end;
            double value = strtod(strdata->c_str(), &end);
            if (*end != '\0')
            {
                value = 0.0;
            }
            return value;
        }

        default:
            return 0.0;
    }
}


void Variant::makeString(StrBld& s, int level, bool json)
{
    switch (mData.type)
    {
        case V_STRING:
        {
            s.clear();
            s.append( *(mData.strData()) );

            if (json)
            {
                jsonifyStr(s);
            }
        }
        break;

        case V_INT:
        {
            s.clear();
            s.append(std::to_string(mData.intData));
        }
        break;

        case V_PTR:
        {
            s.clear();
            s.appendFmt("%llx", (uint64)mData.ptrData);
        }
        break;

        case V_DOUBLE:
        {
            s.clear();
            s.appendFmt("%lg", mData.dblData);
        }
        break;

        case V_BOOL:
        {
            s.clear();
            s.append(mData.boolData ? "true" : "false");
        }
        break;

        case V_EMPTY:
        {
            s.clear();
            if (json)
            {
                s.append("null");
            }
        }
        break;

        case V_NULL:
        {
            s.clear();
            if (json)
            {
                s.append("null");
            }
        }
        break;

        case V_ARRAY:
        {
            s.appendc('[');
            level++;
            for (Iter<Variant> i; forEach(i); )
            {
                if (i.pos() != 0)
                {
                    s.appendc(',');
                }

                appendNewline(s, level, json);

                appendQuote(s, i->type());

                StrBld tmps;
                i->makeString(tmps, level, json);

                s.append(tmps);
                appendQuote(s, i->type());

            }
            level--;
            appendNewline(s, level, json);
            s.appendc(']');
        }
        break;

        case V_OBJECT:
        {
            s.appendc('{');

            level++;
            for (Iter<Variant> i; mData.objectData->forEach(i); )
            {
                if (i.pos() != 0)
                {
                    s.append(",");
                }
                appendNewline(s, level, json);
                appendQuote(s, V_STRING);

                if (json)
                {
                    StrBld keys(i.key());
                    jsonifyStr(keys);
                    s.append(keys);
                }
                else
                {
                    s.append(i.key());
                }
                appendQuote(s, V_STRING);

                s.appendc(':');

                appendQuote(s, i->type());

                StrBld tmps;
                i->makeString(tmps, level, json);

                s.append(tmps);
                appendQuote(s, i->type());

            }
            level--;
            appendNewline(s, level, json);
            s.appendc('}');
        }
        break;

        default:
        {
            dbgerr("TODO: makeString not handled for type %d\n", mData.type);
        }
    }

    level--;
}

void Variant::makeYamlString(StrBld& s, int level, Variant::Type parenttype)
{
    switch (mData.type)
    {
        case V_STRING:
        {
            std::string* ps = mData.strData();

            if (ps->find('\n') != std::string::npos)
            {
                s.append("|");
                appendNewlineYaml(s, level);
                for (size_t i = 0; i < ps->length(); i++)
                {
                    char c = (*ps)[i];
                    if (c == '\n')
                    {
                        appendNewlineYaml(s, level);
                    }
                    else
                    {
                        s.appendc(c);
                    }
                }
            }
            else
            {
                s.append(*ps);
            }
        }
        break;

        case V_INT:
        {
            s.append(std::to_string(mData.intData));
        }
        break;

        case V_DOUBLE:
        {
            s.appendFmt("%lg", mData.dblData);
        }
        break;

        case V_PTR:
        {
            s.appendFmt("%llx", (uint64)mData.ptrData);
        }
        break;

        case V_BOOL:
        {
            s.append(mData.boolData ? "true" : "false");
        }
        break;

        case V_NULL:
        case V_EMPTY:
        {
            s.append("null");
        }
        break;

        case V_ARRAY:
        {
            for (Iter<Variant> i; forEach(i); )
            {
                appendNewlineYaml(s, level);
                s.append("- ");

                StrBld tmps;
                level++;
                i->makeYamlString(tmps, level, V_ARRAY);
                level--;
                s.append(tmps);
            }
        }
        break;

        case V_OBJECT:
        {
            for (Iter<Variant> i; forEach(i); )
            {
                s.append(i.key());
                s.append(": ");

                level++;
                if (i->isObject())
                {
                    appendNewlineYaml(s, level);
                }

                StrBld tmps;
                i->makeYamlString(tmps, level, V_OBJECT);
                s.append(tmps);

                level--;

                // if (mData.objectData->length() > 1)
                if ((i.pos() + 1) < mData.objectData->length())
                {
                    appendNewlineYaml(s, level);
                }
            }
        }
        break;

        default:
        {
            dbgerr("TODO: makeYamlString not handled for type %d\n", mData.type);
        }
    }

    level--;
}


void Variant::jsonifyStr(StrBld& s)
{
    StrBld ts(s.length());

    // First check if the string needs any esc chars
    bool escneeded = false;
    const char* p = s.c_str();
    for (int i = 0; i < (int)s.length(); i++)
    {
        uint32_t c = (unsigned char)p[i];
        if (c >= 0x80 || (c != '/' && strfind(ESCAPE_CHARS, (char)c, NULL)))
        {
            escneeded = true;
            break;
        }
    }
    if (!escneeded)
    {
        return;
    }

    // Build a new string replacing with json style esc chars
    p = s.c_str();
    for (int i = 0; i < (int)s.length(); i++)
    {
        uint32_t c = (unsigned char)p[i];

        if (c >= 0x80)
        {
            int lused = 0;
            c = makeUnicode(&p[i], (int)s.length() - i, &lused);

            ts.appendFmt("\\u%04X", c);
            i += lused - 1;
        }
        else
        {
            // If it is an esc char, we want to escape with a proper code.
            // NOTE: We make an exception for / here even though spec calls for it to be escaped
            size_t pos;
            if (c != '/' && strfind(ESCAPE_CHARS, (char)c, &pos))
            {
                c = ESCAPE_CODES[pos];
                ts.appendc('\\');
                ts.appendc((char)c);
            }
            else
            {
                ts.appendc((char)c);
            }
        }
    }

    s.moveFrom(ts);
}

void Variant::createArray(const char* initvalue /*= 0*/)
{
    if (deleteData())
    {
        if (initvalue == NULL)
        {
            mData.type = V_ARRAY;
            mData.arrayData = new ObjArray();
        }
        else
        {
            JsonParser json;
            json.parseInternal(*this, initvalue, strlen(initvalue),
                JsonParser::FLAG_FLEXQUOTES | JsonParser::FLAG_ARRAYONLY, nullptr);
        }
     }
    else
    {
        dbgerr("createArray() failed\n");
    }
}

void Variant::append(const Variant& elem)
{
    if (isArray())
    {
        mData.arrayData->append(elem);
    }
    else
    {
        dbgerr("Can't push/append, not an array\n");
    }
}

Variant Variant::pop()
{
    Variant ret;

    if (isArray())
    {
        if (mData.arrayData->length() > 0)
        {
            ret = mData.arrayData->get(mData.arrayData->length() - 1);
            mData.arrayData->remove(mData.arrayData->length() - 1);
        }
    }
    else
    {
        dbgerr("Can't pop, not an array\n");
    }

    return ret;
}


void Variant::createObject(const char* initjson /* = NULL*/)
{
    if (deleteData())
    {
        if (initjson == NULL)
        {
            mData.type = V_OBJECT;
            mData.objectData = new PropArray();
        }
        else
        {
            JsonParser json;
            json.parseInternal(*this, initjson, strlen(initjson),
                JsonParser::FLAG_FLEXQUOTES | JsonParser::FLAG_OBJECTONLY, nullptr);
        }
    }
    else
    {
        dbgerr("createObject() failed\n");
    }
}

Variant& Variant::setProp(const char* key, const Variant& value /*= VEMPTY */)
{
    assert(key);
    if (mData.type != V_OBJECT)
    {
        dbglog("setProperty(%s) failed -- not an object\n", key);
        return VNULL;
    }

    return mData.objectData->addOrModify(key, value);
}

bool Variant::removeProp(const char* key)
{
    assert(key);
    if (mData.type == V_OBJECT)
    {
        if (mData.objectData->remove(key))
        {
            return true;
        }
    }
    return false;
}

bool Variant::hasKey(const char* key)
{
    assert(key);
    if (mData.type == V_OBJECT)
    {
        return mData.objectData->exists(key);
    }
    return false;
}

const char* Variant::getKey(size_t pos) const
{
    if (mData.type == V_OBJECT)
    {
        return mData.objectData->getKey(pos);
    }
    return nullptr;
}

Variant& Variant::operator[](const char* key)
{
    assert(key);
    if (mData.type == V_OBJECT)
    {
        return mData.objectData->get(key);
    }
    else if (mData.type != V_NULL && mData.type != V_EMPTY)
    {
        dbglog("[%s] failed--not an object\n", key);
    }
    return VNULL;
}

const Variant& Variant::operator[](const char* key) const
{
    assert(key);
    if (mData.type == V_OBJECT)
    {
        return mData.objectData->get(key);
    }
    else if (mData.type != V_NULL && mData.type != V_EMPTY)
    {
        dbglog("[%s] failed--not an object\n", key);
    }
    return VNULL;
}

Variant& Variant::operator[](size_t i)
{
    if (mData.type == V_ARRAY)
    {
        return mData.arrayData->get(i);
    }
    else if (mData.type == V_OBJECT)
    {
        return mData.objectData->get(i);
    }
    else if (mData.type != V_NULL && mData.type != V_EMPTY)
    {
        dbgerr("Failed--not an object or array\n");
    }
    return VNULL;
}

Variant& Variant::objPath(strparam pathkey, char delim)
{
    Variant* v = this;

    if (!pathkey.empty())
    {
        base::Splitter sp(pathkey, delim);
        while (!sp.eof())
        {
            std::string key = sp.getStr();
            if (v->isObject())
            {
                v = &((*v)[key.c_str()]);
            }
        }
    }
    return *v;
}

const char* Variant::typeName() const
{
    switch (mData.type)
    {
        case V_EMPTY: return "empty";
        case V_NULL: return "null";
        case V_INT: return "int";
        case V_PTR: return "ptr";
        case V_BOOL: return "bool";
        case V_DOUBLE: return "double";
        case V_ARRAY: return "array";
        case V_OBJECT: return "object";
        default: return nullptr;
    }
}

size_t Variant::length() const
{
   if (mData.type == V_ARRAY)
    {
        return mData.arrayData->length();
    }
    else if (mData.type == V_OBJECT)
    {
        return mData.objectData->length();
    }
    else if (mData.type == V_STRING)
    {
        return mData.strData()->length();
    }
    return 1;
}

bool Variant::deleteData()
{
    // Note: this check is important.  Nothing can be written to NULL.  Any caller
    // trying to write to variant first will call this function.  If the call fails
    // the caller will not write data.
    if (mData.type == V_NULL)
    {
        return false;
    }

    switch (mData.type)
    {
        case V_STRING:
        {
            std::string* strdata = mData.strData();

            // Call the string destructor on the inplace newed object.
            using namespace std;
	        strdata->std::string::~string();
        }
        break;

        case V_ARRAY:
        {
            delete mData.arrayData;
            mData.arrayData = NULL;
        }
        break;

        case V_OBJECT:
        {
            delete mData.objectData;
            mData.objectData = NULL;
        }
        break;

        default:
        break;
    }
    mData.type = V_EMPTY;
    return true;
}


void Variant::copyFrom(const Variant* src)
{
    if (this != src)
    {
        if (!deleteData())
        {
            return;
        }

        mData.type = src->mData.type;
        switch (src->mData.type)
        {
            case V_INT:
                mData.intData = src->mData.intData;
                break;

            case V_PTR:
                mData.ptrData = src->mData.ptrData;
                break;

            case V_BOOL:
                mData.boolData = src->mData.boolData;
                break;

            case V_DOUBLE:
                mData.dblData = src->mData.dblData;
                break;

            case V_STRING:
                // Inplace new a std::string object
                new (&mData.strMemData) std::string(*(src->mData.strData()));
                break;

            case V_ARRAY:
                // Create the array object using the copy constructor.
                mData.arrayData = new ObjArray(*(src->mData.arrayData));
                break;

            case V_OBJECT:
                // Create the proparray object using the copy constructor.
                mData.objectData = new PropArray(*(src->mData.objectData));
                break;

            case V_EMPTY:
            case V_NULL:
                // If a NULL or EMPTY is being copied, we set the data type of src to EMPTY.
                // Setting it to NULL is not good because further assignments will fail.
                mData.type = V_EMPTY;
                break;

            default:
                dbgerr("TODO: copyfrom missing %d\n", src->mData.type);
        }
    }
}

void Variant::assignStr(const char* src)
{
    if (mData.type == V_STRING)
    {
        mData.strData()->assign(src);
    }
    else
    {
        if (deleteData())
        {
            mData.type = V_STRING;
            new (&mData.strMemData) std::string(src);
        }
    }
}

void Variant::assignInt(int64_t src)
{
    if (mData.type == V_INT)
    {
        mData.intData = src;
    }
    else
    {
        if (deleteData())
        {
            mData.type = V_INT;
            mData.intData = src;
        }
    }
}

void Variant::assignPtr(void* src)
{
    if (mData.type == V_PTR)
    {
        mData.ptrData = src;
    }
    else
    {
        if (deleteData())
        {
            mData.type = V_PTR;
            mData.ptrData = src;
        }
    }
}

void Variant::assignDbl(double src)
{
    if (mData.type == V_DOUBLE)
    {
        mData.dblData = src;
    }
    else
    {
        if (deleteData())
        {
            mData.type = V_DOUBLE;
            mData.dblData = src;
        }
    }
}

void Variant::assignBool(bool src)
{
    if (mData.type == V_BOOL)
    {
        mData.boolData = src;
    }
    else
    {
        if (deleteData())
        {
            mData.type = V_BOOL;
            mData.boolData = src;
        }
    }
}

std::string Variant::toString() const
{
    if (mData.type == V_STRING)
    {
        return *(mData.strData());
    }
    else
    {
        StrBld sb;
        ((Variant*)this)->makeString(sb, 0, false);
        return sb.toString();
    }
}

bool Variant::forEach(Iter<Variant>& iter)
{
    if (mData.type == V_ARRAY)
    {
        return mData.arrayData->forEach(iter);
    }
    else if (mData.type == V_OBJECT)
    {
        return mData.objectData->forEach(iter);
    }
    return false;
}

bool Variant::readJsonFile(const char* filename)
{
    JsonParser json;

    if (!json.readFile(filename))
    {
        dbgerr("Failed to read json file: %s\n", filename);
        return false;
    }

    json.parse(*this);
    if (json.failed())
    {
        dbgerr("Failed to parse json file\n");
        clear();
        return false;
    }
    return true;
}


bool Variant::readYamlFile(const char* filename)
{
    YamlParser yaml;

    if (!yaml.readFile(filename))
    {
        dbgerr("Failed to read yaml file: %s\n", filename);
        return false;
    }

    yaml.parse(*this);
    if (yaml.failed())
    {
        dbgerr("Failed to parse yaml file\n");
        clear();
        return false;
    }
    return true;
}


// Variant::PropArray::

Variant& Variant::PropArray::addOrModify(const char* keyname, const Variant& value)
{
    PropKeyStr k;
    k.set(keyname);

    auto i = mDataMap.find(k);
    if (i != mDataMap.end())
    {
        // Already exists, modify the value
        i->second = value;
        return i->second;
    }
    else
    {
        // Not found, add it
        mOrdered.push_back(k);
        return mDataMap[k] = value;
    }
}

bool Variant::PropArray::remove(const char* keyname)
{
    PropKeyStr k;
    k.set(keyname);

    auto i = mDataMap.find(k);
    if (i == mDataMap.end())
    {
        return false;
    }

    mDataMap.erase(i);
    for (auto j = mOrdered.begin(); j != mOrdered.end();)
    {
        if (strcmp(j->get(), k.get()) == 0)
        {
            j = mOrdered.erase(j);
            break;
        }
        else
        {
            j++;
        }
    }
    return true;
}

Variant& Variant::PropArray::get(const char* key)
{
    PropKeyStr k;
    k.set(key);

    auto i = mDataMap.find(k);
    if (i != mDataMap.end())
    {
        return i->second;
    }
    return VNULL;
}

Variant& Variant::PropArray::get(size_t i)
{
    if (i < length())
    {
        return get(mOrdered[i].get());
    }
    return VNULL;
}

const char* Variant::PropArray::getKey(size_t i)
{
    if (i < length())
    {
        return mOrdered[i].get();
    }
    return nullptr;
}

bool Variant::PropArray::exists(const char* key)
{
    PropKeyStr k;
    k.set(key);
    auto i = mDataMap.find(k);
    return (i != mDataMap.end());
}

bool Variant::PropArray::forEach(Iter<Variant>& iter)
{
    iter.mPos++;
    if (iter.mPos < mOrdered.size())
    {
        Variant& obj = get(iter.mPos);
        iter.mObj = &obj;
        iter.mKey = mOrdered.at(iter.mPos).get();
        return true;
    }
    return false;
}


} // base
