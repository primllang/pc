#include "primalc.h"

EntityType PlSymbolTable::ScTable::getOne(std::string sc, const char* sym)
{
    // If more than one entries exist, returns the first one
    sc += ".";
    sc += sym;
    auto range = mTbl.equal_range(sc);
    if (range.first != range.second)
    {
        return range.first->second;
    }
    return nullptr;
}

size_t PlSymbolTable::ScTable::count(std::string sc, const char* sym)
{
    sc += ".";
    sc += sym;
    size_t ret = 0;
    auto range = mTbl.equal_range(sc);
    for (auto i = range.first; i != range.second; i++)
    {
        ret++;
    }
    return ret;
}

bool PlSymbolTable::ScTable::createScopeSc(const std::string& sc)
{
    if (mTbl.find(sc) != mTbl.end())
    {
        //dbglog("Cannot add dup scope '%s'\n", sc.c_str());
        return false;
    }
    mTbl.insert(std::make_pair(sc, (EntityType)0));
    return true;
}

bool PlSymbolTable::ScTable::createSymbolSc(std::string sc, const char* sym, EntityType en)
{
    sc += ".";
    sc += sym;
    mTbl.insert(std::make_pair(sc, en));
    return true;
}

PlArr<EntityType> PlSymbolTable::ScTable::getAll(std::string sc, const char* sym)
{
    sc += ".";
    sc += sym;
    PlArr<EntityType> ret;
    auto range = mTbl.equal_range(sc);
    for (auto i = range.first; i != range.second; i++)
    {
        ret.append(i->second);
    }
    return ret;
}

void PlSymbolTable::ScTable::dump(base::StrBld& bld)
{
    for (auto it : mTbl)
    {
        std::string scope = it.first;
        if (scope.back() == '>')
        {
            size_t cnt = 0;
            for (auto jt : mTbl)
            {
                std::string key = jt.first;
                std::string s;
                auto pos = key.find_last_of('.');
                if (pos != key.npos && key.back() != '>')
                {
                    s = key.substr(pos + 1);
                    key = key.substr(0, pos);
                    if (key == scope)
                    {
                        if (cnt == 0)
                        {
                            bld.appendFmt("'%s'\n", scope.c_str());
                        }
                        bld.appendFmt("    '%s': %llx\n", s.c_str(), (void*)(jt.second));
                        cnt++;
                    }
                }
            }

        }
    }
}


// PlSymbolTable::

void PlSymbolTable::init()
{
    pushScope(L_UNITDEPS);
    popScope();
    pushScope(L_UNITWIDESCOPE);
}

void PlSymbolTable::pushScope(const char* name)
{
    scAdd(mScope, name);
    mTbl.createScopeSc(mScope);
    //dbglog("SymbolTable push scope, current: %s\n", mScope.c_str());
}

void PlSymbolTable::popScope()
{
    std::string child;
    mScope = scParent(mScope, &child);
    //dbglog("SymbolTable pop scope, current: %s\n", mScope.c_str());
}

PlArr<EntityType> PlSymbolTable::findAll(std::string sc, const char* symbol)
{
    return mTbl.getAll(sc, symbol);
}

EntityType PlSymbolTable::findSymbolSc(std::string sc, const char* symbol, size_t* count)
{
    EntityType ret = mTbl.getOne(sc, symbol);

    if (count)
    {
        size_t cn = mTbl.count(sc, symbol);
        *count = cn;
        if (mSpew && cn > 1)
        {
            dbglog("%zu dups for symbol '%s' found at sc=%s --returning 1\n", cn, sc.c_str(), symbol);
        }
    }

    if (mSpew)
    {
        dbglog("findSymbol '%s' in scope '%s': %s\n", symbol, sc.c_str(), ret == nullptr ? "NOT FOUND" : "FOUND ->");
        if (ret != nullptr)
        {
            ret->dbgDump("");
        }
    }
    return ret;
}

EntityType PlSymbolTable::findSymbol(const char* symbol)
{
    return findSymbol(symbol, nullptr, nullptr);
}

EntityType PlSymbolTable::findSymbol(const char* symbol, std::string* retsc, size_t* count)
{
    std::string sc = mScope;
    while (!sc.empty())
    {
        EntityType en = findSymbolSc(sc, symbol, count);
        if (en)
        {
            if (retsc)
            {
                *retsc = sc;
            }
            return en;
        }
        sc = scParent(sc);
    }
    return nullptr;
}


bool PlSymbolTable::addSymbol(const char* symbol, EntityType en, PlUnit* unit)
{
    return addSymbolSc(mScope, symbol, en, unit);
}

bool PlSymbolTable::addSymbolSc(std::string sc, const char* symbol, EntityType en, PlUnit* unit)
{
    if (mSpew)
    {
        dbglog("addSymbol '%s' in scope '%s' %llx\n", symbol, sc.c_str(), (uint64)en);
    }

    // TODO: properly implement duplicate names if parameter types are different
    // Currently, we are allowing dups without checking for everything except typedefs
    // check for duplicate at this scopes, and error out if already exists

    auto arr = mTbl.getAll(sc, symbol);
    if (arr.count() > 0 && en->mKind == EKind::TypeDef)
    {
        EntityType preven = arr.get(0);
        std::string prevunit = (preven->mUnit != nullptr) ? preven->mUnit->mName : "";
        unit->mErrCol.add(preven, "Duplicate symbol '%s' from unit '%s' previously added from unit '%s'",
            symbol, unit->mName.c_str(), prevunit.c_str());
        return false;
    }

    mTbl.createSymbolSc(sc, symbol, en);

    if (en)
    {
        // Set the unit in the entity and mark it as symbolic
        en->addAttrib(EAttribFlags::a_symbol);
        en->mUnit = unit;
    }
    return true;
}

void PlSymbolTable::dumpSymbols(base::StrBld& bld)
{
    mTbl.dump(bld);
}

void PlSymbolTable::dbgDump()
{
#ifdef _DEBUG
    base::StrBld bld;
    dumpSymbols(bld);
    dbglog("%s", bld.c_str());
#endif
}

std::string PlSymbolTable::scParent(std::string scope, std::string* child)
{
    size_t pos = scope.find_last_of('.');
    if (pos == std::string::npos)
    {
        return std::string();
    }
    if (child)
    {
        *child = scope.substr(pos + 1);
    }
    return scope.substr(0, pos);
}

void PlSymbolTable::scAdd(std::string& scope, strparam child)
{
    assert(!child.empty());
    if (!scope.empty())
    {
        scope.append(".");
    }
    scope.append(child);
    scope.append(">");
}

// PlSymbolAg::
PlSymbolAg::~PlSymbolAg()
{
    if (mNeedPop)
    {
        mSymTable->popScope();
    }
}

void PlSymbolAg::addSym(EntityType en, const char* symname)
{
    mSymTable->addSymbol(symname, en, mUnit);
}


void PlSymbolAg::process(EntityType en, const char* name)
{
    if (en->mKind == EKind::FuncBody || en->mKind == EKind::Object || en->mKind == EKind::Interf)
    {
        std::string ssname;
        if (name == nullptr || *name == '\0')
        {
            ssname = getSymName(en);
            name = ssname.c_str();
        }

        // Add symbol and push scope
        if (!mScopeOnly)
        {
            mSymTable->addSymbol(name, en, mUnit);

            if (en->mKind == EKind::Object || en->mKind == EKind::Interf)
            {
                // Add/update the symbol scope for this entity
                std::string sc = mSymTable->scCur();
                mSymTable->scAdd(sc, name);

                // Create a symscope for this scope
                en->createResol(nullptr, sc.c_str(), ETag::SymScope);
            }
        }
        mSymTable->pushScope(name);

        if (!mScopeOnly)
        {
            // Add any template arguments at this scope
            auto arr = en->getChildren(ETag::TemplArg);
            for (int i = 0; i < arr.count(); i++)
            {
                EntityType e = arr.get(i);
                mSymTable->addSymbol(e->getStr().c_str(), e, mUnit);
            }
        }

        mNeedPop = true;
    }
    else if (en->mKind == EKind::Impl || en->mKind == EKind::StmtBlock || en->mKind == EKind::ForStmt)
    {
        std::string ssname;
        if (name == nullptr || *name == '\0')
        {
            ssname = getSymName(en);
            name = ssname.c_str();
        }

        // Push scope
        if (*name != '\0')
        {
            mSymTable->pushScope(name);
            mNeedPop = true;
        }
    }
    else if (name && name[0] != '\0')
    {
        if (!mScopeOnly)
        {
            mSymTable->addSymbol(name, en, mUnit);
        }
    }
}

std::string PlSymbolAg::getSymName(EntityType en)
{
    // Gets the appropriate name for en entity based on the kind
    std::string ss;

    if (en)
    {
        switch (en->mKind)
        {
            case EKind::FuncBody:
            case EKind::FuncMapping:
            case EKind::FuncDef:
                ss = en->getIdentStr(ETag::FuncName);
                break;

            case EKind::Object:
            case EKind::Impl:
                ss = en->getIdentStr(ETag::ClassName);
                break;

            case EKind::Interf:
                ss = en->getIdentStr(ETag::InterfName);
                break;

            case EKind::VarDecl:
            case EKind::ObjectFld:
                ss = en->getIdentStr(ETag::VarName);
                break;

            case EKind::TypeDef:
                ss = en->getIdentStr(ETag::TypeName);
                break;

            case EKind::FuncParam:
                ss = en->getIdentStr(ETag::ParamName);
                break;

            case EKind::StmtBlock:
            case EKind::ForStmt:
                ss = en->getStr();
                break;

            default:
                break;
        }
    }
    return ss;
}