#include "primalc.h"

// CppCode::
class CppCode
{
public:
    bool mHeaderMode;
    bool mPubMode;
    std::string mNamespace;
    PlUnit* mUnit;

    CppCode() :
        mHeaderMode(false),
        mPubMode(false),
        mUnit(nullptr)
    {
    }

    void mkTemplate(EntityType p, std::string& declstr, std::string& usestr)
    {
        //p->dbgDump("mktemp ");

        declstr.clear();
        usestr.clear();
        auto arr = p->getChildren(ETag::TemplArg);
        for (int i = 0; i < arr.count(); i++)
        {
            EntityType t = arr.get(i);

            if (i == 0)
            {
                declstr.assign("template <");
                usestr.assign("<");
            }
            else
            {
                declstr.append(", ");
                usestr.append(", ");
            }
            declstr.append("typename ");
            declstr.append(t->getStr());
            usestr.append(t->getStr());
        }
        if (!declstr.empty())
        {
            declstr.append(">");
            usestr.append(">");
        }
    }

    void mkResolvedType(EntityType p, ETag tg, std::string& typestr)
    {
        typestr.clear();
        // Check the resolved reference to see if we need to use a mapped type
        if (p->mResolvedRef)
        {
            //p->dbgDump("type p: ");
            //p->mResolvedRef->dbgDump("resolve type p: ");

            // Found a typedef.  If it is a cppobject, cpp code already has a using
            // defined for it elsewhere, so we don't want to get the cppdirec, we use
            // the typename instead.
            if (p->mResolvedRef->hasAttrib(EAttribFlags::a_cppobject))
            {
                typestr = p->mResolvedRef->getIdentStr(ETag::TypeName);
            }
            else
            {
                EntityType cppdirec = p->mResolvedRef->getChild(EKind::CppDirec);
                if (cppdirec)
                {
                    typestr = cppdirec->getCppDirecStr(CppDirecType::type);
                }
                else
                {
                    typestr = p->mResolvedRef->getChildStr(EKind::Generic, ETag::Primary);
                }
            }
        }
        if (typestr.empty())
        {
            typestr = p->getIdentStr(tg);
        }
    }

    void emitNsOpen(Code& c)
    {
        if (!mNamespace.empty())
        {
            c.emit(CppKeyword::cpp_namespace);
            c.emit(mNamespace);
            c.braceOpen();
        }
    }

    void emitNsClose(Code& c)
    {
        if (!mNamespace.empty())
        {
            c.braceClose(false);
            c.emitFmt(" // namespace %s", mNamespace.c_str());
            c.nl();
        }
    }

    void emitSubEntities(Code& c, EntityType ep, ETag tg)
    {
        if (ep == nullptr)
        {
            return;
        }
        auto arr = ep->getChildren(tg);
        for (size_t i = 0; i < arr.count(); i++)
        {
            auto e = arr.get(i);
            emitEntity(c, e);
        }
    }

    void emitVarType(Code& c, EntityType ep, ETag tg)
    {
        std::string restype;
        mkResolvedType(ep, tg, restype);
        c.emit(restype);

        auto arr = ep->getChildren(ETag::TemplArg);
        if (arr.count() > 0)
        {
            c.emit("<", false);
            for (size_t i = 0; i < arr.count(); i++)
            {
                std::string tempclass = arr.get(i)->getStr();
                if (ep->hasAttrib(EAttribFlags::a_boxed))
                {
                    if (base::streql(tempclass, "string"))
                    {
                        tempclass = sCppProp.getStr(CppPropType::strtype);
                    }
                    c.emit(sCppProp.mkBoxName(tempclass), false);
                }
                else
                {
                    c.emit(sCppProp.mkClassName(tempclass), false);
                }
            }
            c.emit(">", false);
        }
    }

    void emitExpr(Code& c, EntityType ep)
    {
        emitSubEntities(c, ep, ETag::Primary);
    }

    void emitParanExpr(Code& c, EntityType ep)
    {
        c.emit("(");
        emitSubEntities(c, ep, ETag::Primary);
        c.emit(")");
    }

    void emitDollarExpr(Code& c, EntityType ep)
    {
        c.emit("_D(");
        emitSubEntities(c, ep, ETag::Primary);
        c.emit(")");
    }

    void emitReturnStmt(Code& c, EntityType ep)
    {
        c.emit(CppKeyword::cpp_return);
        EntityType retval = ep->getChild(ETag::ReturnVal);
        if (retval)
        {
            emitExpr(c, retval);
        }
        c.semiln();
    }

    void emitBreakStmt(Code& c, EntityType ep)
    {
        EntityType condval = ep->getChild(ETag::CondVal);
        if (condval)
        {
            c.emit(CppKeyword::cpp_if);
            c.emit("(", false);
            emitExpr(c, condval);
            c.emit(")", false);
            c.braceOpen();
            c.indentInc();
        }
        c.emitln("break;");
        if (condval)
        {
            c.indentDec();
            c.braceClose(true);
        }
    }

    void emitContinStmt(Code& c, EntityType ep)
    {
        EntityType condval = ep->getChild(ETag::CondVal);
        if (condval)
        {
            c.emit(CppKeyword::cpp_if);
            c.emit("(", false);
            emitExpr(c, condval);
            c.emit(")", false);
            c.braceOpen();
            c.indentInc();
        }
        c.emitln("continue;");
        if (condval)
        {
            c.indentDec();
            c.braceClose(true);
        }
    }

    void emitForStmt(Code& c, EntityType ep)
    {
        c.emit(CppKeyword::cpp_for);

        c.emit("(");

        EntityType range = ep->getChild(EKind::Range);
        if (range)
        {
            // Range for
            bool rev = range->hasAttrib(EAttribFlags::a_reverse);
            bool inc = range->hasAttrib(EAttribFlags::a_inclusive);
            EntityType fromexp = range->getChild(ETag::RangeFromVal);
            EntityType toexp = range->getChild(ETag::RangeToVal);

            c.emit(CppKeyword::cpp_auto);
            std::string forvar = ep->getIdentStr(ETag::ForVar);
            c.emit(forvar);
            c.emit("= ");

            // fwd:     for (auto i = fromval; i < toval; i++)
            // fwd inc: for (auto i = fromval; i <= toval; i++)
            // rev:     for (auto i = toval; i-- > fromval; )
            // rev inc: for (auto i = toval + 1; i-- > fromval; )
            if (!rev)
            {
                // fwd
                emitExpr(c, fromexp);
            }
            else
            {
                // rev
                emitExpr(c, toexp);

                if (inc)
                {
                    c.emit(" + 1");
                }
            }
            c.emit("; ");

            c.emit(forvar);
            if (!rev)
            {
                // fwd
                if (inc)
                {
                    // fwd inc
                    c.emit("<=");
                }
                else
                {
                    // fwd exc
                    c.emit("<");
                }
                emitExpr(c, toexp);
                c.emit("; ");

                c.emit(forvar, false);
                c.emit("++", false);
            }
            else
            {
                // rev
                c.emit("-- >", false);
                emitExpr(c, fromexp);
                c.emit("; ");
            }
        }
        else
        {
            EntityType iter = ep->getChild(EKind::Iter);
            if (iter)
            {
                // for (auto i = arr->iterFwd(); arr->iterFwdLoop(i); )
                // for (auto i = arr->iterRev(); arr->iterRevLoop(i); )

                // Obj iter for
                bool rev = iter->hasAttrib(EAttribFlags::a_reverse);
                EntityType iterval = iter->getChild(ETag::IterVal);
                std::string forvar = ep->getIdentStr(ETag::ForVar);
                if (!iterval)
                {
                    pushErr(c, "'for' statement missing iter class");
                    return;
                }

                c.emit(CppKeyword::cpp_auto);
                c.emit(forvar);
                c.emit("= ");
                emitExpr(c, iterval);
                c.emit("->", false);
                c.emit(rev ? "iterRev" : "iterFwd", false);
                c.emit("(); ");

                emitExpr(c, iterval);
                c.emit("->", false);
                c.emit(rev ? "iterRevLoop" : "iterFwdLoop", false);
                c.emit("(", false);
                c.emit(forvar);
                c.emit("); ", false);
            }
        }

        c.emit(")");

        c.nl();

        // Emit the stmt block. A null stmtblock will emit a {}
        emitStmtBlock(c, ep->getChild(EKind::StmtBlock));
    }

    void emitLoopStmt(Code& c, EntityType ep)
    {
        c.emitln("for (;;)");
        // Emit the stmt block. A null stmtblock will emit a {}
        emitStmtBlock(c, ep->getChild(EKind::StmtBlock));
    }

    void emitWhileStmt(Code& c, EntityType ep)
    {
        EntityType condval = ep->getChild(ETag::CondVal);
        if (!condval)
        {
            pushErr(c, "Missing 'while' condition");
            return;
        }

        c.emit(CppKeyword::cpp_while);
        c.emit("(", false);
        emitExpr(c, condval);
        c.emitln(")");
        // Emit the stmt block. A null stmtblock will emit a {}
        emitStmtBlock(c, ep->getChild(EKind::StmtBlock));
    }

    void emitIfStmt(Code& c, EntityType ep)
    {
        // If part
        EntityType ifcond = ep->getChild(ETag::CondVal);
        if (!ifcond)
        {
            pushErr(c, "Missing 'if' condition");
            return;
        }
        c.emit(CppKeyword::cpp_if);
        c.emit("(", false);
        emitExpr(c, ifcond);
        c.emitln(")");
        emitStmtBlock(c, ep->getChild(EKind::StmtBlock));

        auto arr = ep->getChildren(ETag::Primary);

        // Else if part
        for (size_t i = 0; i < arr.count(); i++)
        {
            auto e = arr.get(i);
            if (e->mKind == EKind::ElseIf)
            {
                EntityType elifcond = e->getChild(ETag::CondVal);
                if (!elifcond)
                {
                    pushErr(c, "Missing 'else if' condition");
                    return;
                }
                c.emit(CppKeyword::cpp_else);
                c.emit(CppKeyword::cpp_if);
                c.emit("(", false);
                emitExpr(c, elifcond);
                c.emitln(")");
                emitStmtBlock(c, e->getChild(EKind::StmtBlock));
            }
        }

        // Else part
        for (size_t i = 0; i < arr.count(); i++)
        {
            auto e = arr.get(i);
            if (e->mKind == EKind::Else)
            {
                c.emit(CppKeyword::cpp_else);
                emitStmtBlock(c, e->getChild(EKind::StmtBlock));
                break;
            }
        }
    }

    void emitSwitchStmt(Code& c, EntityType ep)
    {
        // If part
        EntityType swval = ep->getChild(ETag::SwitchVal);
        if (!swval)
        {
            pushErr(c, "'switch' value missing");
            return;
        }
        c.emit(CppKeyword::cpp_switch);
        c.emit("(", false);
        emitExpr(c, swval);
        c.emitln(")");

        c.braceOpen();
        c.indentInc();

        // case parts
        auto arr = ep->getChildren(EKind::Case, ETag::Primary);
        for (size_t i = 0; i < arr.count(); i++)
        {
            auto cen = arr.get(i);

            auto valarr = cen->getChildren(EKind::Literal, ETag::CaseVal);
            for (size_t vi = 0; vi < valarr.count(); vi++)
            {
                if (valarr.get(vi)->hasAttrib(EAttribFlags::a_ellipses))
                {
                    c.emit("default");
                }
                else
                {
                    c.emit(CppKeyword::cpp_case);
                    c.emit(valarr.get(vi)->getStr());
                }
                c.emitln(":");
            }
            emitStmtBlock(c, cen->getChild(EKind::StmtBlock));
            c.emitln("break;");
        }

        c.indentDec();
        c.braceClose(true);
    }

    void emitStmtBlock(Code& c, EntityType ep)
    {
        c.braceOpen();
        c.indentInc();

        if (ep)
        {
            auto arr = ep->getChildren();
            for (size_t i = 0; i < arr.count(); i++)
            {
                // Emit the statement entity
                emitEntity(c, arr.get(i));

                // Emit additional requirements (ie a ; or nl)
                switch (arr.get(i)->mKind)
                {
                    case EKind::FuncCall:
                    {
                        // Emit a semicolon and nl unless the next object is a deref
                        bool nextderef = ((i + 1) < arr.count()) && arr.get(i + 1)->mKind == EKind::Deref;
                        if (!nextderef)
                        {
                            c.semiln();
                        }
                    }
                    break;

                    default:
                    break;
                }
            }

            // If func body, emit defer'd statements
            EntityType deferen = ep->getChild(EKind::DeferStmt);
            if (deferen)
            {
                auto arr = deferen->getChildren();
                for (size_t i = 0; i < arr.count(); i++)
                {
                    if (i == 0)
                    {
                        c.emitln("// 'defer' statements ");
                    }
                    emitStmtBlock(c, arr.get(i));
                }
            }
        }

        c.indentDec();
        c.braceClose(true);
    }

    void emitVarDecl(Code& c, EntityType ep)
    {
        emitVarType(c, ep, ETag::VarType);

        c.emit(ep->getIdentStr(ETag::VarName));

        EntityType initval = ep->getChild(ETag::DeclInitVal);
        if (initval)
        {
            c.emit("= ");
            emitExpr(c, initval);
        }
        c.semiln();
    }

    void emitVarAssign(Code& c, EntityType ep)
    {
        emitIdent(c, ep, ETag::VarName);

        c.emit(ep->getChildStr(EKind::Operator, ETag::AssignOp));

        EntityType assignval = ep->getChild(ETag::AssignVal);
        if (assignval)
        {
            emitExpr(c, assignval);
        }
        else
        {
            // TODO: Proper error handling
            pushErr(c, "Missing expression");
        }
        c.semiln();
    }

    void emitNew(Code& c, EntityType ep)
    {
        emitVarType(c, ep, ETag::ClassName);

        c.emit("::");
        c.emit(sCppProp.getStr(CppPropType::createmethod));
        c.emit("()");
    }

    void emitOperator(Code& c, EntityType ep)
    {
        c.emit(ep->getStr());
    }

    void emitVarEval(Code& c, EntityType ep)
    {
        c.emit(ep->getIdentStr(ETag::VarName));
        EntityType re = ep->mResolvedRef;
        if (re && (re->mKind == EKind::ForStmt) && re->hasAttrib(EAttribFlags::a_boxed))
        {
            c.emit(L_PTR, false);
            c.emit(sCppProp.getStr(CppPropType::boxdatafld), false);
        }
    }

    void emitDeref(Code& c, EntityType ep)
    {
        if (ep->hasAttrib(EAttribFlags::a_noderef))
        {
            c.emit(".", false);
        }
        else
        {
            c.emit("->", false);
        }
    }

    void emitLiteral(Code& c, EntityType ep)
    {
        if (ep->mDT == DataType::d_string)
        {
            std::string s = ep->mToken.strNoQuotes(false);

            c.emit(sCppProp.getStr(CppPropType::strlitctor));
            c.emit("(\"");
            c.emit(s);
            c.emit("\", ", false);
            c.emit(std::to_string(s.length()));
            c.emit(")");
        }
        else
        {
            c.emit(ep->getStr());
        }
    }

    void emitIdent(Code& c, EntityType ep, ETag tg)
    {
        auto arr = ep->getChildren(tg);
        EKind prevk = EKind::None;
        EntityType preven = nullptr;
        for (size_t i = 0; i < arr.count(); i++)
        {
            std::string id = arr.get(i)->getStr();
            EntityType resen = arr.get(i)->mResolvedRef;
            EKind k = EKind::None;
            PlUnit* un = nullptr;
            if (resen)
            {
                k = resen->mKind;
                un = resen->mUnit;
            }

            switch (prevk)
            {
                case EKind::VarDecl:
                case EKind::ForStmt:
                case EKind::FuncParam:
                {
                    if (preven && preven->hasAttrib(EAttribFlags::a_noderef))
                    {
                        c.emit(".", false);
                    }
                    else
                    {
                        c.emit("->", false);
                    }
                }
                break;

                case EKind::FileRoot:
                {
                    c.emit("::", false);
                }
                break;

                default:
                break;
            }

            if (k == EKind::FileRoot && un)
            {
                c.emit(un->cppNamespace());
            }
            else
            {
                c.emit(id, false);
            }

            prevk = k;
            preven = resen;
        }
    }

    void emitFuncCall(Code& c, EntityType ep)
    {
        if (!ep->mResolvedRef)
        {
            // std::string name = ep->getIdentStr(ETag::FuncName);
            // err(c, "Function '%s' definition not found", name.c_str());
            return;
        }

        EntityType funcdefen = ep->mResolvedRef;

        emitIdent(c, ep, ETag::FuncName);

        c.emit("(");

        // TODO: Currently the entire function call is made into initializer list
        // but need to do this param by param or for the last param only
        if (funcdefen->hasAttrib(EAttribFlags::a_ellipses))
        {
            c.emit("{");
        }

        auto arr = ep->getChildren(ETag::FuncArgVal);
        for (size_t i = 0; i < arr.count(); i++)
        {
            emitExpr(c, arr.get(i));
            if ((i + 1) < arr.count())
            {
                c.emit(",");
            }
        }
        if (funcdefen->hasAttrib(EAttribFlags::a_ellipses))
        {
            c.emit("}");
        }
        c.emit(")");
    }

    void emitFuncParams(Code& c, EntityType ep)
    {
        c.emit("(");
        if (ep)
        {
            auto params = ep->getChildren(EKind::FuncParam);
            for (size_t i = 0; i < params.count(); i++)
            {
                auto p = params.get(i);

                if (p->hasAttrib(EAttribFlags::a_ellipses))
                {
                    c.emit(sCppProp.getStr(CppPropType::varargspec));
                    c.emit("(");
                }

                emitVarType(c, p, ETag::ParamType);

                if (p->hasAttrib(EAttribFlags::a_ellipses))
                {
                    c.emit(")");
                }

                if (p->hasAttrib(EAttribFlags::a_inout))
                {
                    c.emit("&", false);
                }
                c.emit(p->getIdentStr(ETag::ParamName));

                if ((i + 1) < params.count())
                {
                    c.emit(",");
                }
            }
        }
        c.emit(")");
    }

    bool emitFuncRetType(Code& c, EntityType ep)
    {
        std::string rettype = ep->getIdentStr(ETag::ReturnType);
        if (rettype.empty())
        {
            c.emit(CppKeyword::cpp_void);
            return false;
        }
        else
        {
            //c.emit(rettype);
            emitVarType(c, ep, ETag::ReturnType);
            return true;
        }
    }

    void emitFuncDef(Code& c, EntityType ep, bool purevirt)
    {
        if (purevirt)
        {
            c.emit(CppKeyword::cpp_virtual);
        }
        (void)emitFuncRetType(c, ep);
        std::string name = ep->getIdentStr(ETag::FuncName);
        c.emit(name);
        emitFuncParams(c, ep);
        if (purevirt)
        {
            c.emit(" = 0");
        }
        c.semiln();
    }

    void emitFuncBody(Code& c, EntityType ep, std::string* classname = nullptr)
    {
        (void)emitFuncRetType(c, ep);

        std::string name = ep->getIdentStr(ETag::FuncName);
        if (base::streql(name, L_MAIN))
        {
            std::string val = sCppProp.getStr(CppPropType::mainfunc);
            if (!val.empty())
            {
                name = val;
            }
        }

        if (classname)
        {
            c.emit(*classname);
            c.emit("::");
        }

        c.emit(name);

        emitFuncParams(c, ep);

        if (mHeaderMode)
        {
            c.semiln();
        }
        else
        {
            // Emit the body as a stmt block. Null is emitted as {}
            emitStmtBlock(c, ep->getChild(EKind::StmtBlock));
        }
    }

    void emitFuncMapping(Code& c, EntityType ep)
    {
        if (!mHeaderMode)
        {
            return;
        }
        c.emit(CppKeyword::cpp_inline);

        bool nonvoid = emitFuncRetType(c, ep);

        c.emit(ep->getIdentStr(ETag::FuncName));

        emitFuncParams(c, ep);

        c.braceOpen();
        c.indentInc();

        std::string cppfuncname;
        EntityType cppdirec = ep->getChild(EKind::CppDirec);
        if (cppdirec)
        {
            cppfuncname = cppdirec->getCppDirecStr(CppDirecType::function);
        }
        if (!cppfuncname.empty())
        {
            if (nonvoid)
            {
                c.emit(CppKeyword::cpp_return);
            }

            c.emit(cppfuncname);

            c.emit("(");

            auto params = ep->getChildren(EKind::FuncParam);
            for (size_t i = 0; i < params.count(); i++)
            {
                auto p = params.get(i);

                c.emit(p->getIdentStr(ETag::ParamName));

                if ((i + 1) < params.count())
                {
                    c.emit(",");
                }
            }
            c.emit(")");
        }
        c.semiln();
        c.indentDec();
        c.braceClose(true);
    }

    void emitObject(Code& c, EntityType ep)
    {
        if (!mHeaderMode)
        {
            return;
        }

        // Emit class decl statement with base object inheritence
        std::string orgname = ep->getSimpIdentStr(ETag::ClassName);

        c.emit(CppKeyword::cpp_class);
        c.emit(sCppProp.mkClassName(orgname));
        c.emit(" : ");
        c.emit(CppKeyword::cpp_public);
        c.emit(sCppProp.getStr(CppPropType::baseclass));

        // Get resolved items for this entity
        auto resols = ep->getChildren(EKind::Resol);

        // Emit any inheritted interfaces
        for (size_t r = 0; r < resols.count(); r++)
        {
            EntityType resen = resols.get(r)->mResolvedRef;
            if (resen && resen->mKind == EKind::Impl)
            {
                std::string intfname = resen->getSimpIdentStr(ETag::InterfName);
                if (!intfname.empty())
                {
                    c.emit(", ");
                    c.emit(CppKeyword::cpp_public);
                    c.emit(sCppProp.mkIntfName(intfname));
                }
            }
        }

        // Emit the body
        c.braceOpen();
        c.emit(CppKeyword::cpp_public);
        c.eraseLast();
        c.emitln(":");

        c.indentInc();

        // Emit member variables
        auto flds = ep->getChildren(EKind::ObjectFld);
        for (size_t i = 0; i < flds.count(); i++)
        {
            auto f = flds.get(i);

            emitVarType(c, f, ETag::VarType);

            c.emit(f->getIdentStr(ETag::VarName));
            c.semiln();
        }

        // Emit methods
        c.indentDec();
        c.emit(CppKeyword::cpp_public);
        c.eraseLast();
        c.emit(":");

        c.indentInc();
        for (size_t r = 0; r < resols.count(); r++)
        {
            EntityType resen = resols.get(r)->mResolvedRef;
            if (resen && resen->mKind == EKind::Impl)
            {
                c.nl();
                emitSubEntities(c, resen, ETag::Primary);
            }
        }

        c.indentDec();
        c.braceClose(false);
        c.semiln();

        // Typedefs
        c.emitFmt("using %s = %s<%s>;",
            orgname.data(),
            sCppProp.getStr(CppPropType::strongreftype).c_str(),
            sCppProp.mkClassName(orgname).c_str());
        c.nl();

        c.emitFmt("using %s = %s<%s>;",
            sCppProp.mkWeak(orgname).c_str(),
            sCppProp.getStr(CppPropType::weakreftype).c_str(),
            sCppProp.mkClassName(orgname).c_str());
        c.nl();

        c.nl();
    }

    void emitInterf(Code& c, EntityType ep)
    {
        if (!mHeaderMode)
        {
            return;
        }

        std::string intfname = ep->getSimpIdentStr(ETag::InterfName);

        std::string declstr;
        std::string usestr;
        mkTemplate(ep, declstr, usestr);

        if (!declstr.empty())
        {
            c.emitln(declstr);
        }

        c.emit(CppKeyword::cpp_class);
        c.emit(sCppProp.mkIntfName(intfname));

        c.braceOpen();
        c.emit(CppKeyword::cpp_public);
        c.eraseLast();
        c.emitln(":");

        // Emit methods

        c.indentInc();
        auto fns = ep->getChildren(EKind::FuncDef);
        for (size_t r = 0; r < fns.count(); r++)
        {
            emitFuncDef(c, fns.get(r), true);
        }

        c.indentDec();
        c.braceClose(false);
        c.semiln();

        if (!intfname.empty())
        {
            c.emitFmt("%susing %s = %s<%s%s>;",
                declstr.c_str(),
                intfname.data(),
                sCppProp.getStr(CppPropType::interfreftype).c_str(),
                sCppProp.mkIntfName(intfname).c_str(),
                usestr.c_str());

            c.nl();
        }

        c.nl();
    }

    void emitImpl(Code& c, EntityType ep)
    {
        if (mHeaderMode)
        {
            return;
        }

        std::string orgname = ep->getSimpIdentStr(ETag::ClassName);
        std::string classname = sCppProp.mkClassName(orgname);

        auto arr = ep->getChildren(ETag::Primary);
        for (size_t i = 0; i < arr.count(); i++)
        {
            EntityType e = arr.get(i);
            if (e->mKind == EKind::FuncBody)
            {
                emitFuncBody(c, e, &classname);
            }
        }
    }

    void emitTypeDef(Code& c, EntityType ep)
    {
        if (mHeaderMode)
        {
            if (ep->hasAttrib(EAttribFlags::a_cppobject))
            {
                std::string orgname = ep->getIdentStr(ETag::TypeName);

                std::string typestr;
                EntityType cppdirec = ep->getChild(EKind::CppDirec);
                if (cppdirec)
                {
                    typestr = cppdirec->getCppDirecStr(CppDirecType::type);
                }

                std::string declstr;
                std::string usestr;
                mkTemplate(ep, declstr, usestr);

                c.emitFmt("// typedef: %s\n", orgname.c_str());

                c.emitFmt("%susing %s = %s<%s%s>;",
                    declstr.c_str(),
                    orgname.c_str(),
                    sCppProp.getStr(CppPropType::strongreftype).c_str(),
                    typestr.c_str(),
                    usestr.c_str());
                c.nl();

                c.emitFmt("%susing %s = %s<%s%s>;",
                    declstr.c_str(),
                    sCppProp.mkWeak(orgname).c_str(),
                    sCppProp.getStr(CppPropType::weakreftype).c_str(),
                    typestr.c_str(),
                    usestr.c_str());
                c.nl();
            }
        }
    }

    void emitCppDirecInc(Code& c, EntityType ep)
    {
        if (mHeaderMode && mPubMode)
        {
            std::string inc = ep->getCppDirecStr(CppDirecType::include);
            if (!inc.empty())
            {
                emitNsClose(c);

                c.emit("#include ");
                if (inc[0] != '<')
                {
                    c.emit("\"");
                }
                c.emit(inc);
                if (inc[0] != '<')
                {
                    c.emit("\"");
                }
                c.nl();

                emitNsOpen(c);
            }
        }
    }

    void emitEntity(Code& c, EntityType ep)
    {
        //dbglog("Emitting kind:%s\n", sEKindMap.toString(ep->mKind));

        // Emit based on the kind of entity
        switch (ep->mKind)
        {
            case EKind::FileRoot:
                emitSubEntities(c, ep, ETag::Primary);
                break;
            case EKind::StmtBlock:
                emitStmtBlock(c, ep);
                break;
            case EKind::FuncBody:
                emitFuncBody(c, ep);
                break;
            case EKind::FuncCall:
                emitFuncCall(c, ep);
                break;
            case EKind::VarDecl:
                emitVarDecl(c, ep);
                break;
            case EKind::VarAssign:
                emitVarAssign(c, ep);
                break;
            case EKind::Operator:
                emitOperator(c, ep);
                break;
            case EKind::VarEval:
                emitVarEval(c, ep);
                break;
            case EKind::Expr:
                emitExpr(c, ep);
                break;
            case EKind::ParanExpr:
                emitParanExpr(c, ep);
                break;
            case EKind::DollarExpr:
                emitDollarExpr(c, ep);
                break;
            case EKind::FuncMapping:
                emitFuncMapping(c, ep);
                break;
            case EKind::ReturnStmt:
                emitReturnStmt(c, ep);
                break;
            case EKind::BreakStmt:
                emitBreakStmt(c, ep);
                break;
            case EKind::ContinStmt:
                emitContinStmt(c, ep);
                break;
            case EKind::ForStmt:
                emitForStmt(c, ep);
                break;
            case EKind::SwitchStmt:
                emitSwitchStmt(c, ep);
                break;
            case EKind::IfStmt:
                emitIfStmt(c, ep);
                break;
            case EKind::LoopStmt:
                emitLoopStmt(c, ep);
                break;
            case EKind::WhileStmt:
                emitWhileStmt(c, ep);
                break;
            case EKind::Literal:
                emitLiteral(c, ep);
                break;
            case EKind::Object:
                emitObject(c, ep);
                break;
            case EKind::Interf:
                emitInterf(c, ep);
                break;
            case EKind::Impl:
                emitImpl(c, ep);
                break;
            case EKind::New:
                emitNew(c, ep);
                break;
            case EKind::CppDirec:
                emitCppDirecInc(c, ep);
                break;
            case EKind::TypeDef:
                emitTypeDef(c, ep);
                break;
            case EKind::Deref:
                emitDeref(c, ep);
                break;

            // Not needed when called in this context
            case EKind::DeferStmt:
            case EKind::FuncParam:
            case EKind::Ident:
                break;

            default:
                pushErr(c, "Cannot emit kind %s", sEKindMap.toString(ep->mKind));
        }
    }

    void pushErr(Code& c, const char* fmt, ...)
    {
        std::string errmsg;
        va_list va;
        va_start(va, fmt);
        base::vformat(errmsg, fmt, va);
        va_end(va);

        c.emitln("//! ERROR: Codegen failure");
        c.emit("//! ");
        c.emitln(errmsg.c_str());

        dbgerr("Codegen failure: %s\n", errmsg.c_str());
    }

    bool generate(EntityType root, Code& c)
    {
        if (!root)
        {
            return false;
        }

        // Include unitwide header
        if (!mPubMode)
        {
            c.emit("#include \"");
            c.emit(sCppProp.getStr(CppPropType::unitwidehdrfn));
            c.emit("\"");
            c.nl();
            c.nl();
        }

        emitNsOpen(c);

        // Emit the entitites
        emitEntity(c, root);

        emitNsClose(c);

        return true;
    }
};


bool plGenerate(PlUnit* unit, EntityType root, OutputFmt fmt, base::Buffer& buf)
{
    CppCode genout;
    if (!root)
    {
        return false;
    }

    // For libs, setup a CPP namespace
    if (unit->mType == UnitType::lib)
    {
        genout.mNamespace = unit->cppNamespace();
    }
    genout.mUnit = unit;

    switch (fmt)
    {
        case OutputFmt::cpp:
        {
            Code c;
            genout.generate(root, c);
            c.moveToBuffer(buf);
        }
        break;

        case OutputFmt::header:
        case OutputFmt::pubheader:
        {
            Code c;
            c.emitln("#pragma once");
            genout.mPubMode = (fmt == OutputFmt::pubheader);
            genout.mHeaderMode = true;
            genout.generate(root, c);
            c.moveToBuffer(buf);
        }
        break;

        default:
        break;
    }

    return true;
}


// Code::
void Code::braceOpen()
{
    if (!mAtNL)
    {
        nl();
    }
    emit("{");
    nl();
}

void Code::braceClose(bool autonl)
{
    if (autonl)
    {
        if (last() != '\n')
        {
            nl();
        }
    }
    emit("}");
    if (autonl)
    {
        nl();
    }
}

void Code::emit(std::string_view txt, bool autosp)
{
    constDef NoSpFirst = "(:;\"), ";
    constDef NoSpLast = "(:;\"{} ";

    if (mAtNL)
    {
        append(mIndent, ' ');
        mAtNL = false;
    }
    else if (autosp && !txt.empty())
    {
        char first = txt.front();
        if ((strchr(NoSpFirst, first) == NULL && strchr(NoSpLast, last()) == NULL))
        {
            appendc(' ');
        }
    }
    append(txt);
}

void Code::emitFmt(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    (void)appendVFmt(fmt, va);
    va_end(va);
}

