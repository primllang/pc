#include "primalc.h"

// PlUnit::
bool PlUnit::init(const base::Path unitpath)
{
    //dbglog("PlUnit::Init %s\n", unitpath.c_str());

    mPath = unitpath;

    // Read the unit information and validate
    if (!plReadYaml("unit", metaFn(), mMeta))
    {
        return false;
    }
    //dbglog("%s\n", unitmeta.toJsonString().c_str());
    mName = mMeta[L_UNIT][L_UNITNAME];
    if (mName.empty())
    {
        dbgerr("Invalid unit file. Unit name missing\n");
        return false;
    }
    if (!sUnitTypeMap.fromString(mMeta[L_UNIT][L_UNITTYPE], &mType))
    {
        dbgerr("Invalid unit type\n");
        return false;
    }

    // Init compiler state
    mCompState.init(this);

    //dbglog("Init unit %s\n", unitpath.c_str());

    mInit = true;
    return true;
}

bool PlUnit::getSrcFiles(bool& srcisnew)
{
    std::vector<base::Path> files;
    bool glob = false;

    if (mMeta[L_UNITSRC].isArray())
    {
        for (auto const& i : mMeta[L_UNITSRC])
        {
            base::Path srcname = i.toString();

            if (!srcname.isExt(L_PRIMALEXT))
            {
                dbgerr("Source file %s must have extension '%s'\n", srcname.c_str(), L_PRIMALEXT);
                return false;
            }
            if (!base::existFile(srcFn(srcname)))
            {
                dbgerr("Source file '%s' not found\n", srcFn(srcname).c_str());
            }

            mCompState.addSrc(srcname);
        }
    }
    else
    {
        glob = base::strieql(mMeta[L_UNITSRC].toString(), "glob");
        if (!glob)
        {
            dbgerr("Source files must be provided or 'glob' specified\n");
            return false;
        }
    }

    if (glob)
    {
        if (!listDirectory(srcDir(), &files, nullptr))
        {
            return false;
        }
        for (auto& fn : files)
        {
            if (fn.isExt(L_PRIMALEXT))
            {
                mCompState.addSrc(fn);
            }
        }
    }

    for (base::Iter<PlFileState> fi; mCompState.mSrcFiles.forEach(fi); )
    {
        base::Path fn(fi->fname());
        if (newerFile(srcFn(fn), cppFn(fn)))
        {
            srcisnew = true;
            fi->setMod();
        }
    }

    return true;
}

base::Path PlUnit::target(BuildConfig bldcfg)
{
    std::string cfg = sBuildConfigMap.toString(bldcfg);
    base::Path targ;

    if (mType == UnitType::exe)
    {
        targ.set(outDir(), cfg);
        targ.add(mName);
        if (!sConfig.isLinux())
        {
            targ.modifyExt("exe");
        }
    }
    else
    {
        targ.set(buildDir(), "lib");
        targ.add(cfg);
        targ.add(mkLibFn(mName));
    }
    return targ;
}


bool PlUnit::build(BuildConfig bldcfg)
{
    if (!mInit)
    {
        return false;
    }

    mErrCol.clearErrors();

    // Create directories (if not found)
    base::createDirectory(targetDir());
    base::createDirectory(diagDir());
    base::createDirectory(buildDir());
    base::createDirectory(astDir());

    // Read in the list of source files, and determine which files have been modified.
    bool srcisnew = false;
    getSrcFiles(srcisnew);

    // If any new source files were found, make a pass to compile
    if (srcisnew)
    {
        dbglog("buildUnit '%s' (%s) \n", mPath.c_str(), mName.c_str());

        // Compile or load representation for source files
        mCompState.compileSrc();

        // Load dependant unit representation
        mCompState.loadUnits();

        // Resolve entities in source file state
        mCompState.resolveSrcEntitites();

        // Generate target files
        for (base::Iter<PlFileState> fi; mCompState.mSrcFiles.forEach(fi); )
        {
            if (fi->enRoot())
            {
                base::Path fn(fi->fname());
                if (!fi->isErr())
                {
                    fi->generateFile(OutputFmt::cpp, cppFn(fn));
                    fi->generateFile(OutputFmt::header, privHdrFn(fn));
                    fi->generateFile(OutputFmt::entity, srcAstFn(fn));
                }
                if (mBldDiagFiles)
                {
                    fi->generateFile(OutputFmt::diag, diagFn(fn));
                }
            }
        }

        // Generate the unit wide header file
        if (!writeUnitWideHdr())
        {
            return false;
        }
    }

    // Generate CMake file, if it meta file is newer
    bool cmakeupdated = false;
    if (!writeCmake(cmakeupdated))
    {
        return false;
    }

    // Generate Ninja files using CMake
    if (cmakeupdated)
    {
        if (!writeNinjaFiles(targetDir()))
        {
            return false;
        }
        //base::createDirectory(astDir());
    }

    if (srcisnew || cmakeupdated)
    {
       // Gather public entities for this unit
        mCompState.gatherAllPubs();

        mCompState.generateFile(OutputFmt::pubheader, pubHdrFn());
        mCompState.generateFile(OutputFmt::entity, unitAstFn());

        if (mBldDiagFiles)
        {
            mCompState.writeDiagInfo(unitDiagFn());
        }
    }

    if (mErrCol.hasErrs())
    {
        return false;
    }

    // Build ninja
    if (!buildNinja(targetDir(), bldcfg))
    {
        return false;
    }

    if (mBldTempFiles && (srcisnew || cmakeupdated))
    {
        moveTemps();
    }

    return true;
}

bool PlUnit::buildNinja(base::Path dir, BuildConfig bldcfg)
{
    bool ret = false;
    DirPush d(dir);

    // Build ninja
    // TODO: do we want to use ninja verbose flag only when global verbose flag is turned on
    std::string bldcmd = base::formatr(
        "cmake --build ./%s --config %s -- --verbose",
        L_BUILDDIR,
        sBuildConfigMap.toString(bldcfg));

    ret = plLogShell(bldcmd, mLog, mErrLog) == 0;

    return ret;
}

bool PlUnit::cleanNinja(base::Path dir)
{
    bool ret = false;
    DirPush d(dir);

    // Clean ninja
    std::string bldcmd = "cmake --build ./ --target clean";
    ret = plLogShell(bldcmd, mLog, mErrLog) == 0;

    return ret;
}

bool PlUnit::existNinjaFiles(base::Path dir)
{
    // Do ninja build files exists in subdir Config::ninjabuilddir under given dir
    base::Path checkfn(dir, L_BUILDDIR);
    checkfn.add("build.ninja");
    return base::existFile(checkfn);
}

bool PlUnit::writeNinjaFiles(base::Path dir)
{
    bool ret = false;
    base::Path prevdir;
    DirPush d(dir);

    // Generate Ninja files using CMake
    std::string cmakecmd = base::formatr("cmake -S ./ -B ./%s -G \"%s\"",
        L_BUILDDIR,
        sConfig.getStr(Config::ninjaconfig).c_str());
    ret = plLogShell(cmakecmd, mLog, mErrLog) == 0;

    return ret;
}

bool PlUnit::writeUnitWideHdr()
{
    base::StrBld cm;

    // Include Cpp Direc specified headers for each source file
    for (base::Iter<PlFileState> fi; mCompState.mSrcFiles.forEach(fi); )
    {
        PlArr<EntityType> ea = fi->getRootChildren(EKind::CppDirec);
        for (size_t i = 0; i < ea.count(); i++)
        {
            EntityType ce = ea.get(i);
            std::string inc = ce->getCppDirecStr(CppDirecType::include);
            if (!inc.empty())
            {
                cm.append("#include ");
                if (inc[0] != '<')
                {
                    cm.appendc('\"');
                }
                cm.append(inc);
                if (inc[0] != '<')
                {
                    cm.appendc('\"');
                }
                cm.append("\n");
            }
        }
    }

    // Public headers for deps units
    for (base::Iter<PlFileState> fi; mCompState.mDepUnits.forEach(fi); )
    {
        PlUnit* du = fi->containingUnit();
        cm.append("#include \"");
        cm.append(du->pubHdrFn().namePart());
        cm.append("\"\n");
    }

    // Headers corresponding to every source file
    for (base::Iter<PlFileState> fi; mCompState.mSrcFiles.forEach(fi); )
    {
        cm.append("#include \"");
        cm.append(privHdrFn(fi->fname()).namePart());
        cm.append("\"\n");
    }

    // Use namespaces for units imported into global namespace
    for (base::Iter<PlFileState> fi; mCompState.mDepUnits.forEach(fi); )
    {
        if (base::streql(fi->containingUnit()->mNS, L_GLOBALNAMESPACE))
        {
            cm.appendFmt("using namespace %s;", fi->containingUnit()->cppNamespace().c_str());
        }
    }

    // Write out the file... only if has changed from whats in the file
    base::Buffer buf;
    cm.moveToBuffer(buf);
    (void)plWrite("Unit wide header", buf, unitWideFn(), true);
    return true;
}

bool PlUnit::makeCmakeLibStr(PlUnit* depunit, base::StrBld& cm, const char* dir, const char* proj, const char* libname, base::Path eincdir)
{
    eincdir.makeRelTo(targetDir());
    cm.appendFmt("add_library(%s STATIC IMPORTED)\nset_target_properties(%s PROPERTIES\n",
        libname, libname);
    cm.append("    INTERFACE_INCLUDE_DIRECTORIES \"");
    if (!eincdir.isAbs())
    {
        cm.append("${CMAKE_CURRENT_SOURCE_DIR}/");
    }
    cm.append(eincdir.withFwdSlash());
    cm.append("\"\n");

    for (size_t bc = 0; bc < sBuildConfigMap.count(); bc++)
    {
        std::string cfg = sBuildConfigMap.toStringI(bc);
        std::string cfgu(cfg);
        base::upperCase(cfgu);

        base::Path lib = base::Path(depunit->mPath, dir);
        lib.add(L_BUILDDIR);
        lib.add("lib");
        lib.add(cfg);
        lib.add(mkLibFn(libname));
        lib.makeRelTo(targetDir());

        cm.appendFmt("    IMPORTED_LOCATION_%s \"%s%s\"\n",
            cfgu.c_str(),
            lib.isAbs() ? "" : "${CMAKE_CURRENT_SOURCE_DIR}/",
            lib.withFwdSlash().c_str());
    }
    cm.append(")\n");
    return true;
}

bool PlUnit::moveTemps()
{
    for (base::Iter<PlFileState> fi; mCompState.mSrcFiles.forEach(fi); )
    {
        base::Path src(buildDir(), fi->fname());
        src.modifyExt("ii");
        if (base::existFile(src))
        {
            src.modifyExt("*");
#ifdef _WIN32
            std::string cmd = base::formatr("move \"%s\" \"%s\"", src.c_str(), diagDir().c_str());
#else
            std::string cmd = base::formatr("mv \"%s\" \"%s\"", src.c_str(), diagDir().c_str());
#endif
            base::shellCmd(cmd, false);
        }
    }
    return false;
}

bool PlUnit::getCmakeLibs(PlUnit* depunit, std::string& addlib, std::string& libnames)
{
    base::StrBld cm;

    // Add the unit itself
    libnames += "\n    ";
    libnames += depunit->mName;
    makeCmakeLibStr(depunit, cm, L_TARGETDIR, depunit->mName.c_str(), depunit->mName.c_str(), depunit->targetDir());

    // Go through ext projects
    if (depunit->mMeta[L_UNITEXT].isArray())
    {
        for (auto const& i : depunit->mMeta[L_UNITEXT])
        {
            if (i.isObject())
            {
                const char* projkey = i.getKey(0);
                base::Variant proj = i[projkey];

                libnames += "\n    ";
                libnames += proj[L_UNITEXTLIB].toString();

                base::Path eincdir = base::Path(depunit->mPath,
                    PlConfig::fixPath(proj[L_UNITEXTINCPATH].toString()));

                makeCmakeLibStr(depunit, cm, projkey, projkey, proj[L_UNITEXTLIB].toString().c_str(), eincdir);
            }
        }
    }
    cm.copyToStr(addlib);
    return true;
}


bool PlUnit::writeCmake(bool& cmakeupdated)
{
    // TODO: Implement list of sources file in the unit meta file (this
    // scheme only works if meta has list of source files)
    // TODO: Need to also account for unit.yaml changing etc
    cmakeupdated = false;
    if (!newerFile(metaFn(), cmakeFn()))
    {
        return true;
    }

    // Read the cmake template file into a buffer (not null terminated)
    base::Buffer cmakebuf;
    if (!plRead("CMake template", cmakebuf, base::Path(sConfig.getPath(Config::templatedir),
        sConfig.getStr(Config::cmaketemplate)), false))
    {
        return false;
    }

    base::StrBld cm;
    cmakeupdated = true;
    cm.append((const char*)cmakebuf.cptr(), cmakebuf.size());

    if (mType == UnitType::exe)
    {
        cm.append("add_executable(");
    }
    else
    {
        cm.append("add_library(");
    }
    // Target name
    cm.append(mName);

    // Source files
    for (base::Iter<PlFileState> fi; mCompState.mSrcFiles.forEach(fi); )
    {
        cm.append("\n    ");
        cm.append(cppNameFn(fi->fname()));
    }
    cm.append("\n)\n");

    // Header files for ext projects used by this unit
    if (!mCompState.mExtIncDirs.empty())
    {
        cm.append("include_directories(\n");

        // Ext projects inc dirs for this unit
        for (base::Iter<base::Path> p; mCompState.mExtIncDirs.forEach(p); )
        {
            base::Path einc(*p);
            einc.makeRelTo(targetDir());

            cm.appendFmt("    %s\n", einc.withFwdSlash().c_str());
        }
        cm.append(")\n");
    }

    // Libs and header files (including unit lib and ext project(s) used by
    // the unit) for each dependant unit
    if (!mCompState.mDepUnits.empty())
    {
        std::string tglnk;
        for (base::Iter<PlFileState> fi; mCompState.mDepUnits.forEach(fi); )
        {
            PlUnit* du = fi->containingUnit();
            std::string libstr;
            std::string namestr;
            getCmakeLibs(du, libstr, namestr);
            tglnk += namestr;

            cm.append(libstr);
        }
        cm.appendFmt("target_link_libraries(%s%s\n)\n\n", mName.c_str(), tglnk.c_str());
    }

    // Ask CPP compiler to save temp files
    if (mBldTempFiles)
    {
        cm.appendFmt("set_target_properties(%s PROPERTIES COMPILE_FLAGS \"-save-temps\")", mName.c_str());
    }

    // Write out the file
    base::Buffer cmlistbuf;
    cm.moveToBuffer(cmlistbuf);
    bool ret = plWrite("CMake file", cmlistbuf, cmakeFn(), true);
    if (!ret)
    {
        cmakeupdated = false;
    }
    if (cmakeupdated)
    {
        if (isFlagSet(base::sBaseGFlags, base::GFLAG_VERBOSE_FILESYS))
        {
            dbglog("CMake file updated\n");
        }
    }
    return true;
}

bool PlUnit::findUnitPath(std::string name, base::Path& path)
{
    // TODO: implement full config search path

    base::Path p;
    // Look in the same directory where the parent is stored.
    p.set(mPath.dirPart(), name);
    if (base::existDirectory(p))
    {
        path = p;
        return true;
    }

    // Look in the directory where the compiler is deployed and config is stored
    p.set(sConfig.cfgFn().dirPart().dirPart(), name);
    if (base::existDirectory(p))
    {
        p.makeAbs();
        path = p;
        return true;
    }

    return false;
}

bool PlUnit::buildExt(BuildConfig bldcfg)
{
    // Build external projects specified under 'ext'

    if (mMeta[L_UNITEXT].isArray())
    {
        for (auto const& i : mMeta[L_UNITEXT])
        {
            if (!i.isObject())
            {
                dbgerr("Invalid project specification in 'ext:'\n");
                return false;
            }
            const char* projkey = i.getKey(0);
            base::Variant proj = i[projkey];

            // Path is relative to the unit directory
            base::Path erelpath = PlConfig::fixPath(proj[L_UNITEXTPATH].toString());
            base::Path edir = base::Path(mPath, erelpath);

            //dbglog("Ext project '%s'\n", projkey);
            if (!existNinjaFiles(edir))
            {
                writeNinjaFiles(edir);
            }

            buildNinja(edir, bldcfg);

            // Add inc file to ext projs inc list
            base::Path eincdir = base::Path(mPath,
                PlConfig::fixPath(proj[L_UNITEXTINCPATH].toString()));
            mCompState.mExtIncDirs.addBack(eincdir);
        }
    }

    return true;
}

bool PlUnit::clean(bool hard)
{
    if (!mInit)
    {
        return false;
    }
    dbglog("cleanUnit%s (%s)\n", hard ? "-hard" :"", mName.c_str());
    if (hard)
    {
        return base::removeDirectory(targetDir());
    }
    else
    {
        bool ret = cleanNinja(buildDir());
        base::removeDirectory(diagDir());
        base::removeDirectory(astDir());
        base::removeDirectory(outDir());

        std::vector<base::Path> files;
        if (listDirectory(targetDir(), &files, nullptr))
        {
            for (auto& fn : files)
            {
                if (fn.isExt(L_HDREXT) || fn.isExt(L_CPPEXT))
                {
                    base::Path fi(targetDir(), fn);
                    base::deleteFile(fi);
                }
            }
        }

        return ret;
    }
}

bool PlUnit::cleanExt(bool hard)
{
    if (!mInit)
    {
        return false;
    }
    if (mMeta[L_UNITEXT].isArray())
    {
        for (auto const& i : mMeta[L_UNITEXT])
        {
            const char* projkey = i.getKey(0);
            base::Variant proj = i[projkey];

            base::Path edir = base::Path(mPath, proj[L_UNITEXTPATH].toString());
            edir.add(L_BUILDDIR);

            dbglog("cleanExt%s '%s' \n", hard ? "(hard)" :"", projkey);
            if (hard)
            {
                return base::removeDirectory(edir);
            }
            else
            {
                cleanNinja(edir);
            }
        }
    }
    return true;
}

bool PlUnit::writeLogs()
{
    plWrite("Log file", mLog, bldLogFn());
    plWrite("Error log file", mErrLog, errLogFn());

    mLog.free();
    mErrLog.free();

    return true;
}


// PlBuilder::

bool PlBuilder::newUnit(UnitType ut, base::Path unitpath, bool nogit)
{
    std::string templatename = sUnitTypeMap.toString(ut);

    // Checks and path setup
    if (unitpath.empty())
    {
        dbgerr("Must specify unit path to create\n");
        return false;
    }

    auto unitname = unitpath.namePart();
    if (!plIsIdent(unitname))
    {
        dbgerr("'%s' is an invalid unit name\n", unitname.c_str());
        return false;
    }

    if (base::existDirectory(unitpath))
    {
        dbgerr("'%s' already exists\n", unitpath.c_str());
        return false;
    }
    if (!unitpath.dirPart().empty())
    {
        if (!base::existDirectory(unitpath.dirPart()))
        {
            dbgerr("'%s' must exist to create the unit\n", unitpath.dirPart().c_str());
            return false;
        }
    }

    // Copy template directory files creating required directories
    base::Path unittempdir(sConfig.getPath(Config::templatedir), templatename);
    if (!base::copyDirectory(unittempdir, unitpath))
    {
        dbgerr("Failed to copy template files to %s\n", unitpath.c_str());
    }

    // Generate the unit YAML
    base::Variant yaml;
    base::Variant unitobj;

    // Unit
    unitobj.createObject();
    unitobj.setProp(L_UNITNAME, unitname);
    if (ut == UnitType::exe)
    {
        unitobj.setProp(L_UNITTYPE, "exe");
    }
    else
    {
        unitobj.setProp(L_UNITTYPE, "lib");
    }
    unitobj.setProp(L_UNITVER, "0.1.1");
    unitobj.setProp("edition", "2023");

    // Src
    base::Variant srcarr;
    srcarr.createArray();
    base::Path m("main");
    m.modifyExt(L_PRIMALEXT);
    srcarr.push(m.c_str());

    // Deps
    base::Variant depsarr;
    depsarr.createArray();
    if (!base::strieql(unitname.c_str(), L_BASEUNIT))
    {
        base::Variant entryobj;
        base::Variant infoobj;
        entryobj.createObject();
        infoobj.createObject();
        infoobj.setProp(L_NAMESPACE, L_GLOBAL);
        entryobj.setProp(L_BASEUNIT, infoobj);
        depsarr.push(entryobj);
    }

    // Ext
    base::Variant extarr;
    extarr.createArray();

    // Put together the object
    yaml.createObject();
    yaml.setProp(L_UNIT, unitobj);
    yaml.setProp(L_UNITSRC, srcarr);
    yaml.setProp(L_UNITDEPS, depsarr);
    yaml.setProp(L_UNITEXT, extarr);

    // Write the YAML file
    base::Buffer buf;
    yaml.getYaml(buf);
    plWrite("Unit yaml", buf, base::Path(unitpath, sConfig.getStr(Config::unityamlfn)));

    // Init git repository
    if (!nogit)
    {
        std::string gitinit("git init ");
        gitinit += unitpath;
        if (base::shellCmd(gitinit, false) != 0)
        {
            dbgerr("Failed to init git repo %s\n", unitpath.c_str());
        }
    }

    printf("Unit '%s' created\n", unitpath.c_str());

    return true;
}

bool PlBuilder::cleanUnit(base::Path unitpath, bool hard)
{
    if (!unitpath.empty())
    {
        PlUnit u;
        if (u.init(unitpath))
        {
            u.cleanExt(hard);
            u.clean(hard);
            return true;
        }
    }
    return false;
}

PlUnit* PlBuilder::getUnitDeps(base::List<PlUnit>& deps, const base::Path& parentpath)
{
    PlUnit* parent = new (deps.allocUnlinked()) PlUnit();
    if (!parent->init(parentpath))
    {
        deps.deleteUnlinked(parent);
        return nullptr;
    }

    if (parent->mMeta[L_UNITDEPS].isArray())
    {
        for (auto const& i : parent->mMeta[L_UNITDEPS])
        {
            std::string depname;
            base::Variant depobj;
            std::string depns;

            // Accept both an object and a string
            if (i.isObject())
            {
                depname.assign(i.getKey(0));
                depobj = i[depname];

                depns = depobj[L_DEPNAMESPACE];
                if (depns.empty())
                {
                    depns = depname;
                }
            }
            else
            {
                depname = i.toString();
                depns = depname;
            }

            //dbglog("Dep [%s] \n", depname.c_str());

            // Does the dep already exist in the discovered list?
            PlUnit* foundunit = nullptr;
            //for (auto& e : deps)
            for (base::Iter<PlUnit> e; deps.forEach(e); )
            {
                if (base::strieql(e->mName, depname))
                {
                    foundunit = &e;
                    break;
                }
            }
            if (!foundunit)
            {
                // Init unit
                base::Path unitpath;
                if (parent->findUnitPath(depname, unitpath))
                {
                    foundunit = getUnitDeps(deps, unitpath);
                    foundunit->mNS = depns;
                }
                else
                {
                    dbgerr("Error: Dep '%s' not found\n", depname.c_str());
                    return nullptr;
                }
            }

            // Add the unit to this units dep path list
            parent->mCompState.addDep(foundunit);
        }
    }

    // Finally, add the parent unit to discovered list
    deps.linkBack(parent);
    return parent;
}

bool PlBuilder::buildUnit(base::Path unitpath, BldOptions opts)
{
    base::List<PlUnit> depunits;

    PlUnit* primary = getUnitDeps(depunits, unitpath);
    bool blderr = false;

    // Build all the units that need to be built
    for (base::Iter<PlUnit> u; depunits.forEach(u); )
    {
        bool runbld = false;

        // Clean if needed
        if (opts.cleanfirst)
        {
            u->cleanExt(opts.cleanhard);
            u->clean(opts.cleanhard);
            runbld = true;
        }
        else
        {
            runbld = (&u == primary || opts.checkdeps);
        }

        // Build
        if (runbld)
        {
            u->mBldDiagFiles = opts.diagfiles;
            u->mBldTempFiles = opts.savetemps;
            u->buildExt(opts.bldcfg);

            bool ret = u->build(opts.bldcfg);

            if (!ret || u->mErrCol.hasErrs())
            {
                //dbgerr("Build failed\n");
                u->mErrCol.print();
                dbgerr("Failed to build unit (%s)\n", u->mName.c_str());
                blderr = true;
            }
            u->writeLogs();
        }

        if (&u == primary && !blderr)
        {
            dbglog("Target: %s\n", u->target(opts.bldcfg).c_str());
        }
    }

    return !blderr;
}

bool PlBuilder::runUnit(base::Path unitpath, BldOptions opts)
{
    if (!unitpath.empty())
    {
        PlUnit u;
        if (u.init(unitpath))
        {
            dbglog("Running %s\n", u.target(opts.bldcfg).c_str());
            return base::shellCmd(u.target(opts.bldcfg), true);
        }
    }
    return false;
}