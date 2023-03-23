#include "primalc.h"

// PlResolver::

PlTypeInfo PlResolver::getType(EntityType rentity)
{
    PlTypeInfo ty(this);
    if (rentity)
    {
        if (rentity->mKind == EKind::Expr)
        {
            ty.fromExpr(rentity);
        }
        else
        {
            ty.fromSigCh(rentity);
        }
    }
    return ty;
}

EntityType PlResolver::createSymScope(EntityType en, strparam classname)
{
    // Setup the sym scope entity in symboltable which is at unit level for the provided
    // class. ex: "unitwide>.CppString>"
    std::string sc = PlSymbolTable::scAtUnitwide(classname);
    return en->createResol(nullptr, sc.data(), ETag::SymScope);
}

EntityType PlResolver::deterIdent(EntityType symen, ETag tg, std::string& typectx, PlArr<EntityType>* allmatches)
{
    assert(symen);

    typectx.clear();
    auto arr = symen->getChildren(tg);

    std::string nextsc;
    std::string sc;
    EntityType en = nullptr;
    size_t matches = 0;
    std::string matsc;
    for (size_t i = 0; i < arr.count(); i++)
    {
        std::string id = arr.get(i)->getStr();

        bool depscope = false;
        en = nullptr;
        if (i == 0)
        {
            // Find the symbol in current and outer scopes
            en = mSymTable->findSymbol(id.c_str(), &sc, &matches);
            if (!en)
            {
                // Look in deps to see if this is a dep unit namespace
                sc.clear();
                mSymTable->scAdd(sc, L_UNITDEPSSCOPE);
                en = mSymTable->findSymbolSc(sc, id.c_str(), &matches);
                depscope = true;
            }
            if (matches > 1)
            {
                matsc = sc;
            }
        }
        else
        {
            en = mSymTable->findSymbolSc(nextsc, id.c_str(), &matches);
            if (matches > 1)
            {
                matsc = nextsc;
            }
        }

        nextsc.clear();
        if (en)
        {
            // Set the resolved value for this portion of the identifier
            arr.get(i)->mResolvedRef = en;

            // Setup next scope
            if (en->mKind == EKind::VarDecl || en->mKind == EKind::ObjectFld || en->mKind == EKind::FuncParam || en->mKind == EKind::ForStmt)
            {
                // Var or obj field, look up type and setup scope.
                if (en->mResolvedRef)
                {
                    if (en->mResolvedRef->hasAttrib(EAttribFlags::a_methods))
                    {
                        EntityType ss = en->mResolvedRef->getChild(ETag::SymScope);
                        if (ss)
                        {
                            nextsc = ss->mToken.cstr();
                        }

                        // Copy the attributes of the resolved entity
                        if (en->mResolvedRef->hasAttrib(EAttribFlags::a_noderef))
                        {
                            en->addAttrib(EAttribFlags::a_noderef);
                        }
                        if (en->mResolvedRef->hasAttrib(EAttribFlags::a_cppobject))
                        {
                            en->addAttrib(EAttribFlags::a_cppobject);
                        }

                        // Set the type context
                        typectx = en->getSimpIdentStr(ETag::VarType);
                    }
                }
                else
                {
                    if (mSymTable->mSpew)
                    {
                        dbglog("No resolved type for sym '%s'\n", id.c_str());
                    }
                }
            }
            else if (depscope)
            {
                // Dep unit scope
                nextsc.clear();
                mSymTable->scAdd(nextsc, L_UNITWIDESCOPE);
                mSymTable->scAdd(nextsc, id);
            }
            else
            {
                nextsc = sc;
                mSymTable->scAdd(nextsc, id);
            }
        }

        if (matches > 1 && allmatches)
        {
            //dbglog("Matches=%lld at sc=%s sym=%s\n", matches, matsc.c_str(), id.c_str());
            PlArr<EntityType> marr = mSymTable->findAll(matsc, id.c_str());
            for (int i = 0; i < marr.count(); i++)
            {
                allmatches->append(marr.get(i));
            }
        }
    }
    return en;
}

void PlResolver::deterType(EntityType rentity, ETag tg)
{
    std::string id = rentity->getIdentStr(tg);
    //dbglog("Det type for %d '%s' var=%s\n", tg, id.c_str(), rentity->getIdentStr(ETag::VarName).c_str());
    //rentity->dbgDump("det ");

    if (id.empty())
    {
        return;
    }

    if (rentity->mDT == DataType::d_none)
    {
        DataType dt;
        if (sDataTypeMap.fromString(id, &dt))
        {
            rentity->mDT = dt;
        }
    }

    if (base::streql(id, sCppKeywordMap.toString(CppKeyword::cpp_auto)))
    {
        // If we are supposed to deduce type (auto), and type is potentially a string, we
        // special case it here to convert auto to string.

        EntityType auten = rentity->getChild(EKind::Ident, tg);
        if (auten && auten->mDT == DataType::d_string)
        {
            id.assign(sDataTypeMap.toString(auten->mDT));
        }
    }

    rentity->mResolvedRef = mSymTable->findSymbol(id.c_str());
    if (rentity->mResolvedRef && (rentity->mDT == DataType::d_none || rentity->mResolvedRef->mKind == EKind::TypeDef))
    {
        if (rentity->mResolvedRef->mKind == EKind::TypeDef)
        {
            // Resolve to the deepest typedef
            EntityType rt = rentity->mResolvedRef;
            for (;;)
            {
                std::string gid = rt->getChildStr(EKind::Generic, ETag::Primary);
                //dbglog("UT: %s\n", gid.c_str());

                EntityType gt = mSymTable->findSymbol(gid.c_str());

                if (gt && gt->mKind == EKind::TypeDef)
                {
                    rentity->mResolvedRef = gt;
                    rt = gt;
                }
                else
                {
                    break;
                }
            }

            if (rentity->mResolvedRef)
            {
                // Since string is a built-in value type
                if (rentity->mDT != DataType::d_string)
                {
                    rentity->mDT = rentity->mResolvedRef->mDT;
                }
            }
        }
        else if (rentity->mResolvedRef->mKind == EKind::Object)
        {
            rentity->mDT = DataType::d_object;
        }
        else
        {
            rentity->mDT = DataType::d_none;
        }
    }
}

void PlResolver::deterTemplArgs(EntityType templtypeen, std::string& typestr)
{
    if (!templtypeen->hasAttrib(EAttribFlags::a_templtype) || templtypeen->mResolvedRef == nullptr)
    {
        return;
    }
    // templtypeen->dbgDump("templtypeen: ");
    // templtypeen->mResolvedRef->dbgDump("templtypeen-res: ");

    auto fromarr = templtypeen->mResolvedRef->getChildren(EKind::Ident, ETag::TemplArg);
    auto toarr = templtypeen->getChildren(EKind::Ident, ETag::TemplArg);
    if (fromarr.count() != toarr.count())
    {
        pushErr(templtypeen, "Templ type argument count mismatch");
        return;
    }

    for (size_t i = 0; i < toarr.count(); i++)
    {
        if (base::streql(typestr, fromarr.get(i)->getStr() ))
        {
            //dbglog("Template argument <%s> replaced with '%s'\n", fromarr.get(i)->getStr().c_str(), toarr.get(i)->getStr().c_str());
            typestr.assign(toarr.get(i)->getStr());
            return;
        }
    }
}


EntityType PlResolver::deterSym(EntityType en, strparam ss)
{
    en->mResolvedRef = mSymTable->findSymbol(ss.data());
    if (en->mResolvedRef)
    {
        en->mDT = en->mResolvedRef->mDT;
    }

    return en->mResolvedRef;
}

void PlResolver::resoImpl(EntityType rentity, ETag tg, EntityType prev)
{
    std::string id = rentity->getIdentStr(ETag::ClassName);
    // Resolve to the corresponding 'object'
    rentity->mResolvedRef = mSymTable->findSymbol(id.c_str());
    if (rentity->mResolvedRef)
    {
        // Add the 'impl' link into the 'object'
        rentity->mResolvedRef->createResol(rentity, nullptr);
    }
    else
    {
        pushErr(rentity, "Cannot find matching object for impl '%s'", id.c_str());
    }
}

void PlResolver::resoForStmt(EntityType rentity, ETag tg, EntityType prev)
{
    // get Iter's Expr VarEval
    // get type of that expression
    // Set type of for var

    PlTypeInfo ty(this);
    EntityType iten = rentity->getChild(EKind::Iter);
    if (iten)
    {
        // Get iter type which must be a collection
        // TODO: Add check for that
        PlTypeInfo collty(this);
        collty = getType(iten->getChild(EKind::Expr, ETag::IterVal));

        // Templ arg is the type
        // TODO: need proper dt
        ty.mName = collty.mTemplArg;
        ty.mDT = DataType::d_object;
        ty.mBoxed = collty.mBoxed;
    }
    else
    {
        EntityType rngen = rentity->getChild(EKind::Range);
        if (rngen)
        {
            ty = getType(rngen->getChild(EKind::Expr, ETag::RangeFromVal));
        }
        else
        {
            pushErr(rentity, "Invalid for syntax");
        }
    }

    EntityType vt = rentity->getChild(ETag::VarType);
    if (vt)
    {
        vt->mDT = ty.mDT;
        vt->setTokStr(ty.mName);
        if (rentity->mDT == DataType::d_none)
        {
            rentity->mDT = ty.mDT;
        }
        if (ty.mBoxed)
        {
            rentity->addAttrib(EAttribFlags::a_boxed);
        }
    }
    deterType(rentity, ETag::VarType);

    if (ty.mDT == DataType::d_object && !ty.mName.empty())
    {
        // Setup the scope for the type
        PlResolver::createSymScope(rentity, ty.mName);
    }
}

void PlResolver::resoFuncCall(EntityType rentity, ETag tg, EntityType prev)
{
    std::string typectx;
    EntityType ctxen = nullptr;

    // Look up the function name
    if (prev && prev->mKind == EKind::Deref)
    {
        EntityType tc = prev->getChild(EKind::TypeCtx);
        if (tc && tc->mResolvedRef)
        {
            EntityType ss = tc->mResolvedRef->getChild(ETag::SymScope);
            if (ss == nullptr && tc->mResolvedRef->mResolvedRef)
            {
                ss = tc->mResolvedRef->mResolvedRef->getChild(ETag::SymScope);
            }
            if (ss)
            {
                //dbglog("Look for func in %s\n", ss->mToken.cstr());
                std::string fnid = rentity->getSimpIdentStr(ETag::FuncName);
                rentity->mResolvedRef = mSymTable->findSymbolSc(ss->mToken.cstr(), fnid.c_str(), nullptr);
            }
        }
    }

    if (rentity->mResolvedRef == nullptr)
    {
        PlArr<EntityType> allmatches;
        EntityType ren = deterIdent(rentity, ETag::FuncName, typectx, &allmatches);
        if (allmatches.count() > 1)
        {
            // Found multiple matches--need to find right one with types
            for (size_t i = 0; i < allmatches.count(); i++)
            {
                if (paramsCompat(rentity, allmatches.get(i)))
                {
                    ren = allmatches.get(i);
                }
            }
        }
        rentity->mResolvedRef = ren;
    }
    dbgCheckIdent(rentity, ETag::FuncName);
    checkReso(rentity, ETag::FuncName, EKind::FuncMapping, EKind::FuncBody, EKind::FuncDef);

    // Add type type context
    if (!typectx.empty())
    {
        EntityType tc = PlEntity::newOrFindEntity(rentity, typectx.c_str(), EKind::TypeCtx);
        ctxen = deterSym(tc, typectx);
    }

    // Get the return type from the func def
    if (rentity->mResolvedRef)
    {
        //rentity->mResolvedRef->dbgDump("funcall: ");
        std::string rettype = rentity->mResolvedRef->getIdentStr(ETag::ReturnType);

        if (ctxen)
        {
            deterTemplArgs(ctxen, rettype);
        }

        EntityType reten = PlEntity::newOrFindEntity(rentity, rettype.c_str(), EKind::Ident, ETag::ReturnType);
        deterSym(reten, rettype);
    }
}

void PlResolver::resoDeref(EntityType rentity, ETag tg, EntityType prev)
{
    if (prev)
    {
        PlTypeInfo ty = getType(prev);
        if (!ty.mName.empty())
        {
            EntityType tc = PlEntity::newOrFindEntity(rentity, ty.mName.c_str(), EKind::TypeCtx);
            tc->mResolvedRef = ty.mTypeEn;
        }
    }
}

void PlResolver::resoVarAssign(EntityType rentity, ETag tg, EntityType prev)
{
    if (!rentity->mResolvedRef)
    {
        std::string typectx;
        rentity->mResolvedRef = deterIdent(rentity, ETag::VarName, typectx);
    }
}

void PlResolver::resoVarEval(EntityType rentity, ETag tg, EntityType prev)
{
    if (!rentity->mResolvedRef)
    {
        std::string typectx;
        rentity->mResolvedRef = deterIdent(rentity, ETag::VarName, typectx);
    }
}

void PlResolver::fixupPhase0(EntityType rentity, ETag tg, EntityType prev)
{
    //dbgDump("Resolve ");
    if (!rentity->mResolvedRef)
    {
        switch (rentity->mKind)
        {
            case EKind::TypeDef:
                deterType(rentity, ETag::VarType);
                break;

            default:
                break;
        }
    }

    // Scope the symbol table
    PlSymbolAg symag(true, mSymTable, mUnit);
    symag.process(rentity, nullptr);

    EntityType prevpeer = nullptr;
    for (auto& sub : rentity->mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            fixupPhase0(&e, sub.mTag, prevpeer);
            prevpeer = &e;
        }
    }
}

void PlResolver::fixupPhase1(EntityType rentity, ETag tg, EntityType prev)
{
    //dbgDump("Resolve ");
    if (!rentity->mResolvedRef)
    {
        switch (rentity->mKind)
        {
            case EKind::FuncParam:
                deterType(rentity, ETag::ParamType);
                break;

            case EKind::VarDecl:
            case EKind::ObjectFld:
                deterType(rentity, ETag::VarType);
                break;

            case EKind::ForStmt:
                resoForStmt(rentity, tg, prev);
                break;

            case EKind::New:
                deterType(rentity, ETag::ClassName);
                break;

            case EKind::FuncDef:
            case EKind::FuncBody:
            case EKind::FuncMapping:
                deterType(rentity, ETag::ReturnType);
                break;

            case EKind::VarAssign:
                resoVarAssign(rentity, tg, prev);
                break;

            case EKind::VarEval:
                resoVarEval(rentity, tg, prev);
                break;

            case EKind::Deref:
                resoDeref(rentity, tg, prev);
                break;

            case EKind::FuncCall:
                resoFuncCall(rentity, tg, prev);
                break;

            case EKind::Impl:
                resoImpl(rentity, tg, prev);
                break;

            default:
                break;
        }
    }

    // Scope the symbol table
    PlSymbolAg symag(true, mSymTable, mUnit);
    symag.process(rentity, nullptr);

    EntityType prevpeer = nullptr;
    for (auto& sub : rentity->mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            fixupPhase1(&e, sub.mTag, prevpeer);
            prevpeer = &e;
        }
    }
}

void PlResolver::fixupAll(EntityType rentity)
{
    fixupPhase0(rentity, ETag::Primary, nullptr);
    fixupPhase1(rentity, ETag::Primary, nullptr);
}

void PlResolver::validateAll(EntityType rentity, ETag tg)
{
    switch (rentity->mKind)
    {
        case EKind::FuncParam:
        case EKind::VarDecl:
        case EKind::ObjectFld:
        {
            if (rentity->mDT == DataType::d_none && rentity->mResolvedRef == nullptr)
            {
                ETag tg = (rentity->mKind == EKind::FuncParam ? ETag::ParamType : ETag::VarType);
                std::string id = rentity->getIdentStr(tg);

                // if type is 'auto' by this point, we let it slide and don't error out
                // and rely on C++ auto functionality

                if (!base::streql(id, sCppKeywordMap.toString(CppKeyword::cpp_auto)))
                {
                    pushErr(rentity->getChild(tg), "Undefined type '%s'", id.c_str());
                }
            }
        }
        break;

        case EKind::New:
        {
            if (rentity->mDT == DataType::d_none && rentity->mResolvedRef == nullptr)
            {
                std::string id = rentity->getIdentStr(ETag::ClassName);
                pushErr(rentity->getChild(tg), "Undefined type '%s' for new", id.c_str());
            }
        }
        break;

        case EKind::FuncDef:
        case EKind::FuncBody:
        {
            if (rentity->mDT == DataType::d_none && rentity->mResolvedRef == nullptr)
            {
                std::string id = rentity->getIdentStr(ETag::ReturnType);
                // if not void return
                if (!id.empty())
                {
                    pushErr(rentity->getChild(tg), "Undefined return type '%s'", id.c_str());
                }
            }
        }
        break;

        case EKind::VarEval:
        {
            if (!rentity->mResolvedRef)
            {
                std::string id = rentity->getIdentStr(ETag::VarName);
                pushErr(rentity->getChild(ETag::VarName), "Type for '%s' is not declared", id.c_str());
            }
        }
        break;

        case EKind::VarAssign:
        {
            checkVarAssign(rentity);
        }
        break;

        case EKind::FuncCall:
        {
            checkFuncCall(rentity);
        }
        break;

        default:
        break;
    }

    for (auto& sub : rentity->mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            validateAll(&e, sub.mTag);
        }
    }
}

void PlResolver::checkReso(EntityType rentity, ETag tg, EKind k1, EKind k2, EKind k3)
{
    if (rentity->mResolvedRef == nullptr)
    {
        //pushErr("Couldn't resolve entity '%s'", getIdentStr(tg).c_str());
        return;
    }

    // OK if the resolved entity is of kind k1, k2, or k3 (if specified)
    if (rentity->mResolvedRef->mKind == k1)
    {
        return;
    }
    if (k2 != EKind::None && rentity->mResolvedRef->mKind == k2)
    {
        return;
    }
    if (k3 != EKind::None && rentity->mResolvedRef->mKind == k3)
    {
        return;
    }
    rentity->mResolvedRef = nullptr;
    pushErr(rentity, "Invalid resolved entity for '%s'", rentity->getIdentStr(tg).c_str());
}

void PlResolver::checkVarAssign(EntityType rentity)
{
    std::string varname = rentity->getIdentStr(ETag::VarName);
    if (!rentity->mResolvedRef)
    {
        pushErr(rentity->getChild(ETag::VarName), "Type for '%s' is not declared", varname.c_str());
        return;
    }

    PlTypeInfo lty = getType(rentity);
    PlTypeInfo rty = getType(rentity->getChild(EKind::Expr, ETag::AssignVal));

    // dbglog("var: %s\n", varname.c_str());
    // lty.dbgDump("LHS");
    // rty.dbgDump("RHS");

    if (lty.mDT != DataType::d_none)
    {
        if (!lty.isCompat(rty))
        {
            pushErr(rentity->getChild(ETag::VarName), "Cannot assign '%s' to '%s' for '%s'", rty.name(), lty.name(), varname.c_str());
            return;
        }
    }
}

void PlResolver::checkFuncCall(EntityType rentity)
{
    std::string funcname = rentity->getIdentStr(ETag::FuncName);
    if (!rentity->mResolvedRef)
    {
        pushErr(rentity->getChild(ETag::FuncName), "Func '%s' is not defined", funcname.c_str());
        return;
    }

    // Validate param types

    auto carr = rentity->getChildren(EKind::Expr, ETag::FuncArgVal);
    auto darr = rentity->mResolvedRef->getChildren(EKind::FuncParam);

    size_t ci = 0;
    size_t di = 0;

    for (;;)
    {
        EntityType c = carr.get(ci);
        EntityType d = darr.get(di);

        if (c == nullptr || d == nullptr)
        {
            break;
        }

        PlTypeInfo cty = getType(c);
        PlTypeInfo dty = getType(d);

        bool cref = c->hasAttrib(EAttribFlags::a_inout);
        bool dref = d->hasAttrib(EAttribFlags::a_inout);

        if (!cty.isCompat(dty))
        {
            pushErr(rentity->getChild(ETag::FuncName), "Incompatible type '%s' for param #%lld in call to '%s' (param '%s')", cty.name(), ci, funcname.c_str(), dty.name());
            return;
        }
        else if (cref != dref)
        {
            pushErr(rentity->getChild(ETag::FuncName), "Param #%lld must be marked with '&' in call to func '%s' ", ci, funcname.c_str());
            return;
        }

        if (!d->hasAttrib(EAttribFlags::a_ellipses))
        {
            di++;
        }
        ci++;

    }
}

bool PlResolver::paramsCompat(EntityType cal, EntityType def)
{
    auto carr = cal->getChildren(EKind::Expr, ETag::FuncArgVal);
    auto darr = def->getChildren(EKind::FuncParam);

    size_t ci = 0;
    size_t di = 0;

    for (;;)
    {
        EntityType c = carr.get(ci);
        EntityType d = darr.get(di);
        if (c == nullptr || d == nullptr)
        {
            break;
        }

        PlTypeInfo cty = getType(c);
        PlTypeInfo dty = getType(d);
        bool cref = c->hasAttrib(EAttribFlags::a_inout);
        bool dref = d->hasAttrib(EAttribFlags::a_inout);

        if (!cty.isCompat(dty))
        {
            return false;
        }
        else if (cref != dref)
        {
            return false;
        }
        if (!d->hasAttrib(EAttribFlags::a_ellipses))
        {
            di++;
        }
        ci++;
    }
    return true;
}

void PlResolver::dbgCheckIdent(EntityType rentity, ETag tg)
{
#ifdef _DEBUG
    if (rentity->mResolvedRef == nullptr)
    {
        mSymTable->mSpew = true;
        dbglog("------ deterIdent '%s':\n", rentity->getIdentStr(tg).c_str());
        //mSymTable->dbgDump();
        std::string typectx;
        deterIdent(rentity, tg, typectx);
        mSymTable->mSpew = false;
    }
#endif
}

void PlResolver::pushErr(EntityType en, const char* fmt, ...)
{
    std::string usermsg;
    va_list va;
    va_start(va, fmt);
    bool ret = base::vformat(usermsg, fmt, va);
    va_end(va);

    mUnit->mErrCol.add(en, usermsg);
}


// PlTypeInfo::

bool PlTypeInfo::fromSigCh(EntityType rentity)
{
    if (!rentity)
    {
        return false;
    }

    if (rentity->mKind == EKind::VarEval || rentity->mKind == EKind::VarAssign)
    {
        // Get var declaration
        if (!rentity->mResolvedRef)
        {
            mResolver->resoVarEval(rentity, ETag::Any, nullptr);
        }
        EntityType decen = rentity->mResolvedRef;
        if (decen)
        {
            if (!decen->mResolvedRef)
            {
                mResolver->deterType(decen, ETag::VarType);
            }
            mTypeEn = decen;
        }
        if (mTypeEn)
        {
            ETag tg = sigTag(mTypeEn);
            mName = mTypeEn->getIdentStr(tg);
            mDT = mTypeEn->mDT;
            mTemplArg = mTypeEn->getIdentStr(ETag::TemplArg);
            mBoxed = mTypeEn->hasAttrib(EAttribFlags::a_boxed);
        }
    }
    else if (rentity->mKind == EKind::DollarExpr)
    {
        mTypeEn = rentity;
        mDT = DataType::d_string;
    }
    else if (rentity->mKind == EKind::Literal)
    {
        mTypeEn = rentity;
        mDT = rentity->mDT;
    }
    else
    {
        ETag tg = sigTag(rentity);
        mName = rentity->getIdentStr(tg);

        if (rentity->mKind == EKind::FuncParam)
        {
            mTypeEn = rentity;
            mDT = rentity->mDT;
        }
        else
        {
            EntityType e = rentity->getChild(tg);
            if (e)
            {
                mTypeEn = e->mResolvedRef;
                mDT = e->mDT;
            }
        }
    }

    if (base::streql(mName, "string"))
    {
        mDT = DataType::d_string;
    }

    return mDT != DataType::d_none;
}

bool PlTypeInfo::fromExpr(EntityType rentity)
{
    // TODO: Needs more work
    auto arr = rentity->getChildren(ETag::Any);
    for (size_t i = 0; i < arr.count(); i++)
    {
        auto e = arr.get(i);
        if (fromSigCh(e))
        {
            return true;
        }
    }
    return false;
}


ETag PlTypeInfo::sigTag(EntityType en)
{
    switch (en->mKind)
    {
        case EKind::FuncParam:
            return ETag::ParamType;

        case EKind::TypeDef:
            return ETag::TypeName;

        case EKind::VarDecl:
        case EKind::ObjectFld:
            return ETag::VarType;

        case EKind::ForStmt:
            return ETag::VarType;

        case EKind::New:
            return ETag::ClassName;

        case EKind::FuncDef:
        case EKind::FuncBody:
            return ETag::ReturnType;

        case EKind::FuncCall:
            return ETag::ReturnType;

        default:
            return ETag::Any;
    }
}

const char* PlTypeInfo::name()
{
    if (mDT == DataType::d_object)
    {
        return mName.c_str();
    }
    else
    {
        return sDataTypeMap.toString(mDT);
    }
}

bool PlTypeInfo::isCompat(const PlTypeInfo& that)
{
    if (that.mDT == DataType::d_none || mDT == DataType::d_none)
    {
        // For now
        return true;
    }
    else if (mDT == DataType::d_object)
    {
        if (that.mDT == mDT && base::streql(mName, that.mName))
        {
            return true;
        }
    }
    else if (mDT == that.mDT)
    {
        return true;
    }
    else if (isInt(mDT) && isInt(that.mDT))
    {
        return true;
    }
    else if (isStr(mDT) && isStr(that.mDT))
    {
        return true;
    }
    else if (isFloat(mDT) && isFloat(that.mDT))
    {
        return true;
    }

    return false;
}

bool PlTypeInfo::isInt(DataType dt)
{
    switch (dt)
    {
        case DataType::d_int8:
        case DataType::d_int16:
        case DataType::d_int32:
        case DataType::d_int64:
        case DataType::d_uint8:
        case DataType::d_uint16:
        case DataType::d_uint32:
        case DataType::d_uint64:
        case DataType::d_usize:
            return true;

        default:
            return false;
    }
    return false;
}

bool PlTypeInfo::isFloat(DataType dt)
{
    switch (dt)
    {
        case DataType::d_float32:
        case DataType::d_float64:
            return true;

        default:
            return false;
    }
    return false;
}

bool PlTypeInfo::isStr(DataType dt)
{
    switch (dt)
    {
        case DataType::d_string:
        case DataType::d_czstr:
            return true;

        default:
            return false;
    }
    return false;
}


void PlTypeInfo::dump(base::StrBld& bld)
{
    bld.append(" #");
    bld.append(sDataTypeMap.toString(mDT));
    bld.appendc(' ');
    bld.append(mName);
}

void PlTypeInfo::dbgDump(const char* label)
{
#ifdef _DEBUG
    base::StrBld bld;
    bld.append(label);
    dump(bld);
    bld.append("\n");
    dbglog("%s", bld.c_str());
#endif
}

