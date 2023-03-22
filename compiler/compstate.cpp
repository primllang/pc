#include "primalc.h"

// PlCompState::

bool PlCompState::writeDiagInfo(const base::Path& fn)
{
    assert(mUnit);
    base::StrBld bld;
    base::Buffer buf;

    bld.append("==Unit Wide Symbols==\n");
    mSymTable.dumpSymbols(bld);

    bld.append("\n==Unit Wide Publics==\n");
    if (mPubsRoot)
    {
        mPubsRoot->dump("", ETag::Primary, bld);
    }

    for (base::Iter<PlFileState> fi; mDepUnits.forEach(fi); )
    {
        bld.appendFmt("\n==Dep Unit: %s==\n", fi->fname().c_str());
        if (fi->enRoot())
        {
            fi->enRoot()->dump("", ETag::Primary, bld);
        }
    }

    bld.moveToBuffer(buf);
    return plWrite("Unit wide diag info", buf, fn);
}

void PlCompState::addSrc(base::Path srcname)
{
    assert(mUnit);
    PlFileState* fi = new (mSrcFiles.allocBack()) PlFileState();
    fi->init(this, mUnit, srcname, mUnit->mBldDiagFiles);
}

void PlCompState::addDep(PlUnit* pu)
{
    // assert(mUnit && pu);
    PlFileState* fi = new (mDepUnits.allocBack()) PlFileState();
    fi->init(this, pu, pu->unitAstFn(), mUnit->mBldDiagFiles);

    // Create an entity to represent the dep unit and add it to symbol table
    PlToken tok;
    tok.setStr(pu->mName.c_str());
    fi->mDepEn = PlEntity::newEntity(nullptr, EKind::FileRoot, tok);

    std::string sc;
    std::string ns = pu->mNS;
    if (ns.empty())
    {
        ns = pu->mName;
    }
    mSymTable.scAdd(sc, L_UNITDEPSSCOPE);
    mSymTable.addSymbolSc(sc, ns.c_str(), fi->mDepEn, pu);
}

bool PlCompState::compileSrc()
{
    assert(mUnit);

    // Compile or load representations for source files
    for (base::Iter<PlFileState> fi; mSrcFiles.forEach(fi); )
    {
        base::Path fn(fi->fname());
        bool compile = fi->isMod();
        if (!compile)
        {
            // Source file doesn't need to be compiled, load the stored representation
            // If we cannot load, we will need to compile.
            if (!fi->loadPrecomp(mUnit->srcAstFn(fn)))
            {
                compile = true;
            }
        }

        if (compile)
        {
            // Compile the source file and generate various files
            base::deleteFile(mUnit->srcAstFn(fn));

            if (!fi->compileSrc(mUnit->srcFn(fn)))
            {
                return false;
            }
        }
    }
    return true;
}

bool PlCompState::loadUnits()
{
    assert(mUnit);
    bool ret = true;

    // Load representations from dependant units
    for (base::Iter<PlFileState> fi; mDepUnits.forEach(fi); )
    {
        dbglog("loadUnit '%s' (%s) ns=%s\n", fi->fname().c_str(), fi->containingUnit()->mName.c_str(), fi->containingUnit()->mNS.c_str());

        // If the unit's namespace is global, load it at global level, otherwise
        // push the ns as a scope
        bool usescope = !base::streql(fi->containingUnit()->mNS, L_GLOBALNAMESPACE);
        if (usescope)
        {
            mSymTable.pushScope(fi->containingUnit()->mNS.c_str());
        }
        if (!fi->loadPrecomp(fi->fname()))
        {
            ret = false;
        }
        fi->readCfg(true);

        if (usescope)
        {
            mSymTable.popScope();
        }
    }

    return true;
}

void PlCompState::resolveSrcEntitites()
{
    assert(mUnit);

    // Prepare by setting up the env such as cpp properties
    for (base::Iter<PlFileState> fi; mSrcFiles.forEach(fi); )
    {
        fi->readCfg();
    }

    // Resolve each source file
    for (base::Iter<PlFileState> fi; mSrcFiles.forEach(fi); )
    {
        if (fi->enRoot())
        {
            base::Path fn = mUnit->srcFn(fi->fname());
            mUnit->mErrCol.begCtx(fn);
            mResolver.fixupAll(fi->enRoot());
            size_t ec = mUnit->mErrCol.endCtx();
            if (ec > 0)
            {
                fi->setErr();
            }
        }
    }

    // Validate each source file
    for (base::Iter<PlFileState> fi; mSrcFiles.forEach(fi); )
    {
        if (fi->enRoot())
        {
            base::Path fn = mUnit->srcFn(fi->fname());
            mUnit->mErrCol.begCtx(fn);

            mResolver.validateAll(fi->enRoot(), ETag::Primary);
            size_t ec = mUnit->mErrCol.endCtx();
            if (ec > 0)
            {
                fi->setErr();
            }
        }
    }
}

bool PlCompState::gatherAllPubs()
{
    assert(mUnit);
    if (mPubsRoot)
    {
        delete mPubsRoot;
    }

    PlToken tok;
    tok.setStr("Unit Publics");
    mPubsRoot = PlEntity::newEntity(nullptr, EKind::FileRoot, tok);

    for (base::Iter<PlFileState> fi; mSrcFiles.forEach(fi); )
    {
        if (fi->enRoot())
        {
            fi->enRoot()->gatherPubs(mPubsRoot, ETag::Primary, false, mUnit->mErrCol);
        }
    }

    // Resolve dep unit files
    for (base::Iter<PlFileState> fi; mDepUnits.forEach(fi); )
    {
        if (fi->enRoot())
        {
            mResolver.fixupAll(fi->enRoot());
        }
    }

    // Resolve public entity root
    if (mPubsRoot)
    {
        mResolver.fixupAll(mPubsRoot);
    }

    return true;
}

bool PlCompState::generateFile(OutputFmt fmt, const base::Path& filename, base::Buffer* outbuf)
{
    bool ret = false;
    base::Buffer buf;
    if (mPubsRoot)
    {
        switch (fmt)
        {
            case OutputFmt::cpp:
            case OutputFmt::header:
            case OutputFmt::pubheader:
            {
                (void)plGenerate(mUnit, mPubsRoot, fmt, buf);
            }
            break;

            case OutputFmt::entity:
            {
                base::PersistWr pwr(L_ENTITYFILESIG);
                mPubsRoot->saveEntity(pwr);
                pwr.moveToBuffer(buf);
            }
            break;

            default:
            break;
        }
    }
    // If a filename is provided, write the buffer to the file
    if (!filename.empty())
    {
        ret = plWrite(sOutputFmtMap.toString(fmt), buf, filename);
    }

    // If specified, copy the buffer to the out
    if (outbuf)
    {
        outbuf->moveFrom(buf);
        ret = true;
    }
    return ret;
}


// PlFileState::

PlArr<EntityType> PlFileState::getRootChildren(EKind ek, ETag tg)
{
    if (enRoot())
    {
        return enRoot()->getChildren(ek, tg);
    }
    return PlArr<EntityType>(0);
}

void PlFileState::readCppDirec(EntityType ce)
{
    if (ce->mToken.equals(sCppDirecTypeMap.toString(CppDirecType::setprop)))
    {
        std::vector<std::string> strs;
        ce->getChildrenStr(EKind::Literal, ETag::Primary, strs);
        if (strs.size() == 2)
        {
            //dbglog("CppProp: %s = %s\n", strs[0].c_str(), strs[1].c_str());
            sCppProp.setStr(strs[0], strs[1]);
        }
    }
}

void PlFileState::readCfg(bool multiroot)
{
    //dbglog("## prep src:%s\n", Fn().c_str());

    if (multiroot)
    {
        PlArr<EntityType> fr = getRootChildren(EKind::FileRoot);
        for (size_t f = 0; f < fr.count(); f++)
        {
            EntityType fre = fr.get(f);
            PlArr<EntityType> rootch = fre->getChildren(EKind::CppDirec);
            for (size_t i = 0; i < rootch.count(); i++)
            {
                EntityType ce = rootch.get(i);
                readCppDirec(ce);
            }
        }
    }
    else
    {
        PlArr<EntityType> rootch = getRootChildren(EKind::CppDirec);
        for (size_t i = 0; i < rootch.count(); i++)
        {
            EntityType ce = rootch.get(i);
            readCppDirec(ce);
        }
    }
}

bool PlFileState::compileSrc(const base::Path& sourcefn)
{
    assert(mContainingUnit);
    assert(mCompState);

    dbglog("compileSrc '%s' (%s)\n", sourcefn.c_str(), mContainingUnit->mName.c_str());

    mEnRoot = new PlEntity();
    mEnRoot->mKind = EKind::FileRoot;
    mEnRoot->mToken.setStr(sourcefn.c_str());
    mEnRoot->addAttrib(EAttribFlags::a_public);

    if (!plParseFile(sourcefn, symTable(), containingUnit(), mEnRoot, (mDiag ? &mTokDiag : nullptr)))
    {
        mState = State::Error;
        dbgerr("Parse pass failed\n");
        return false;
    }
    mState = State::Compiled;
    return true;
}

bool PlFileState::loadPrecomp(const base::Path& entfn)
{
    assert(mContainingUnit);
    dbglog("loadPrecomp '%s' (%s)\n", entfn.c_str(), mContainingUnit->mName.c_str());

    base::PersistRd prd(L_ENTITYFILESIG);
    if (prd.load(entfn))
    {
        mEnRoot = loadEntity(nullptr, prd);
        if (!mEnRoot)
        {
            mState = State::Error;
            return false;
        }

        mEnRoot->updateSymTbl(ETag::Primary, symTable(), containingUnit());

    }
    mState = State::AstLoaded;
    return true;
}

void PlFileState::genDiagInfo(base::StrBld& bld)
{
    if (mEnRoot)
    {
        symTable()->dumpSymbols(bld);

        bld.append("\n\n==Parse Tree==\n");

        // dbglog("%s ", mSrcName.c_str());
        // mEnRoot->dbgDump("Root from diag:\n");

        mEnRoot->dump("", ETag::Primary, bld);

        // bld.append("\n==Publics==\n");
        // base::Variant v;
        // base::Buffer b;
        // mEnRoot->toVar(v, true);
        // v.getYaml(b);
        // bld.append((const char*)b.cptr(), b.size());
    }

    bld.append("\n==Tokens==\n");
    bld.append(mTokDiag);
}

PlSymbolTable* PlFileState::symTable()
{
    return mCompState ? mCompState->symTable() : nullptr;
}


EntityType PlFileState::loadEntity(EntityType parent, base::PersistRd& prd, ETag etag)
{
    // IMPORTANT: Must match PlEntity::saveEntity
    EntityType en = nullptr;

    en = PlEntity::read(parent, prd, etag);
    if (!en)
    {
        dbgerr("loadEntity() failed to create and read entity\n");
        return nullptr;
    }

    auto subentarrlen = prd.rdArr();
    for (uint32 i = 0; i < subentarrlen; i++)
    {
        if (prd.inErr())
        {
            return nullptr;
        }

        ETag tag = (ETag)prd.rdUInt32();
        auto entarrlen = prd.rdArr();
        for (uint32 i = 0; i < entarrlen; i++)
        {
            EntityType ret = loadEntity(en, prd, tag);
            if (!ret)
            {
                return nullptr;
            }
            prd.endElem();
        }

        prd.endElem();
    }

    return en;
}

bool PlFileState::generateFile(OutputFmt fmt, const base::Path& filename, base::Buffer* outbuf)
{
    bool ret = false;
    base::Buffer buf;

    if (!mEnRoot)
    {
        dbgerr("Cannot generate file because obj not init\n");
        return false;
    }
    switch (fmt)
    {
        case OutputFmt::cpp:
        case OutputFmt::header:
        case OutputFmt::pubheader:
        {
            (void)plGenerate(mContainingUnit, mEnRoot, fmt, buf);
        }
        break;

        case OutputFmt::entity:
        {
            base::PersistWr pwr(L_ENTITYFILESIG);
            mEnRoot->saveEntity(pwr);
            pwr.moveToBuffer(buf);
        }
        break;

        case OutputFmt::diag:
        {
            base::StrBld c;
            genDiagInfo(c);
            c.moveToBuffer(buf);
        }
        break;
    }

    // If a filename is provided, write the buffer to the file
    if (!filename.empty())
    {
        ret = plWrite(sOutputFmtMap.toString(fmt), buf, filename);
    }

    // If specified, copy the buffer to the out
    if (outbuf)
    {
        outbuf->moveFrom(buf);
        ret = true;
    }

    return ret;
}
