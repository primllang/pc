#include "primalc.h"

// PlEntity::

EntityType PlEntity::newEntity(EntityType parent, EKind ekind, const PlToken& tok, ETag tg)
{
    //dbglog("add type=%s group=%s tok=%s\n", sEKindMap.toString(ekind), sETagMap.toString(tg), tok.toString().c_str());
    EntityType en = nullptr;

    if (parent)
    {
        for (SubEntity& sub : parent->mSubEntities)
        {
            if (sub.mTag == tg)
            {
                // Add to existing tag
                en = &sub.mEntities.emplace_back();
            }
        }
        if (!en)
        {
            // Add to a new tag
            SubEntity& sub = parent->mSubEntities.emplace_back();
            sub.mTag = tg;

            en = &sub.mEntities.emplace_back();
        }
    }
    else
    {
        en = new PlEntity();
    }
    if (en)
    {
        en->mKind = ekind;
        en->mToken = tok;
    }
    else
    {
        dbgerr("Failed to create entity\n");
    }
    return en;
}

EntityType PlEntity::newOrFindEntity(EntityType parent, const char* tokstr, EKind ekind, ETag etag)
{
    auto arr = parent->getChildren(ekind, etag);
    for (size_t i = 0; i < arr.count(); i++)
    {
        if (arr.get(i)->mToken.equals(tokstr))
        {
            return arr.get(i);
        }
    }
    EntityType en = PlEntity::newEntity(parent, ekind, PlToken::sNilTok, etag);
    en->setTokStr(tokstr);
    return en;
}

std::string PlEntity::getCppDirecStr(CppDirecType cdtype, size_t index)
{
    std::string s;
    if (mKind != EKind::CppDirec)
    {
        dbglog("Not a CppDirect entity\n");
    }
    else if (mToken.equals(sCppDirecTypeMap.toString(cdtype)))
    {
        if (index == 0)
        {
            EntityType valen = getChild(EKind::Literal);
            if (valen)
            {
                s = valen->mToken.strNoQuotes(true);
            }
        }
        else
        {
            PlArr<EntityType> arr = getChildren(EKind::Literal);
            EntityType valen = arr.get(index);
            if (valen)
            {
                s = valen->mToken.strNoQuotes(true);
            }
        }
    }
    return s;
}

EntityType PlEntity::getChild(ETag tg)
{
    // Gets only a single child... don't use if multiple children
    // are expected.
    for (auto& sub : mSubEntities)
    {
        if (sub.mTag == tg)
        {
            return &sub.mEntities.front();
        }
    }
    return nullptr;
}

EntityType PlEntity::getChild(EKind ek, ETag tg)
{
    // Gets only a single child... don't use if multiple children
    // are expected.
    for (auto& sub : mSubEntities)
    {
        if (sub.mTag == tg)
        {
            for (auto& e : sub.mEntities)
            {
                if (e.mKind == ek)
                {
                    return &e;
                }
            }
        }
    }
    return nullptr;
}

PlArr<EntityType> PlEntity::getChildren(ETag tg)
{
    for (auto& sub : mSubEntities)
    {
        if (sub.mTag == tg || tg == ETag::Any)
        {
            PlArr<EntityType> arr(sub.mEntities.size());
            size_t i = 0;
            for (auto& e : sub.mEntities)
            {
                arr.set(i++, &e);
            }
            return arr;
        }
    }
    return PlArr<EntityType>(0);
}

PlArr<EntityType> PlEntity::getChildren(EKind ek, ETag tg)
{
    for (auto& sub : mSubEntities)
    {
        if (sub.mTag == tg || tg == ETag::Any)
        {
            size_t cnt = 0;
            for (auto& e : sub.mEntities)
            {
                if (e.mKind == ek)
                {
                    cnt++;
                }
            }
            if (cnt == 0)
            {
                break;
            }
            PlArr<EntityType> arr(cnt);
            size_t i = 0;
            for (auto& e : sub.mEntities)
            {
                if (e.mKind == ek)
                {
                    arr.set(i++, &e);
                }
            }
            return arr;
        }
    }
    return PlArr<EntityType>(0);
}

std::string PlEntity::getStrings(EKind ekind, ETag tg, std::string sep, size_t max)
{
    // Value segments stored in tagged entries.  This function
    // will return a concated value with seperator sep. Ex: "math.sign"
    std::string ret;
    size_t cnt = 0;
    for (auto& sub : mSubEntities)
    {
        if (sub.mTag == tg)
        {
            for (auto& e : sub.mEntities)
            {
                if (e.mKind != ekind)
                {
                    continue;
                }
                if (cnt > 0)
                {
                    ret += sep;
                }

                ret += e.getStr();

                cnt++;
                if (cnt >= max)
                {
                    break;
                }
            }
        }
    }
    return ret;
}

void PlEntity::getChildrenStr(EKind ekind, ETag tg, std::vector<std::string>& strs)
{
    // Strip quotes
    std::string ret;
    size_t cnt = 0;
    for (auto& sub : mSubEntities)
    {
        if (sub.mTag == tg)
        {
            for (auto& e : sub.mEntities)
            {
                if (e.mKind != ekind)
                {
                    continue;
                }
                strs.push_back(e.mToken.strNoQuotes(true));
            }
        }
    }
}

void PlEntity::dbgDump(const char* label)
{
#ifdef _DEBUG
    base::StrBld bld;
    bld.append(label);
    dumpEn(this, ETag::Primary, bld);
    bld.append("\n");
    for (auto& sub : mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            bld.append("   ");
            dumpEn(&e, sub.mTag, bld);
            bld.append("\n");
        }
    }

    // PlTypeInfo ty;
    // ty.fromEn(this, sigTag());
    // bld.append("Type:");
    // ty.dump(bld);

    dbglog("%s", bld.c_str());
#endif
}

void PlEntity::dump(const std::string& sp, ETag tg, base::StrBld& bld)
{
    bld.append(sp);
    bld.append("_.");
    dumpEn(this, tg, bld);
    bld.appendc('\n');

    for (auto& sub : mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            e.dump(sp + " |", sub.mTag, bld);
        }
    }
}

void PlEntity::dumpEn(EntityType en, ETag tg, base::StrBld& bld)
{
    bld.append(sEKindMap.toString(en->mKind));
    if (tg != ETag::Primary)
    {
        bld.appendc('.');
        bld.append(sETagMap.toString(tg));
    }
    if (en->mDT != DataType::d_none)
    {
        bld.append(" #");
        bld.append(sDataTypeMap.toString(en->mDT));
    }
    bld.appendFmt(" %llx", en);
    if (!en->mToken.isEmpty())
    {
        bld.append(" '");
        bld.append(en->mToken.toString());
        bld.appendc('\'');
    }
    if (en->mResolvedRef)
    {
        bld.appendFmt(" => {%s '%s' %llx}", sEKindMap.toString(en->mResolvedRef->mKind),
            en->mResolvedRef->mToken.toString().c_str(),en-> mResolvedRef);
    }
    en->dumpAttrib(bld);
}

void PlEntity::dumpAttrib(base::StrBld& bld)
{
    bld.appendc(' ');
    for (size_t i = 0; i < sEntityAttribMap.count(); i++)
    {
        if (mAttribFlags.isBitNoSet(i))
        {
            bld.appendc('~');
            bld.append(sEntityAttribMap.toStringI(i));
        }
    }
}

void PlEntity::toVar(base::Variant& var, bool pubonly)
{
    // Pubonly only applies to root objects in this case
    if (pubonly && mKind != EKind::FileRoot)
    {
        pubonly = false;
    }

    // Count subentities
    size_t subcnt = 0;
    for (auto& sub : mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            if (pubonly && !e.isPublic())
            {
                continue;
            }
            subcnt++;
        }
    }

    // Handle the case where the root is empty, by providing an empty object.
    if (mKind == EKind::FileRoot && subcnt == 0)
    {
        return;
    }

    var.createObject();
    var.setProp(L_KIND, sEKindMap.toString(mKind));
    if (!mToken.isEmpty())
    {
        var.setProp(L_TOK, mToken.toString());
    }
    if (mAttribFlags.value() != 0)
    {
        var.setProp(L_ATTRIB, mAttribFlags.value());
    }

    if (subcnt > 0)
    {
        base::Variant subentarr;
        subentarr.createArray();
        var.setProp(L_SUB, subentarr);
        for (auto& sub : mSubEntities)
        {
            base::Variant subent;
            subent.createObject(L_ENTSINIT);
            subent.setProp(L_TAG, sETagMap.toString(sub.mTag));
            for (auto& e : sub.mEntities)
            {
                if (pubonly && !e.isPublic())
                {
                    continue;
                }
                base::Variant ent;
                e.toVar(ent, false);
                subent[L_ENTS].push(ent);
            }
            var[L_SUB].push(subent);
        }
    }
}

void PlEntity::write(base::PersistWr& pwr)
{
    // IMPORTANT: Must match read()
    pwr.wrUInt32((uint32)mKind);
    pwr.wrUInt32((uint32)mDT);
    pwr.wrUInt64(mAttribFlags.value());
    pwr.wrStr(mToken.toString().c_str());
    pwr.wrUInt32((uint32)mToken.mFlags);
    pwr.wrUInt32((uint32)mToken.mBegLine);
    pwr.wrUInt32((uint32)mToken.mBegColumn);
}

EntityType PlEntity::read(EntityType parent, base::PersistRd& prd, ETag etag)
{
    EntityType en = nullptr;

    // IMPORTANT: Must match write()
    PlToken tok;
    EAttribFlags attrib;
    EKind kind = (EKind)prd.rdUInt32();
    DataType dt = (DataType)prd.rdUInt32();
    attrib.setValue(prd.rdUInt64());
    tok.setStr(prd.rdStr().c_str());
    tok.mFlags = prd.rdUInt32();
    tok.mBegLine = prd.rdUInt32();
    tok.mBegColumn = prd.rdUInt32();

    if (!prd.inErr())
    {
        // Create the entity
        en = PlEntity::newEntity(parent, kind, tok, etag);
        en->mAttribFlags = attrib;
        en->mDT = dt;
    }

    return en;
}

void PlEntity::saveEntity(base::PersistWr& pwr)
{
    // IMPORTANT: Must match PlFileState::loadEntity
    // Persist the entity
    write(pwr);

    auto subentarr = pwr.wrArr();
    for (auto& sub : mSubEntities)
    {
        pwr.wrUInt32((uint)sub.mTag);

        auto entarr = pwr.wrArr();
        for (auto& e : sub.mEntities)
        {
            e.saveEntity(pwr);
            pwr.endElem(entarr);
        }
        pwr.endElem(subentarr);
    }
}

EntityType PlEntity::cloneEntity(EntityType fromen, EntityType parent, ETag tg)
{
    // Create a new entry to clone the entity
    EntityType en = PlEntity::newEntity(parent, fromen->mKind, fromen->mToken, tg);
    en->mAttribFlags = fromen->mAttribFlags;
    en->mDT = fromen->mDT;
    en->mUnit = fromen->mUnit;
    en->mResolvedRef = fromen->mResolvedRef;

    // Clone subentities
    for (auto& sub : fromen->mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            cloneEntity(&e, en, sub.mTag);
        }
    }

    return en;
}

void PlEntity::gatherPubs(EntityType parent, ETag tg, bool nonpub, PlErrs& errs)
{
    if (!nonpub && !isPublic())
    {
        return;
    }

    EntityType en = PlEntity::newEntity(parent, mKind, mToken, tg);
    en->mAttribFlags = mAttribFlags;
    en->mDT = mDT;

    for (auto& sub : mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            bool neednonpub = nonpub ||
                (mKind == EKind::FuncMapping) ||
                (mKind == EKind::TypeDef) ||
                (mKind == EKind::Interf) ||
                (mKind == EKind::CppDirec);

            e.gatherPubs(en, sub.mTag, neednonpub, errs);
        }
    }
}

EntityType PlEntity::createResol(EntityType resolref, const char* tokstr, ETag tg)
{
    auto arr = getChildren(EKind::Resol, tg);
    for (size_t i = 0; i < arr.count(); i++)
    {
        if (arr.get(i)->mResolvedRef == resolref)
        {
            return arr.get(i);
        }
    }

    PlToken tok;
    if (tokstr)
    {
        tok.setStr(tokstr);
    }
    EntityType en = PlEntity::newEntity(this, EKind::Resol, tok, tg);
    en->mResolvedRef = resolref;
    return en;
}

void PlEntity::updateSymTbl(ETag tg, PlSymbolTable* symtable, PlUnit* unit)
{
    PlSymbolAg symag(false, symtable, unit);

    std::string ss;
    if (hasAttrib(EAttribFlags::a_symbol))
    {
        ss = PlSymbolAg::getSymName(this);
    }
    symag.process(this, ss.c_str());
    for (auto& sub : mSubEntities)
    {
        for (auto& e : sub.mEntities)
        {
            e.updateSymTbl(sub.mTag, symtable, unit);
        }
    }
}

