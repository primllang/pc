#pragma once

#include <list>
#include <unordered_map>
#include <limits.h>
#include "var.h"
#include "cmdline.h"
#include "parse.h"
#include "tables.h"
#include "config.h"
#include "persist.h"

#include "lits.h"

class PlUnit;
class PlEntity;
class PlSymbolTable;
class PlCompState;
class PlTypeInfo;

typedef PlEntity* EntityType;

class PlErrs
{
public:
    PlErrs() :
        mCtxErrCnt(0)
    {
    }
    bool hasErrs()
    {
        return !mErrs.empty();
    }
    void begCtx(base::Path ctxfn)
    {
        mCtxFn = ctxfn;
        mCtxErrCnt = 0;
    }
    size_t endCtx()
    {
        mCtxFn.clear();
        return mCtxErrCnt;
    }
    void add(EntityType en, std::string msgstr);
    void add(EntityType en, const char* fmt, ...);

    void clearErrors()
    {
        mErrs.clear();
    }
    void print();
private:
    base::List<std::string> mErrs;
    base::Path mCtxFn;
    size_t mCtxErrCnt;
};


class PlDiscern
{
public:
    // isXXX functions
    bool isKeyword(strparam str)
    {
        return sKeywordMap.exists(str);
    }
    bool isSimpleIdent(strparam str);
    bool isCompoundIdent(strparam str);
    bool isDigit(char c)
    {
        return (c >='0' && c <= '9');
    }
    bool isNumPart(const std::string& s);
    bool isNumStart(const std::string& s)
    {
        return (!s.empty() && isDigit(s[0]));
    }
    bool isStrLit(const std::string& s);
    bool isVal(strparam str, strparam val)
    {
        return base::streql(str, val);
    }
    bool isAssignOp(strparam str)
    {
        return sAssignOpsMap.exists(str.data());
    }
    bool isExprOp(strparam str)
    {
        return sExprOpsMap.exists(str.data());
    }
    bool isDollarOp(strparam str)
    {
        return str == "$";
    }
    bool isValueType(strparam str)
    {
        DataType dt;
        if (sDataTypeMap.fromString(str, &dt))
        {
            return dt != DataType::d_object && dt != DataType::d_none;
        }
        return false;
    }
};


class PlToken
{
public:
    static PlToken sNilTok;

    PlToken()
    {
        zero();
    }
    void clear()
    {
        zero();
    }
    void setStr(const char* str, size_t len)
    {
        mStr.set(str, len);
    }
    void setStr(const char* str)
    {
        mStr.set(str);
    }
    const size_t length() const
    {
        return strlen(mStr.get());
    }
    const char* cstr() const
    {
        return mStr.get();
    }
    std::string toString() const
    {
        return std::string(mStr.get());
    }
    std::string strNoQuotes(bool allowsingle) const;
    bool isEmpty() const
    {
        const char* s = mStr.get();
        return s[0] == '\0';
    }
    bool equals(const char* s) const
    {
        //dbglog("PrToken-Equals: %s %s\n", toString().c_str(), s);
        return strcmp(s, mStr.get()) == 0;
    }
    bool equals(char c) const
    {
        const char* s = mStr.get();
        return s[0] == c && s[1] == '\0';
    }
    bool isComment() const
    {
        return isFlagSet(mFlags, PlToken::FLAG_CPPCOMMENT) || isFlagSet(mFlags, PlToken::FLAG_CCOMMENT);
    }

public:
    enum Flags
    {
        FLAG_EOL = 0x1,
        FLAG_NL = 0x2,
        FLAG_CCOMMENT = 0x4,
        FLAG_CPPCOMMENT = 0x8
    };

    int32_t mFlags;
    int32_t mBegLine;
    int32_t mBegColumn;

private:
    typedef base::FixedStr<16> TokStr;

    TokStr mStr;

    void zero()
    {
        mStr.clear();
        mFlags = 0;
        mBegLine = 0;
        mBegColumn = 0;
    }
};


class PlTokenParser
{
public:
    PlTokenParser() :
        mCurIndex(0)
    {
    }
    bool tokenize(const std::string& srcfn, bool removecomments = true);

    // Returns true if tokens are available
    bool avail()
    {
        return mCurIndex < mTokens.size() && !mParser.failed();
    }
    bool availForLine(size_t line)
    {
        return mCurIndex < mTokens.size() && !mParser.failed() && mTokens[mCurIndex].mBegLine == line;
    }
    size_t curLine()
    {
        return mTokens[mCurIndex].mBegLine;
    }
    void expectLine(size_t lin);

    // Advance to next token
    bool advance()
    {
        if (avail())
        {
            mCurIndex++;
            return true;
        }
        return false;
    }
    // Move back to prev token
    bool goBack()
    {
        if (mCurIndex > 0 && !mParser.failed())
        {
            mCurIndex--;
            return true;
        }
        return false;
    }
    // Returns a reference to current token or null
    const PlToken& token()
    {
        return avail() ? mTokens[mCurIndex] : PlToken::sNilTok;
    }
    // Returns true if current token matches
    bool is(const char* tokstr)
    {
        //dbglog("is %s %s?\n", token().toString().c_str(), tokstr);
        return token().equals(tokstr);
    }
    bool isAtEOL()
    {
        return isFlagSet(token().mFlags, PlToken::FLAG_EOL);
    }
    bool isAtNL()
    {
        return isFlagSet(token().mFlags, PlToken::FLAG_NL);
    }
    bool isComment()
    {
        return token().isComment();
    }

    // Expect current token to be on new line. Else error.
    void expectNL();
    // Matches the current token, if matches advances. Else error.
    bool advance(const char* tokstr);

    // Entity
    EntityType createEn(EntityType parent, EKind ekind, ETag etag = ETag::Primary);
    EntityType createNilEn(EntityType parent, EKind ekind, ETag etag = ETag::Primary);
    EntityType createPubEn(EntityType parent, EKind ekind, ETag etag, bool pub);
    EntityType createNilPubEn(EntityType parent, EKind ekind, ETag etag, bool pub);
    EntityType createSingleEn(EntityType parent, EKind ekind, ETag etag = ETag::Primary);

    void pushErr(const char* fmt, ...);
    bool inErr()
    {
        return mParser.failed();
    }

    size_t tokIndex()
    {
        return mCurIndex;
    }

    void dumpTokens(base::StrBld& bld);
    void dbgTok(const char* label);
    void dbgDumpTokens();

public:
    class Checkpoint
    {
    public:
        Checkpoint() = delete;
        Checkpoint(PlTokenParser* parser) :
            mParser(parser)
        {
            save();
        }
        ~Checkpoint() = default;
        void save()
        {
            mParser->saveState(this);
        }
        void restore()
        {
            mParser->restoreState(this);
        }
    private:
    friend class PlTokenParser;
        PlTokenParser* mParser;
        size_t mIndex;
        bool mError;
    };

private:
    class BaseParser : public base::Parser
    {
    public:
        BaseParser() :
            mNL(true)
        {
        }
        void initBase(const char* source, size_t len)
        {
            Parser::initParser(source, len);
            enableSinglePunc(true);
        }
        bool readToken(PlToken& tok);
        void combinePunc();
        bool failed()
        {
            return Parser::failed();
        }
    private:
        bool mNL;
    };

    void saveState(Checkpoint* chk)
    {
        //TODO: error state
        chk->mIndex = mCurIndex;
    }
    void restoreState(Checkpoint* chk)
    {
        if (chk->mIndex < mTokens.size())
        {
            mCurIndex = chk->mIndex;
        }
    }

private:
    BaseParser mParser;
    std::vector<PlToken> mTokens;
    std::string mSrcFn;
    size_t mCurIndex;
};


class Code : public base::StrBld
{
public:
    Code() :
        mAtNL(true),
        mIndent(0)
    {
    }
    void emit(std::string_view txt, bool autosp = true);
    void emitln(std::string_view txt)
    {
        emit(txt, false);
        nl();
    }
    void emitFmt(const char* fmt, ...);
    void emit(CppKeyword kw)
    {
        emit(sCppKeywordMap.toString(kw));
        sp();
    }
    void emit(Code& code)
    {
        append(code);
    }
    void semiln()
    {
        emitln(";");
    }
    void sp()
    {
        emit(" ", false);
    }
    void nl()
    {
        emit(LINE_END, false);
        mAtNL = true;
    }
    void indentInc()
    {
        mIndent += C_INDENT;
    }
    void indentDec()
    {
        mIndent = (mIndent > C_INDENT) ? (mIndent - C_INDENT) : 0;
    }
    void braceOpen();
    void braceClose(bool autonl);

private:
    bool mAtNL;
    size_t mIndent;
};


template <class T>
class PlArr : public base::Buffer
{
public:
    PlArr() :
        mCount(0)
    {
    }
    PlArr(size_t count) :
        mCount(count)
    {
        if (count > 0)
        {
            alloc(count * sizeof(T));
            zero();
        }
    }
    size_t count()
    {
        return mCount;
    }
    void set(size_t index, T e)
    {
        if (index < mCount)
        {
            ((T*)ptr())[index] = e;
        }
    }
    T get(size_t index)
    {
        if (index < mCount)
        {
            return ((T*)ptr())[index];
        }
        else
        {
            return T{0};
        }
    }
    void append(T e)
    {
        size_t cursize = size();
        if (mCount == (cursize / sizeof(T)))
        {
            if (cursize == 0)
            {
                cursize = sizeof(T);
            }
            reAlloc(cursize * 2);
        }
        ((T*)ptr())[mCount++] = e;
    }

private:
    size_t mCount;
};


class PlEntity
{
public:
    PlToken mToken;
    EAttribFlags mAttribFlags;
    EntityType mResolvedRef;
    PlUnit* mUnit;
    EKind mKind;
    DataType mDT;

public:
    PlEntity() :
        mResolvedRef(nullptr),
        mUnit(nullptr),
        mKind(EKind::None),
        mDT(DataType::d_none)
    {
    }
    ~PlEntity() = default;
    NOCOPY(PlEntity)

    static EntityType newEntity(EntityType parent, EKind ekind, const PlToken& tok, ETag tg = ETag::Primary);
    static EntityType newOrFindEntity(EntityType parent, const char* tokstr, EKind ekind, ETag tg = ETag::Primary);
    static EntityType cloneEntity(EntityType fromen, EntityType parent, ETag tg = ETag::Primary);

    void setTokStr(const std::string& s)
    {
        mToken.setStr(s.c_str());
    }
    void updateKind(EKind kind)
    {
        mKind = kind;
    }
    const char* getKindStr()
    {
        return sEKindMap.toString(mKind);
    }
    std::string getIdentStr(ETag tg)
    {
        // Get a concatenated string for child entities tagged with tg
        return getStrings(EKind::Ident, tg, ".");
    }
    std::string getSimpIdentStr(ETag tg)
    {
        return getChildStr(EKind::Ident, tg);
    }
    std::string getStr()
    {
        // Gets string of the entity
        return mToken.toString();
    }
    std::string getChildStr(EKind ekind, ETag tg)
    {
        // Gets string only a single child... don't use if multiple are expected.
        return getStrings(ekind, tg, " ", 1);
    }
    void getChildrenStr(EKind ekind, ETag tg, std::vector<std::string>& strs);
    std::string getCppDirecStr(CppDirecType cdtype, size_t index = 0);

    EntityType getChild(ETag tg = ETag::Primary);
    EntityType getChild(EKind ek, ETag tg = ETag::Primary);
    PlArr<EntityType> getChildren(ETag tg = ETag::Primary);
    PlArr<EntityType> getChildren(EKind ek, ETag tg = ETag::Primary);
    std::string getStrings(EKind ekind, ETag tg, std::string sep, size_t max = UINT_MAX);

    void addAttrib(EAttribFlags::etype attr)
    {
        mAttribFlags.set(attr);
    }
    bool hasAttrib(EAttribFlags::etype attr)
    {
        return mAttribFlags.isSet(attr);
    }
    bool isPublic()
    {
        return hasAttrib(EAttribFlags::a_public);
    }
    void saveEntity(base::PersistWr& pwr);
    void gatherPubs(EntityType parent, ETag tg, bool nonpub, PlErrs& errs);
    EntityType createResol(EntityType resolref, const char* tokstr, ETag tg = ETag::Primary);
    void updateSymTbl(ETag tg, PlSymbolTable* symtable, PlUnit* unit);

    void toVar(base::Variant& var, bool pubonly);
    void dump(const std::string& sp, ETag tg, base::StrBld& bld);
    void dbgDump(const char* label);

private:
friend class PlFileState;
friend class PlResolver;

    void dumpAttrib(base::StrBld& bld);
    void write(base::PersistWr& pwr);
    static EntityType read(EntityType parent, base::PersistRd& prd, ETag etag);
    static void dumpEn(EntityType en, ETag tg, base::StrBld& bld);

    struct SubEntity
    {
        ETag mTag;
        std::list<PlEntity> mEntities;
    };
    std::list<SubEntity> mSubEntities;
};


class PlResolver
{
public:
    PlResolver() :
        mSymTable(nullptr),
        mUnit(nullptr)
    {
    }
    void init(PlSymbolTable* symtable, PlUnit* unit)
    {
        mSymTable = symtable;
        mUnit = unit;
    }
    void fixupAll(EntityType rentity);
    void validateAll(EntityType rentity, ETag tg);
    void pushErr(EntityType en, const char* fmt, ...);
    PlTypeInfo getType(EntityType rentity);
    static EntityType createSymScope(EntityType en, strparam classname);

private:
friend class PlTypeInfo;

    PlSymbolTable* mSymTable;
    PlUnit* mUnit;

    void resoFuncCall(EntityType rentity, ETag tg, EntityType prev);
    void resoForStmt(EntityType rentity, ETag tg, EntityType prev);
    void resoImpl(EntityType rentity, ETag tg, EntityType prev);
    void resoDeref(EntityType rentity, ETag tg, EntityType prev);
    void resoVarAssign(EntityType rentity, ETag tg, EntityType prev);
    void resoVarEval(EntityType rentity, ETag tg, EntityType prev);

    void fixupPhase0(EntityType rentity, ETag tg, EntityType prev);
    void fixupPhase1(EntityType rentity, ETag tg, EntityType prev);

    void deterTemplArgs(EntityType templtypeen, std::string& typestr);
    EntityType deterIdent(EntityType symen, ETag tg, std::string& typectx, PlArr<EntityType>* allmatches = nullptr);
    EntityType deterSym(EntityType en, strparam ss);
    void deterType(EntityType rentity, ETag tg);

    void dbgCheckIdent(EntityType rentity, ETag tg);
    void checkReso(EntityType rentity, ETag tg, EKind k1, EKind k2 = EKind::None, EKind k3 = EKind::None);
    void checkFuncCall(EntityType rentity);
    void checkVarAssign(EntityType rentity);

    bool paramsCompat(EntityType cal, EntityType def);
};


class PlTypeInfo
{
public:
    std::string mName;
    DataType mDT;
    EntityType mTypeEn;
    std::string mTemplArg;
    bool mBoxed;

    PlTypeInfo() = delete;
    PlTypeInfo(PlResolver* res) :
        mDT(DataType::d_none),
        mTypeEn(nullptr),
        mResolver(res),
        mBoxed(false)
    {
    }

    bool fromSigCh(EntityType rentity);
    bool fromExpr(EntityType rentity);
    bool isCompat(const PlTypeInfo& that);
    const char* name();
    void dump(base::StrBld& bld);
    void dbgDump(const char* label);

private:
    PlResolver* mResolver;

    ETag sigTag(EntityType en);
    bool isInt(DataType dt);
    bool isStr(DataType dt);
    bool isFloat(DataType dt);
};


class PlSymbolTable
{
public:
    PlSymbolTable() :
        mSpew(false)
    {
    }
    ~PlSymbolTable() = default;
    NOCOPY(PlSymbolTable)

    void init();
    void pushScope(const char* name);
    void popScope();
    EntityType findSymbol(const char* symbol);
    EntityType findSymbol(const char* symbol, std::string* retsc, size_t* count);
    EntityType findSymbolSc(std::string sc, const char* symbol, size_t* count);
    PlArr<EntityType> findAll(std::string sc, const char* symol);
    bool addSymbolSc(std::string sc, const char* symbol, EntityType entity, PlUnit* unit = nullptr);
    bool addSymbol(const char* symbol, EntityType entity, PlUnit* unit = nullptr);
    void dumpSymbols(base::StrBld& bld);
    void dbgDump();
    std::string scCur()
    {
        return mScope;
    }

    static std::string scParent(std::string scope, std::string* child = nullptr);
    static void scAdd(std::string& scope, strparam child);
    static std::string scAtUnitwide(strparam id)
    {
        std::string sc;
        scAdd(sc, L_UNITWIDESCOPE);
        scAdd(sc, id);
        return sc;
    }

public:
    bool mSpew;

private:
    class ScTable
    {
    public:
        bool createScopeSc(const std::string& sc);
        bool createSymbolSc(std::string sc, const char* sym, EntityType en);
        PlArr<EntityType> getAll(std::string sc, const char* sym);
        EntityType getOne(std::string sc, const char* sym);
        size_t count(std::string sc, const char* sym);
        void dump(base::StrBld& bld);

    private:
        std::multimap<std::string, EntityType> mTbl;
    };

    ScTable mTbl;
    std::string mScope;
};


class PlSymbolAg
{
public:
    PlSymbolAg() = delete;
    PlSymbolAg(bool scopeonly, PlSymbolTable* symtable, PlUnit* unit) :
        mSymTable(symtable),
        mUnit(unit),
        mNeedPop(false),
        mScopeOnly(scopeonly)
    {
    }
    ~PlSymbolAg();
    void process(EntityType en, const char* name);
    void addSym(EntityType en, const char* symname);
    static std::string getSymName(EntityType en);

private:
    PlSymbolTable* mSymTable;
    PlUnit* mUnit;
    bool mNeedPop;
    bool mScopeOnly;
};


class PlFileState
{
public:
    PlFileState() :
        mState(State::None),
        mCompState(nullptr),
        mContainingUnit(nullptr),
        mEnRoot(nullptr),
        mDiag(false),
        mDepEn(nullptr)
    {
    }
    NOCOPY(PlFileState)
    ~PlFileState()
    {
        delete mEnRoot;
        delete mDepEn;
        mEnRoot = nullptr;
    }
    void init(PlCompState* compstate, PlUnit* unit, base::Path srcname, bool diag)
    {
        assert(compstate && unit);

        mCompState = compstate;
        mContainingUnit = unit;
        mSrcName = srcname;
        mDiag = diag;
    }

    bool compileSrc(const base::Path& sourcefn);
    bool loadPrecomp(const base::Path& entfn);

    bool generateFile(OutputFmt fmt, const base::Path& filename, base::Buffer* outbuf = nullptr);
    void genDiagInfo(base::StrBld& bld);

    PlArr<EntityType> getRootChildren(EKind ek, ETag tg = ETag::Primary);

    PlSymbolTable* symTable();
    EntityType enRoot()
    {
        return mEnRoot;
    }
    PlUnit* containingUnit()
    {
        return mContainingUnit;
    }
    base::Path fname()
    {
        return mSrcName;
    }
    void readCfg(bool multiroot = false);
    bool isMod()
    {
        return mState == State::NeedCompile;
    }
    void setMod()
    {
        mState = State::NeedCompile;
    }
    bool isErr()
    {
        return mState == State::Error;
    }
    void setErr()
    {
        mState = State::Error;
    }

private:
    enum class State
    {
        None,
        NeedCompile,
        Compiled,
        AstLoaded,
        Error
    } mState;

    base::Path mSrcName;
    PlCompState* mCompState;
    PlUnit* mContainingUnit;
    EntityType mEnRoot;
    base::StrBld mTokDiag;
    bool mDiag;

    EntityType loadEntity(EntityType parent, base::PersistRd& prd, ETag etag = ETag::Primary);
    void readCppDirec(EntityType ce);

public:
    EntityType mDepEn;
};


class PlCompState
{
public:
    PlCompState() :
        mUnit(nullptr),
        mPubsRoot(nullptr),
        mDepsRoot(nullptr)
    {
    }
    ~PlCompState()
    {
        delete mPubsRoot;
        delete mDepsRoot;
    }
    void init(PlUnit* unit)
    {
        mUnit = unit;
        mSymTable.init();
        mDepsRoot = PlEntity::newEntity(nullptr, EKind::FileRoot, PlToken::sNilTok);
        mResolver.init(symTable(), mUnit);
    }
    void addSrc(base::Path srcname);
    void addDep(PlUnit* pu);
    PlSymbolTable* symTable()
    {
        return &mSymTable;
    }
    bool compileSrc();
    bool loadUnits();
    void resolveSrcEntitites();
    bool gatherAllPubs();
    bool generateFile(OutputFmt fmt, const base::Path& filename, base::Buffer* outbuf = nullptr);
    bool writeDiagInfo(const base::Path& fn);

public:
    base::List<PlFileState> mSrcFiles;
    base::List<PlFileState> mDepUnits;
    base::List<base::Path> mExtIncDirs;
    EntityType mPubsRoot;
    EntityType mDepsRoot;

private:
    PlSymbolTable mSymTable;
    PlUnit* mUnit;
    PlResolver mResolver;
};


class PlCppProp
{
public:
    PlCppProp();
    std::string getStr(CppPropType keyenum);
    void setStr(CppPropType keyenum, std::string value);
    void setStr(std::string key, std::string value);

    std::string mkBoxName(strparam org)
    {
        return base::formatr("%s<%s>", getStr(CppPropType::boxclass).c_str(), org.data());
    }
    std::string mkClassName(strparam org)
    {
        return base::formatr("_%s_class", org.data());
    }
    std::string mkIntfName(strparam org)
    {
        return base::formatr("_%s_intf", org.data());
    }
    std::string mkWeak(strparam org)
    {
        return base::formatr("_%s_weak", org.data());
    }
    bool convValidNum(std::string& num, DataType& dtype);

private:
    base::Variant mProp;
};
extern PlCppProp sCppProp;


class PlUnit
{
public:
    PlUnit() :
        mInit(false),
        mType(UnitType::none),
        mBldDiagFiles(false),
        mBldTempFiles(false)
    {
    }
    NOCOPY(PlUnit)

    bool init(const base::Path unitpath);
    bool build(BuildConfig bldcfg);
    bool buildExt(BuildConfig bldcfg);
    bool clean(bool hard);
    bool cleanExt(bool hard);
    bool findUnitPath(std::string name, base::Path& path);
    bool writeLogs();
    base::Path target(BuildConfig bldcfg);

    base::Path srcDir()
    {
        return base::Path(mPath, L_SRCDIR);
    }
    base::Path targetDir()
    {
        return base::Path(mPath, L_TARGETDIR);
    }
    base::Path diagDir()
    {
        return base::Path(targetDir(), L_DIAGDIR);
    }
    base::Path buildDir()
    {
        return base::Path(targetDir(), L_BUILDDIR);
    }
    base::Path outDir()
    {
        return base::Path(targetDir(), L_OUTDIR);
    }
    base::Path astDir()
    {
        return base::Path(buildDir(), L_ASTDIR);
    }
    base::Path metaFn()
    {
        return base::Path(mPath, sConfig.getStr(Config::unityamlfn));
    }
    base::Path cmakeFn()
    {
        return base::Path(targetDir(), "CMakeLists.txt");
    }
    base::Path srcFn(const base::Path& fn)
    {
        return base::Path(srcDir(), fn);
    }
    base::Path cppFn(const base::Path& fn)
    {
        base::Path p(targetDir(), fn);
        p.modifyExt(L_CPPEXT);
        return p;
    }
    base::Path cppNameFn(const base::Path& fn)
    {
        base::Path p(fn);
        p.modifyExt(L_CPPEXT);
        return p;
    }
    base::Path privHdrFn(const base::Path& fn)
    {
        base::Path p(targetDir(), fn);
        p.modifyExt(L_HDREXT);
        return p;
    }
    base::Path diagFn(const base::Path& fn)
    {
        base::Path d(fn);
        d.modifyExt("");
        d += "_diag";
        base::Path p(diagDir(), d);
        p.modifyExt(L_DIAFILEEXT);
        return p;
    }
    base::Path unitDiagFn()
    {
        base::Path pub("pub_");
        pub += mName;
        pub += "_diag";

        base::Path p(diagDir(), pub);
        p.modifyExt(L_DIAFILEEXT);
        return p;
    }
    base::Path srcAstFn(const base::Path& fn)
    {
        base::Path p(astDir(), fn);
        p.modifyExt(L_ENTFILEEXT);
        return p;
    }
    base::Path unitAstFn()
    {
        base::Path pub("pub_");
        pub += mName;
        base::Path p(astDir(), pub);
        p.modifyExt(L_ENTFILEEXT);
        return p;
    }
    base::Path unitWideFn()
    {
        return base::Path(targetDir(), sCppProp.getStr(CppPropType::unitwidehdrfn));
    }
    base::Path pubHdrFn()
    {
        base::Path pub("pub_");
        pub += mName;
        base::Path p(targetDir(), pub);
        p.modifyExt(L_HDREXT);
        return p;
    }
    base::Path bldLogFn()
    {
        return base::Path(diagDir(), "unit_bld.log");
    }
    base::Path errLogFn()
    {
        return base::Path(diagDir(), "unit_err.log");
    }
    std::string mkLibFn(std::string name)
    {
        std::string ln;
        if (sConfig.isLinux())
        {
            ln.append("lib");
        }
        ln.append(name);
        if (sConfig.isLinux())
        {
            ln.append(".a");
        }
        else
        {
            ln.append(".lib");
        }
        return ln;
    }
    std::string cppNamespace()
    {
        std::string ns = mName;
        ns += "_pl";
        return ns;
    }

public:
    std::string mName;
    std::string mNS;
    UnitType mType;
    std::string mVer;
    base::Variant mMeta;
    PlCompState mCompState;
    base::Path mPath;
    bool mBldDiagFiles;
    bool mBldTempFiles;
    base::Buffer mLog;
    base::Buffer mErrLog;
    PlErrs mErrCol;

private:
    bool mInit;

    bool getSrcFiles(bool& srcisnew);
    bool writeUnitWideHdr();

    bool writeCmake(bool& cmakeupdated);
    bool getCmakeLibs(PlUnit* depunit, std::string& addlib, std::string& libnames);
    bool makeCmakeLibStr(PlUnit* depunit, base::StrBld& cm, const char* dir, const char* proj, const char* libname, base::Path eincdir);

    bool moveTemps();

    bool existNinjaFiles(base::Path dir);
    bool writeNinjaFiles(base::Path dir);
    bool buildNinja(base::Path dir, BuildConfig bldcfg);
    bool cleanNinja(base::Path dir);

    class DirPush
    {
    public:
        DirPush() = delete;
        DirPush(const base::Path& dir)
        {
            currentDirectory(mPrev);
            changeDirectory(dir);
        }
        ~DirPush()
        {
            changeDirectory(mPrev);
        }
    private:
        base::Path mPrev;
    };
};


class PlBuilder
{
public:
    struct BldOptions
    {
        BuildConfig bldcfg = BuildConfig::Debug;
        bool cleanfirst = false;
        bool cleanhard = false;
        bool diagfiles = false;
        bool checkdeps = false;
        bool savetemps = false;
    };

    bool cleanUnit(base::Path unitpath, bool hard);
    bool newUnit(UnitType utype, base::Path unitpath, bool nogit = false);
    bool buildUnit(base::Path unitpath, BldOptions opts);
    bool runUnit(base::Path unitpath, BldOptions opts);

private:
    PlUnit* getUnitDeps(base::List<PlUnit>& deps, const base::Path& parentpath);
};

bool plParseFile(const base::Path& sourcefn, PlSymbolTable* symtable, PlUnit* containingunit, EntityType root, base::StrBld* diag);
bool plGenerate(PlUnit* unit, EntityType root, OutputFmt fmt, base::Buffer& buf);

bool plReadYaml(const char* content, const base::Path fn, base::Variant& var);
bool plReadJson(const char* content, const base::Path fn, base::Variant& var);
bool plWrite(const char* content, const base::Buffer& buf, const base::Path fn, bool onlywhendiff = false);
bool plRead(const char* content, base::Buffer& buf, const base::Path fn, bool nullterm);
int plLogShell(const std::string& cmd, base::Buffer& log, base::Buffer& errlog);
bool plIsIdent(strparam str);
void plPrintErr(strparam str);

