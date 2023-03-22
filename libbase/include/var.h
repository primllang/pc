#pragma once

#include "util.h"
#include "sys.h"
#include <map>
#include <vector>

#define VNULL  Variant::sNull
#define VEMPTY  Variant::sEmpty

namespace base
{

// Variant class is inspired by Javascript's Var semantics.  It maintains data of different types such
// as ints, doubles, strings.  It also stores objects with properties, arrays of variants.
class Variant
{
    class ObjArray : public std::vector<Variant>
    {
    public:
        void append(const Variant& e)
        {
            push_back(e);
        }
        size_t length()
        {
            return size();
        }
        void remove(size_t pos)
        {
            erase(begin() + pos);
        }
        Variant& get(size_t pos)
        {
            if (pos >= length())
            {
                return VNULL;
            }
            return at(pos);
        }
        bool forEach(Iter<Variant>& iter)
        {
            iter.mPos++;
            if (iter.mPos < length())
            {
                iter.mObj = &get(iter.mPos);
                return true;
            }
            return false;
        }
    };

    class PropArray
    {
    public:
        Variant& addOrModify(const char* keyname, const Variant& value);
        bool remove(const char* keyname);
        size_t length()
        {
            return mOrdered.size();
        }
        Variant& get(const char* key);
        Variant& get(size_t i);
        bool exists(const char* key);
        const char* getKey(size_t i);
        bool forEach(Iter<Variant>& iter);

    private:
        typedef FixedStr<24> PropKeyStr;
        struct KeyComp
        {
            bool operator()(const PropKeyStr& a, const PropKeyStr& b) const
            {
                return strcmp(a.get(), b.get()) < 0;
            }
        };

        std::map<PropKeyStr, Variant, KeyComp> mDataMap;
        std::vector<PropKeyStr> mOrdered;
    };

public:
    enum Type
    {
        V_EMPTY,
        V_NULL,
        V_INT,
        V_BOOL,
        V_DOUBLE,
        V_STRING,
        V_ARRAY,
        V_OBJECT,
        V_PTR
    };

public:
    Variant()
    {
        mData.type = V_EMPTY;
    }
    explicit Variant(int64_t i)
    {
        mData.type = V_INT;
        mData.intData = i;
    }
    explicit Variant(int i)
    {
        mData.type = V_INT;
        mData.intData = i;
    }
    explicit Variant(void* p)
    {
        mData.type = V_PTR;
        mData.ptrData = p;
    }
    Variant(double d)
    {
        mData.type = V_DOUBLE;
        mData.dblData = d;
    }
    Variant(std::string s)
    {
        mData.type = V_STRING;
        new (&mData.strMemData) std::string(s);
    }
    Variant(const char* s)
    {
        mData.type = V_STRING;
        new (&mData.strMemData) std::string(s);
    }
    Variant(Variant const& src)
    {
        mData.type = V_EMPTY;
        *this = src;
    }
    ~Variant()
    {
        (void)deleteData();
    }
    Variant& operator=(const Variant& src)
    {
        copyFrom(&src);
        return *this;
    }
    Variant& operator=(const Variant* src)
    {
        copyFrom(src);
        return *this;
    }
    Variant& operator=(const char* src)
    {
        assignStr(src);
        return *this;
    }
    Variant& operator=(const std::string& src)
    {
        assignStr(src);
        return *this;
    }
    Variant& operator=(bool src)
    {
        assignBool(src);
        return *this;
    }
    Variant& operator=(int64_t src)
    {
        assignInt(src);
        return *this;
    }
    Variant& operator=(int src)
    {
        assignInt((int64_t)src);
        return *this;
    }
    Variant& operator=(double src)
    {
        assignDbl(src);
        return *this;
    }
    Variant& operator=(float src)
    {
        assignDbl((double)src);
        return *this;
    }
    operator int64_t() const
    {
        return makeInt();
    }
    // operator const int64_t()
    // {
    //     return makeInt();
    // }
    operator double() const
    {
        return makeDbl();
    }

    // Typecast the value as an bool
    operator bool() const
    {
        return makeBool();
    }

    // Type cast to a copy of a string
    operator const std::string() const
    {
        return toString();
    }

    operator const std::string_view() const
    {
        return cstr();
    }

    // Returns a pointer to the string.
    // NOTE: Returns "" if not string type.
    const char* cstr() const
    {
        if (mData.type == V_STRING)
        {
            return mData.strData()->c_str();
        }
        return "";
    }

    // Returns a *copy* of the string performing any conversions.  Format is similar to
    // json but does not do any escaping (for proper Json strings, use toJsonString()
    // return Copy of the string
    std::string toString() const;

    std::string toJsonString() const
    {
        StrBld sb;
        ((Variant*)this)->makeString(sb, 0, true);
        return sb.toString();
    }
    void getJson(base::Buffer& buf) const
    {
        StrBld sb;
        ((Variant*)this)->makeString(sb, 0, true);
        sb.moveToBuffer(buf);
    }

    std::string toYamlString() const
    {
        StrBld sb;
        ((Variant*)this)->makeYamlString(sb, 0, V_NULL);
        return sb.toString();
    }
    void getYaml(base::Buffer& buf) const
    {
        StrBld sb;
        ((Variant*)this)->makeYamlString(sb, 0, V_NULL);
        sb.moveToBuffer(buf);
    }

    bool parseJson(const char* jsontxt);

    // Formats a new string using printf style formatting and assigns it to the variant
    bool format(const char* fmt, ...);

    // Returns the variant type V_xxx for the object
    // returns  Variant type
    Type type() const
    {
        return (Type)(mData.type);
    }

    // Returns the variant type of the object as a string
    // returns  String containing type name
    const char* typeName() const;

    // Returns true if the object is empty or null.  It is prefered way
    // to check that the variant doesn't have data.
    bool empty() const
    {
        return (mData.type == V_NULL) || (mData.type == V_EMPTY);
    }

    bool isObject() const
    {
        return (mData.type == V_OBJECT);
    }
    bool isArray() const
    {
        return (mData.type == V_ARRAY);
    }
    bool isString() const
    {
        return (mData.type == V_STRING);
    }

    void setPtrVal(void* src)
    {
        assignPtr(src);
    }
    void* getPtrVal()
    {
        return (mData.type == V_PTR) ? mData.ptrData : nullptr;
    }
    bool isPtrVal()
    {
        return (mData.type == V_PTR);
    }

    // Returns the length.  If an array, returns array length.  If an object, returns
    // the number of properties. If string, returns length of string.  Otherwise, returns 1.
    size_t length() const;

    // Returns a reference to the variant in an array
    Variant& operator[](size_t i);
    // const Variant& operator[](size_t i) const
    // {
    //     return const_cast<Variant*>(this)->operator[](i);
    // }

    // Returns a reference to the variant inside an object or a function using key
    Variant& operator[](const char* key);
    // Variant& operator[](const std::string& key)
    // {
    //     // return this->operator[](key.c_str());
    //     return const_cast<Variant*>(this)->operator[](key);
    // }
    const Variant& operator[](const char* key) const;
    const Variant& operator[](const std::string& key) const
    {
        return const_cast<Variant*>(this)->operator[](key.c_str());
    }
    // Variant const& operator[](char const * key) const
    // {
    //     return const_cast<Variant*>(this)->operator[](key);
    // }

    Variant& objPath(strparam pathkey, char delim);

    // Creates an array
    //  initvalue Optional JSON to initialize the array
    void createArray(const char* initvalue = NULL);

    // Adds a variant at the end of an array
    //   elem Variant to add
    void append(const Variant& elem = VEMPTY);
    void push(const Variant& elem)
    {
        (void)append(elem);
    }

    // Removes the last item from the array and returns it
    // returns  Copy of the last element
    Variant pop();

    // Creates an objects with optional initial JSON value
    void createObject(const char* initjson = NULL);

    Variant& setProp(const char* key, const Variant& value = VEMPTY);
    bool removeProp(const char* key);

    bool hasKey(const char* key);
    const char* getKey(size_t pos) const;

    // Iterator
    bool forEach(Iter<Variant>& iter);

    void clear()
    {
        (void)deleteData();
    }

    bool readJsonFile(const char* filename);
    bool readYamlFile(const char* filename);

private:
    //#pragma pack(push, 4)
    struct VarData
    {
        union
        {
            int64_t intData;
            double dblData;
            bool boolData;
            char strMemData[sizeof(std::string)];
            ObjArray* arrayData;
            PropArray* objectData;
            void* ptrData;
        };
        Type type;

        std::string* strData() const
        {
            assert(type == V_STRING);
            return (std::string*)&strMemData;
        }
    };
//    #pragma pack(pop)

    VarData mData;

private:
    bool deleteData();
    void copyFrom(const Variant* src);

    void assignInt(int64_t src);
    void assignPtr(void* src);
    void assignDbl(double src);
    void assignBool(bool src);
    void assignStr(const char* src);
    void assignStr(const std::string& src)
    {
        assignStr(src.c_str());
    }

    int64_t makeInt() const;
    double makeDbl() const;
    bool makeBool() const
    {
        return makeInt() == 0 ? false : true;
    }
    void makeString(StrBld& s, int level, bool json);
    void makeYamlString(StrBld& s, int level, Variant::Type parenttype);

    void jsonifyStr(StrBld& s);
    void appendQuote(StrBld& s, Type type)
    {
        if (type == V_STRING)
        {
            s.appendc('"');
        }
    }
    void appendNewline(StrBld& s, int level, bool json)
    {
        if (json)
        {
            s.append(LINE_END);
            for (int i = 0; i < level; i++)
            {
                s.appendc('\t');
            }
        }
    }
    void appendNewlineYaml(StrBld& s, int level, bool nl = true)
    {
        if (nl)
        {
            s.append(LINE_END);
        }
        for (int i = 0; i < level; i++)
        {
            s.append("  ");
        }
    }

public:
    static Variant sEmpty;
    static Variant sNull;

    Variant(Type type)
    {
        mData.type = type;
    }
    void internalSetPtr(const Variant* v);
};

// Global operator to compare a variant and string
inline bool operator==(const Variant& lhs, const std::string& rhs)
{
    return lhs.toString() == rhs;
}
inline bool operator!=(const Variant& lhs, const std::string& rhs) { return !(lhs == rhs); }

// Global operator to compare a variant and const char *
inline bool operator==(const Variant& lhs, const char *rhs)
{
    return strcmp(lhs.toString().c_str(), rhs) == 0;
}
inline bool operator!=(const Variant& lhs, const char *rhs) { return !(lhs == rhs); }

template <class CollType>
class IterVariant
{
public:
    IterVariant(CollType& coll, size_t const index) :
        mIndex(index),
        mColl(coll)
    {
    }
    bool operator!= (IterVariant const& that) const
    {
        return mIndex != that.mIndex;
    }
    Variant const & operator* () const
    {
        return mColl[mIndex];
    }
    IterVariant const& operator++ ()
    {
        ++mIndex;
        return *this;
    }

private:
    size_t mIndex;
    CollType& mColl;
};

inline IterVariant<Variant> begin(Variant& coll)
{
    return IterVariant<Variant>(coll, 0);
}

inline IterVariant<Variant> end(Variant& coll)
{
    return IterVariant<Variant>(coll, coll.length());
}

inline IterVariant<Variant const> begin(Variant const & coll)
{
    return IterVariant<Variant const>(coll, 0);
}

inline IterVariant<Variant const> end(Variant const & coll)
{
    return IterVariant<Variant const>(coll, coll.length());
}


} // base

