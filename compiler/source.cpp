#include "primalc.h"


class PlSourceParser : public PlTokenParser, PlDiscern
{
public:
    PlSourceParser() = delete;
    PlSourceParser(PlSymbolTable* symtable, PlUnit* unit) :
        mSymTable(symtable),
        mUnit(unit),
        mRoot(nullptr),
        mLoopDepth(0),
        mStmtBlockDepth(0)
    {
    }
    ~PlSourceParser() = default;
    bool inErr()
    {
        return PlTokenParser::inErr();
    }
    PlSymbolTable* symTable()
    {
        return mSymTable;
    }
    PlUnit* unitPtr()
    {
        return mUnit;
    }

private:
    // peekXXX functions

    std::string peekIdent()
    {
        Checkpoint chk(this);
        std::string ret;
        uint d = 0;
        //while (avail())
        size_t lin = curLine();
        while (availForLine(lin))
        {
            std::string s = token().toString();
            if (!isSimpleIdent(s))
            {
                ret.clear();
                break;
            }
            ret += s;
            advance();
            d++;
            if (d < C_MAXIDENTSCOPE)
            {
                if (is("."))
                {
                    ret += '.';
                    advance();
                    continue;
                }
            }
            break;
        }
        chk.restore();
        return ret;
    }

    // Return next 2 items on the same line if available, an item can either be a token
    // or an identifier with scoping periods. Parser state is not changed.
    void peekNext2(std::string& s1, std::string& s2, bool* is1ident, bool* is2ident)
    {
        Checkpoint chk(this);

        size_t lin = curLine();
        for (int i = 0; i < 2; i++)
        {
            std::string item;
            bool isident = false;
            uint d = 0;
            while (availForLine(lin))
            {
                std::string s = token().toString();
                if (!isSimpleIdent(s))
                {
                    if (item.empty())
                    {
                        item = s;
                    }
                    advance();
                    break;
                }
                isident = true;
                item += s;
                advance();
                d++;
                if (d < C_MAXIDENTSCOPE)
                {
                    if (is("."))
                    {
                        item += '.';
                        advance();
                        continue;
                    }
                }
                break;
            }

            if (i == 0)
            {
                s1 = item;
                *is1ident = isident;
            }
            else
            {
                s2 = item;
                *is2ident = isident;
            }
        }

        chk.restore();
    }

    void parseIdent(EntityType parent, ETag etag, bool pub)
    {
        EntityType en;
        uint d = 0;
        //while (avail())
        size_t lin = curLine();
        while (availForLine(lin))
        {
            if (!isSimpleIdent(token().toString()))
            {
                pushErr("Invalid identifier");
                break;
            }
            en = createPubEn(parent, EKind::Ident, etag, pub);
            advance();
            d++;
            if (d < C_MAXIDENTSCOPE)
            {
                if (is("."))
                {
                    advance();
                    continue;
                }
            }
            break;
        }
    }

    EntityType parseSimpleIdent(EntityType parent, ETag etag, bool pub)
    {
        if (!isSimpleIdent(token().toString()))
        {
            pushErr("Invalid identifier");
            return nullptr;
        }
        EntityType en = createPubEn(parent, EKind::Ident, etag, pub);
        advance();
        return en;
    }

    void parseTemplArg(EntityType parent, bool pub)
    {
        // Template argument
        // TODO: add arbitary number and complexity of template arg
        if (is("<"))
        {
            advance();
            parseIdent(parent, ETag::TemplArg, pub);
            advance(">");

            // Mark this type is templated
            parent->addAttrib(EAttribFlags::a_templtype);

            PlDiscern dc;
            if (dc.isValueType(parent->getIdentStr(ETag::TemplArg)))
            {
                parent->addAttrib(EAttribFlags::a_boxed);
            }

        }
    }

    void parseVarType(EntityType parent, ETag etag, bool pub)
    {
        std::string item1;
        std::string item2;
        bool is1ident = false;
        bool is2ident = false;
        peekNext2(item1, item2, &is1ident, &is2ident);

        // If type has template args, create a new typedef with name containing hash codes of temp args.
        // Add the typedef at root level, if doesn't exist, and add actual type info as children to typedef
        // Use the new typedef as the type here

        if (is1ident && isVal(item2, "<"))
        {
            // Create a temorary entity
            EntityType temp = new PlEntity();
            temp->mKind = EKind::FileRoot;

            // Parse elements into a temporary entity
            EntityType ten = createNilPubEn(temp, EKind::TypeDef, ETag::Primary, pub);
            parseIdent(ten, etag, pub);
            parseTemplArg(ten, pub);

            bool boxed = ten->hasAttrib(EAttribFlags::a_boxed);
            std::string genname;
            base::StrBld nm;

            // Build a hashed name that contains type and template info
            auto id = ten->getChildren(EKind::Ident, etag);
            for (size_t i = 0; i < id.count(); i++)
            {
                nm.append(id.get(i)->getStr());
                nm.appendc('_');

                genname.assign(id.get(i)->getStr());
            }

            // Read template args, and also copy them into parent as they
            // are expected by code generation
            auto tid = ten->getChildren(EKind::Ident, ETag::TemplArg);
            for (size_t i = 0; i < tid.count(); i++)
            {
                std::string s = tid.get(i)->getStr();
                if (nm.last() != '_')
                {
                    nm.appendc('_');
                }
                nm.appendFmt("%d", base::strHash(s));

                PlEntity::cloneEntity(tid.get(i), parent, ETag::TemplArg);
            }
            std::string hname = nm.toString();

            EntityType gen = createNilPubEn(ten, EKind::Generic, ETag::Primary, pub);
            gen->setTokStr(genname.c_str());

            // Set the new entity's name to be hashed name
            ten->setTokStr(hname);

            // Look for the element in root and if not found, add it by cloning the temp
            auto arr = mRoot->getChildren(EKind::TypeDef, ETag::Primary);
            EntityType found = nullptr;
            for (size_t i = 0; i < arr.count(); i++)
            {
                if (arr.get(i)->mToken.equals(hname.c_str()))
                {
                    found = arr.get(i);
                    break;
                }
            }
            // TODO: If found, vaidate consistincy of temp arguments
            if (found == nullptr)
            {
                ten = PlEntity::cloneEntity(ten, mRoot);

                // Add to symbol
                mSymTable->addSymbol(hname.c_str(), ten, mUnit);
            }
            delete temp;

            // Set type as the hashed name
            EntityType t = createNilPubEn(parent, EKind::Ident, etag, pub);
            t->setTokStr(hname);

            if (boxed)
            {
                parent->addAttrib(EAttribFlags::a_boxed);
            }
        }
        else
        {
            parseIdent(parent, etag, pub);
            parseTemplArg(parent, pub);
        }
    }

    EntityType parseCppDirec(EntityType parent, CppDirecType mustbecdtype, bool pub)
    {
        advance(L_CPPDIREC);

        EntityType en = createNilPubEn(parent, EKind::CppDirec, ETag::Primary, pub);
        advance(".");

        PlSymbolAg symag(false, symTable(), unitPtr());
        CppDirecType cdtype;
        if (sCppDirecTypeMap.fromString(token().toString(), &cdtype) && cdtype != CppDirecType::none)
        {
            if (mustbecdtype != CppDirecType::none && cdtype != mustbecdtype)
            {
                pushErr("Cpp directerive not allowed here, must be %s", sCppDirecTypeMap.toString(mustbecdtype));
                return nullptr;
            }

            en->mToken = token();
            advance();

            size_t n = 0;
            advance("(");
            while (avail())
            {
                if (is(")"))
                {
                    break;
                }
                parseLiteral(en, pub);

                if (is(L_COMMA))
                {
                    advance();
                }
                n++;
            }
            advance(")");
        }
        else
        {
            pushErr("Invalid Cpp directerive");
            return nullptr;
        }
        return en;
    }

    void parseObject(EntityType parent, bool pub)
    {
        advance(L_OBJECT);

        EntityType en = createNilPubEn(parent, EKind::Object, ETag::Primary, pub);
        en->addAttrib(EAttribFlags::a_methods);
        en->mDT = DataType::d_object;

        // Parse class identifier
        parseIdent(en, ETag::ClassName, pub);
        std::string name = en->getIdentStr(ETag::ClassName);

        expectNL();

        PlSymbolAg classsym(false, symTable(), unitPtr());

        // Now parse either a object body
        if (is("{"))
        {
            // Add symbol and push scope
            classsym.process(en, name.c_str());

            advance("{");
            while (avail())
            {
                if (is("}"))
                {
                    break;
                }

                EntityType flden = createNilEn(en, EKind::ObjectFld);

                PlSymbolAg fldsym(false, symTable(), unitPtr());

                size_t lin = curLine();
                parseVarType(flden, ETag::VarType, false);
                expectLine(lin);
                parseIdent(flden, ETag::VarName, false);

                std::string fldname = flden->getIdentStr(ETag::VarName);

                fldsym.process(flden, fldname.c_str());

                expectNL();
            }
            advance("}");
        }
    }

    void parseImpl(EntityType parent, bool pub)
    {
        advance(L_IMPL);

        EntityType en = createNilPubEn(parent, EKind::Impl, ETag::Primary, pub);

        // Two cases:
        //    1) impl classname
        //    2) impl intfname for classname
        std::string item1;
        std::string item2;
        bool is1ident = false;
        bool is2ident = false;
        peekNext2(item1, item2, &is1ident, &is2ident);

        if (is1ident && isVal(item2, L_FOR))
        {
            parseIdent(en, ETag::InterfName, pub);
            advance(L_FOR);
        }
        parseIdent(en, ETag::ClassName, pub);
        std::string name = en->getIdentStr(ETag::ClassName);

        expectNL();

        PlSymbolAg classsym(false, symTable(), unitPtr());

        // Now parse either a object body
        if (is("{"))
        {
            // Add symbol and push scope
            classsym.process(en, name.c_str());

            advance("{");
            while (avail())
            {
                if (is("}"))
                {
                    break;
                }

                if (is(L_FUNC))
                {
                    parseFunc(en, false);
                }
                else
                {
                    pushErr("Not allowed inside 'impl'");
                }
            }
            advance("}");
        }
    }

    void parseIntf(EntityType parent, bool pub)
    {
        advance(L_INTERF);
        EntityType en = createNilPubEn(parent, EKind::Interf, ETag::Primary, pub);
        en->addAttrib(EAttribFlags::a_methods);
        en->mDT = DataType::d_object;

        // Parse interface identifier
        parseIdent(en, ETag::InterfName, pub);
        std::string name = en->getIdentStr(ETag::InterfName);

        PlSymbolAg classsym(false, symTable(), unitPtr());

        // Template arg
        parseTemplArg(en, pub);

        expectNL();

        // Now parse either a object body
        if (is("{"))
        {
            // Add symbol and push scope
            classsym.process(en, name.c_str());

            advance("{");
            while (avail())
            {
                if (is("}"))
                {
                    break;
                }

                if (is(L_FUNC))
                {
                    // Parse function definition
                    parseFunc(en, false, true);
                }
                else
                {
                    pushErr("Not allowed inside 'impl'");
                }
            }
            advance("}");
        }
    }

    void parseFunc(EntityType parent, bool pub, bool defonly = false)
    {
        advance(L_FUNC);

        // Assume it is a function defintion, update to body or mapping later
        EntityType en = createNilPubEn(parent, defonly ? EKind::FuncDef : EKind::FuncBody, ETag::Primary, pub);

        // Parse function identifier
        parseIdent(en, ETag::FuncName, pub);
        std::string name = en->getIdentStr(ETag::FuncName);

        PlSymbolAg symag(false, symTable(), unitPtr());
        symag.process(en, name.c_str());

        // Parse params
        advance("(");
        parseParamList(en, pub, defonly ? nullptr : &symag);
        advance(")");

        // Parse return value, if it exists
        if (is(L_PTR))
        {
            advance();
            parseIdent(en, ETag::ReturnType, pub);
        }

        expectNL();

        if (defonly)
        {
            return;
        }

        // Now parse either a function body or a function mapping
        if (is("{"))
        {
            // Function body, parse statements
            en->updateKind(EKind::FuncBody);

            parseStmtBlock(en, false);
        }
        else if (is(L_MAPSTO))
        {
            // Function mapping
            en->updateKind(EKind::FuncMapping);
            advance(L_MAPSTO);
            parseCppDirec(en, CppDirecType::function, false);
        }
    }

    void parseParamList(EntityType parent, bool pub, PlSymbolAg* symag)
    {
        size_t lin = curLine();
        while (availForLine(lin))
        {
            if (is(")"))
            {
                break;
            }

            EntityType en = createNilEn(parent, EKind::FuncParam);

            parseVarType(en, ETag::ParamType, pub);

            if (is(L_AMP))
            {
                advance(L_AMP);
                en->addAttrib(EAttribFlags::a_inout);
            }
            parseIdent(en, ETag::ParamName, pub);

            if (symag)
            {
                std::string name = en->getIdentStr(ETag::ParamName);
                symag->addSym(en, name.c_str());
            }

            if (is(L_ELLIPSES))
            {
                advance();
                // Mark the func and param as vararg
                en->addAttrib(EAttribFlags::a_ellipses);
                parent->addAttrib(EAttribFlags::a_ellipses);
            }

            if (is(L_COMMA))
            {
                advance();
                if (is(")"))
                {
                    pushErr("')' not expected after ','");
                }
            }
        }
    }

    void parseForStmt(EntityType parent)
    {
        PlSymbolAg symag(true, symTable(), unitPtr());

        // for i : range(0, count)
        // for b : rev rangeinc(0, n)
        advance(L_FOR);

        EntityType en = createNilEn(parent, EKind::ForStmt);

        std::string scname = base::formatr("_for_%d", token().mBegLine);
        en->setTokStr(scname.c_str());

        symag.process(en, scname.c_str());

        EntityType autoen = createEn(en, EKind::Ident, ETag::VarType);
        autoen->setTokStr(sCppKeywordMap.toString(CppKeyword::cpp_auto));

        // Add the for variable to symbol table.  The symbol doesn't point
        // to the variable entity, it needs to point to the for statement entity
        // which is the var's decl entity in this case.
        EntityType ven = parseSimpleIdent(en, ETag::ForVar, false);
        std::string varname = en->getIdentStr(ETag::ForVar);
        symag.addSym(en, varname.c_str());

        advance(":");

        bool rev = false;
        if (is(L_REV))
        {
            rev = true;
            advance();
        }

        if (is(L_RANGE) || is(L_RANGEINC))
        {
            bool inc = is(L_RANGEINC);
            EntityType rangeen = createNilEn(en, EKind::Range);
            advance();
            if (rev)
            {
                rangeen->addAttrib(EAttribFlags::a_reverse);
            }
            if (inc)
            {
                rangeen->addAttrib(EAttribFlags::a_inclusive);
            }

            advance("(");
            parseExpr(rangeen, ETag::RangeFromVal);
            advance(L_COMMA);
            parseExpr(rangeen, ETag::RangeToVal);
            advance(")");
        }
        else if (is(L_ITER))
        {
            EntityType iteren = createNilEn(en, EKind::Iter);
            advance();
            if (rev)
            {
                iteren->addAttrib(EAttribFlags::a_reverse);
            }

            advance("(");
            parseExpr(iteren, ETag::IterVal);
            advance(")");
        }
        else
        {
            pushErr("Invalid element in 'for' statement");
        }

        //en->dbgDump("FOR ");
        expectNL();
        mLoopDepth++;
        parseStmtBlock(en, false);
        mLoopDepth--;
    }

    void parseLoopStmt(EntityType parent)
    {
        advance(L_LOOP);
        EntityType en = createNilEn(parent, EKind::LoopStmt);
        expectNL();
        mLoopDepth++;
        parseStmtBlock(en, true);
        mLoopDepth--;
    }

    void parseDeferStmt(EntityType parent)
    {
        if (mStmtBlockDepth <= 1)
        {
            advance(L_DEFER);
            EntityType en = createSingleEn(parent, EKind::DeferStmt);
            expectNL();
            parseStmtBlock(en, true);
        }
        else
        {
            pushErr("'defer' only allowed at func scope");
        }
    }

    void parseWhileStmt(EntityType parent)
    {
        advance(L_WHILE);
        EntityType en = createNilEn(parent, EKind::WhileStmt);
        parseExpr(en, ETag::CondVal);
        expectNL();
        mLoopDepth++;
        parseStmtBlock(en, true);
        mLoopDepth--;
    }

    void parseContinStmt(EntityType parent)
    {
        if (mLoopDepth > 0)
        {
            advance(L_CONTINUE);
            EntityType en = createNilEn(parent, EKind::ContinStmt);
            if (is(L_IF))
            {
                advance(L_IF);
                parseExpr(en, ETag::CondVal);
            }
        }
        else
        {
            pushErr("'continue' only allowed inside a loop");
        }
    }

    void parseBreakStmt(EntityType parent)
    {
        if (mLoopDepth > 0)
        {
            advance(L_BREAK);
            EntityType en = createNilEn(parent, EKind::BreakStmt);
            if (is(L_IF))
            {
                advance(L_IF);
                parseExpr(en, ETag::CondVal);
            }
        }
        else
        {
            pushErr("'break' only allowed inside a loop");
        }
    }

    void parseCaseStmt(EntityType parent, bool* foundellipses = nullptr)
    {
        advance(L_CASE);
        EntityType en = createNilEn(parent, EKind::Case);

        if (foundellipses)
        {
            *foundellipses = false;
        }

        size_t lin = curLine();
        while (availForLine(lin))
        {
            EntityType caen = createEn(en, EKind::Literal, ETag::CaseVal);
            if (is(L_ELLIPSES))
            {
                caen->addAttrib(EAttribFlags::a_ellipses);
                advance();
                if (foundellipses)
                {
                    *foundellipses = true;
                }
                break;
            }
            else
            {
                advance();
            }

            if (is(L_COMMA))
            {
                advance();
            }
        }
        expectNL();
        parseStmtBlock(en, true);
    }

    void parseSwitchStmt(EntityType parent)
    {
        advance(L_SWITCH);
        EntityType en = createNilEn(parent, EKind::SwitchStmt);
        parseExpr(en, ETag::SwitchVal);
        expectNL();

        bool foundellipses = false;

        advance("{");
        while (avail())
        {
            if (is(L_CASE))
            {
                parseCaseStmt(en, &foundellipses);
                if (foundellipses)
                {
                    break;
                }
            }
            else if (is("}"))
            {
                break;
            }
            else
            {
                pushErr("Invalid element inside switch");
                break;
            }
            expectNL();
        }
        advance("}");
    }

    void parseIfStmt(EntityType parent)
    {
        advance(L_IF);
        EntityType en = createNilEn(parent, EKind::IfStmt);
        parseExpr(en, ETag::CondVal);
        expectNL();
        parseStmtBlock(en, true);
        expectNL();
        for (;;)
        {
            if (is(L_ELSE))
            {
                advance(L_ELSE);
                EntityType elen = nullptr;
                if (is(L_IF))
                {
                    advance(L_IF);
                    elen = createNilEn(en, EKind::ElseIf);

                    parseExpr(elen, ETag::CondVal);
                }
                else
                {
                    elen = createNilEn(en, EKind::Else);
                }
                expectNL();
                parseStmtBlock(elen, true);
            }
            else
            {
                break;
            }
        }
    }

    void parseStmtBlock(EntityType parent, bool scope)
    {
        PlSymbolAg symag(true, symTable(), unitPtr());

        advance("{");
        EntityType en = createNilEn(parent, EKind::StmtBlock);
        if (scope)
        {
            std::string scname = base::formatr("_sb_%d", token().mBegLine);
            en->setTokStr(scname.c_str());

            symag.process(en, scname.c_str());
        }

        mStmtBlockDepth++;
        while (avail())
        {
            if (is("}"))
            {
                break;
            }
            parseStmt(en);
            expectNL();
        }
        advance("}");
        mStmtBlockDepth--;
    }

    void parseStmt(EntityType parent)
    {
        if (is(L_RETURN))
        {
            parseReturnStmt(parent);
        }
        else if (is(L_FOR))
        {
            parseForStmt(parent);
        }
        else if (is(L_WHILE))
        {
            parseWhileStmt(parent);
        }
        else if (is(L_LOOP))
        {
            parseLoopStmt(parent);
        }
        else if (is(L_DEFER))
        {
            parseDeferStmt(parent);
        }
        else if (is(L_IF))
        {
            parseIfStmt(parent);
        }
        else if (is(L_CONTINUE))
        {
            parseContinStmt(parent);
        }
        else if (is(L_BREAK))
        {
            parseBreakStmt(parent);
        }
        else if (is(L_SWITCH))
        {
            parseSwitchStmt(parent);
        }
        else if (is(L_VAR))
        {
            parseVarDecl(parent);
        }
        else if (is("{"))
        {
            parseStmtBlock(parent, true);
        }
        else
        {
            bool deref = false;
            while (avail())
            {
                // Peek the next to items (identifiers or tokens) to determine
                // if we are are looking at a function call, var decl or assignemnt
                std::string item1;
                std::string item2;
                bool is1ident = false;
                bool is2ident = false;
                peekNext2(item1, item2, &is1ident, &is2ident);

                // When there are no keywords to detect the type of statement, we
                // use following pattern:
                //    <Ident> ( ::= <FuncCall>>
                //    <Ident> <AssignOp> ::= <VarAssign>
                // TODO: Instead of identifier, needs to handle values with [] etc and proper rvalues

                if (is1ident && isAssignOp(item2) && !deref)
                {
                    parseVarAssign(parent);
                }
                else if (is1ident && isVal(item2, "("))
                {
                    parseFuncCall(parent);
                    if (is("."))
                    {
                        createEn(parent, EKind::Deref);
                        advance();
                        deref = true;
                        continue;
                    }
                }
                else
                {
                    pushErr("Unrecognized statement [%s %s]", item1.c_str(), item2.c_str());
                }
                break;
            }
        }
    }

    void parseTypeDef(EntityType parent, bool pub)
    {
        advance(L_TYPEDEF);

        EntityType en = createNilPubEn(parent, EKind::TypeDef, ETag::Primary, pub);

        // Parse type identifier
        parseIdent(en, ETag::TypeName, pub);
        std::string name = en->getIdentStr(ETag::TypeName);

        PlSymbolAg symag(false, symTable(), unitPtr());

        if (is(L_MAPSTO))
        {
            symag.process(en, name.c_str());

            // Type def mapping
            advance(L_MAPSTO);

            if (is(L_CPPDIREC))
            {
                // Typedef points to a CPP type
                EntityType cppdirec = parseCppDirec(en, CppDirecType::type, false);
                if (cppdirec)
                {
                    // Cpp directrive type can specifiy additional info
                    //     index 0: cpp type to map to
                    //     index 1: name of a intf (set of methods) that map to cpp methods
                    //     index 2: flags separated by |, with one or more of following:
                    //                 template: allow template params like <T>
                    //                 cppobject: cpp class is an ref counted object
                    std::string cppintf = cppdirec->getCppDirecStr(CppDirecType::type, 1);
                    std::string cppflags = cppdirec->getCppDirecStr(CppDirecType::type, 2);

                    if (!cppintf.empty())
                    {
                        // Setup the scope for the the interface. "unitwide>.CppString>"
                        PlResolver::createSymScope(en, cppintf);

                        // Mark the type as having methods and as a cpp object
                        en->addAttrib(EAttribFlags::a_methods);
                        en->mDT = DataType::d_object;

                        // Process the flags
                        base::Splitter sp(cppflags, '|');
                        while (!sp.eof())
                        {
                            std::string f = sp.getStr();
                            if (base::strieql(f, "template"))
                            {
                                //en->addAttrib(EAttribFlags::a_templated);

                                // TODO: Finish template support. For now only a single argument T is allowed
                                EntityType t = createNilPubEn(en, EKind::Ident, ETag::TemplArg, pub);
                                t->setTokStr("T");
                            }
                            else if (base::strieql(f, "cppobject"))
                            {
                                en->addAttrib(EAttribFlags::a_cppobject);
                            }
                        }

                        if (!en->hasAttrib(EAttribFlags::a_cppobject))
                        {
                            en->addAttrib(EAttribFlags::a_noderef);
                        }
                    }
                }
            }
            else
            {
                // Typedef to another type, parsed as generic value
                parseGeneric(en, false);
            }
        }
    }

    void parseReturnStmt(EntityType parent)
    {
        advance(L_RETURN);
        EntityType en = createNilEn(parent, EKind::ReturnStmt);

        parseExprOrNew(en, ETag::ReturnVal);
    }

    void parseVarAssign(EntityType parent)
    {
        EntityType en = createNilEn(parent, EKind::VarAssign);

        parseIdent(en, ETag::VarName, false);

        parseOperator(en, ETag::AssignOp);
        std::string opstr = en->getChildStr(EKind::Operator, ETag::AssignOp);
        if (!isAssignOp(opstr))
        {
            pushErr("Invalid assignment operator: %s", opstr.c_str());
        }

        parseExprOrNew(en, ETag::AssignVal);
    }

    void parseLiteral(EntityType parent, bool pub)
    {
        createPubEn(parent, EKind::Literal, ETag::Primary, pub);
        advance();
    }

    void parseGeneric(EntityType parent, bool pub)
    {
        createPubEn(parent, EKind::Generic, ETag::Primary, pub);
        advance();
    }

    void parseOperator(EntityType parent, ETag etag)
    {
        createEn(parent, EKind::Operator, etag);
        advance();
    }

    EntityType parseNumber(EntityType parent, ETag etag)
    {
        EntityType en = nullptr;
        std::string num;
        if (is("-"))
        {
            num += "-";
            advance();
        }

        size_t lin = curLine();
        while (availForLine(lin))
        {
            if (is("."))
            {
                num += ".";
                advance();
                continue;
            }
            if (isNumPart(token().toString()))
            {
                num += token().toString();
                advance();
                continue;
            }
            break;
        }

        // Validate and convert to CPP numeric literal
        DataType dtype = DataType::d_none;
        if (!sCppProp.convValidNum(num, dtype))
        {
            pushErr("Invalid numeric literal %s", num.c_str());
        }
        else
        {
            en = createNilEn(parent, EKind::Literal, etag);
            en->setTokStr(num);
            en->mDT = dtype;
        }
        return en;
    }

    void parseFuncCall(EntityType parent)
    {
        EntityType en = createNilEn(parent, EKind::FuncCall);

        parseIdent(en, ETag::FuncName, false);

        int paramcnt = 0;
        advance("(");

        size_t lin = curLine();
        while (availForLine(lin))
        {
            if (is(")"))
            {
                break;
            }

            parseExprOrNew(en, ETag::FuncArgVal);

            paramcnt++;
            if (is(L_COMMA))
            {
                advance();
            }
        }
        advance(")");
    }

    void parseVarDecl(EntityType parent)
    {
        advance(L_VAR);

        EntityType en = createNilEn(parent, EKind::VarDecl);

        std::string item1;
        std::string item2;
        bool is1ident = false;
        bool is2ident = false;
        peekNext2(item1, item2, &is1ident, &is2ident);

        EntityType autoen = nullptr;
        if (is1ident && isVal(item2, "="))
        {
            // case 1: var varname = initval
            autoen = createEn(en, EKind::Ident, ETag::VarType);
            autoen->setTokStr(sCppKeywordMap.toString(CppKeyword::cpp_auto));

            parseIdent(en, ETag::VarName, false);
        }
        else if (item2.empty())
        {
            // case 2: var varname
            pushErr("var must specify type or initial value");
        }
        else
        {
            // case 3: var vartype varname [= initval]
            parseVarType(en, ETag::VarType, false);
            parseIdent(en, ETag::VarName, false);
        }

        std::string symname = PlSymbolAg::getSymName(en);
        PlSymbolAg symag(false, symTable(), unitPtr());
        symag.process(en, symname.c_str());

        if (is("="))
        {
            advance();
            EntityType initen = parseExprOrNew(en, ETag::DeclInitVal);

            if (autoen)
            {
                EntityType newen = nullptr;
                if (initen)
                {
                    newen = initen->getChild(EKind::New);
                }

                if (newen)
                {
                    // Change auto to type from the new expression
                    autoen->setTokStr(newen->getIdentStr(ETag::ClassName));

                    // Add vartype
                    auto arr = newen->getChildren(ETag::TemplArg);
                    for (size_t i = 0; i < arr.count(); i++)
                    {
                        EntityType te = arr.get(i);
                        EntityType e = PlEntity::newEntity(en, te->mKind, te->mToken, ETag::TemplArg);
                        e->mAttribFlags = te->mAttribFlags;
                        e->mDT = te->mDT;
                    }
                }
                else
                {
                    autoen->mDT = en->mDT;
                }
            }
        }
    }

    EntityType parseExprOrNew(EntityType parent, ETag etag)
    {
        EntityType en = nullptr;
        if (is(L_NEW))
        {
            en = createNilEn(parent, EKind::Expr, etag);
            EntityType newen = createNilEn(en, EKind::New);
            advance();
            parseVarType(newen, ETag::ClassName, false);
        }
        else
        {
            en = parseExpr(parent, etag);
        }
        return en;
    }

    EntityType parseExpr(EntityType parent, ETag etag)
    {
        EntityType en = createNilEn(parent, EKind::Expr, etag);

        DataType exdt = DataType::d_none;
        size_t lin = curLine();
        while (availForLine(lin))
        {
            // We stop parsing the expression upon hitting a , or a )
            if (is(L_COMMA) || (is(")")))
            {
                break;
            }

            if (is("("))
            {
                EntityType paren = createNilEn(en, EKind::ParanExpr);
                advance();
                parseExpr(paren, ETag::Primary);
                advance(")");

                continue;
            }

            if (is("["))
            {
                EntityType paren = createNilEn(en, EKind::SquareExpr);
                advance();
                parseExpr(paren, ETag::Primary);
                advance("]");

                continue;
            }

            std::string item1;
            std::string item2;
            bool is1ident = false;
            bool is2ident = false;
            peekNext2(item1, item2, &is1ident, &is2ident);

            if (is1ident && isVal(item2, "("))
            {
                parseFuncCall(en);
                continue;
            }

            if (isVal(item1, L_AMP))
            {
                if (is2ident)
                {
                    advance();
                    EntityType vareven = createNilEn(en, EKind::VarEval);
                    parseIdent(vareven, ETag::VarName, false);
                    en->addAttrib(EAttribFlags::a_inout);
                    continue;
                }
                else
                {
                    pushErr("& can only be used for a variable");
                }
            }

            if (isDollarOp(item1))
            {
                advance();
                EntityType unaren = createNilEn(en, EKind::DollarExpr);
                if (isVal(item2, "("))
                {
                    advance();
                    parseExpr(unaren, ETag::Primary);
                    advance(")");

                    continue;
                }

                // For operators like $, even if there is no paren, we want
                // to add parent around the term $ is specified for, so it can
                // be turned into a function call.
                std::string item1;
                std::string item2;
                bool is1ident = false;
                bool is2ident = false;
                peekNext2(item1, item2, &is1ident, &is2ident);

                if (is1ident && isVal(item2, "("))
                {
                    parseFuncCall(unaren);
                    continue;
                }
                if (is1ident)
                {
                    EntityType vareven = createNilEn(unaren, EKind::VarEval);
                    parseIdent(vareven, ETag::VarName, false);
                    continue;
                }
            }

            if (is1ident)
            {
                EntityType vareven = createNilEn(en, EKind::VarEval);
                parseIdent(vareven, ETag::VarName, false);
                continue;
            }

            if (!is1ident && !is2ident)
            {
                if (isNumStart(item1) || (isVal(item1, "-") && isNumStart(item2)))
                {
                    EntityType nen = parseNumber(en, ETag::Primary);
                    if (nen)
                    {
                        exdt = nen->mDT;
                    }

                    continue;
                }
            }

            if (is("."))
            {
                createEn(en, EKind::Deref);
                advance();
                continue;
            }

            if (isExprOp(item1))
            {
                parseOperator(en, ETag::Primary);
                continue;
            }

            if (isStrLit(item1))
            {
                // string literal
                EntityType lten = createEn(en, EKind::Literal);
                if (item1[0] == '\'')
                {
                    lten->mDT = DataType::d_char;
                }
                else
                {
                    lten->mDT = DataType::d_string;
                }

                exdt = lten->mDT;
                advance();
                continue;
            }

            //dbgDumpTokens();
            if (is("="))
            {
                pushErr("Assignement not allowed in an expression");
            }
            else
            {
                pushErr("Unrecognized expression term");
            }
        }

        parent->mDT = exdt;

        // TODO: If on same line, check for .method calls
        //dbgTok("@end parseExpr");

        return en;
    }

public:
    void parseRoot(EntityType parent)
    {
        mRoot = parent;
        while (avail())
        {
            if (isComment())
            {
                advance();
            }
            else if (is(L_FUNC))
            {
                parseFunc(parent, false);
            }
            else if (is(L_TYPEDEF))
            {
                parseTypeDef(parent, false);
            }
            else if (is(L_CPPDIREC))
            {
                parseCppDirec(parent, CppDirecType::none, false);
            }
            else if (is(L_OBJECT))
            {
                parseObject(parent, false);
            }
            else if (is(L_IMPL))
            {
                parseImpl(parent, false);
            }
            else if (is(L_INTERF))
            {
                parseIntf(parent, false);
            }
            else if (is(L_PUB))
            {
                // Pub
                advance(L_PUB);
                if (is(L_FUNC))
                {
                    parseFunc(parent, true);
                }
                else if (is(L_TYPEDEF))
                {
                    parseTypeDef(parent, true);
                }
                else if (is(L_CPPDIREC))
                {
                    parseCppDirec(parent, CppDirecType::none, true);
                }
                else if (is(L_OBJECT))
                {
                    parseObject(parent, false);
                }
                else if (is(L_IMPL))
                {
                    parseImpl(parent, false);
                }
                else if (is(L_INTERF))
                {
                    parseIntf(parent, true);
                }
                else
                {
                    pushErr("Not allowed to be marked 'pub'");
                    break;
                }
            }
            else if (is(L_VAR))
            {
                parseVarDecl(parent);
            }
            else
            {
                pushErr("Unrecognized root element");
            }
        }
    }

private:
    PlSymbolTable* mSymTable;
    PlUnit* mUnit;
    EntityType mRoot;
    int mLoopDepth;
    int mStmtBlockDepth;

}; // PlSourceParser class


bool plParseFile(const base::Path& sourcefn, PlSymbolTable* symtable, PlUnit* containingunit, EntityType root, base::StrBld* diag)
{
    PlSourceParser ep(symtable, containingunit);
    if (!ep.tokenize(sourcefn))
    {
        return false;
    }

    if (diag)
    {
        ep.dumpTokens(*diag);
    }

    ep.parseRoot(root);

    return !ep.inErr();
}

