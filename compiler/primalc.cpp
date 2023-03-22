#include "primalc.h"

int main(int argc, char **argv)
{
    base::CmdLine cmd;
    cmd.init(argc, argv, {
            {'C', "config"},
            {' ', "lib"},
            {' ', "exe"},
            {' ', "verbose"},
            {' ', "cleanfirst"},
            {' ', "checkdeps"},
            {' ', "diagfiles"},
            {' ', "hard"},
            {' ', "debug"},
            {' ', "release"},
            {' ', "relwithdeb"},
            {' ', "savetemps"},
            {' ', "nogit"},
            {'?', "help"}
        });

    PlBuilder builder;

    // Check/show usage
    if (cmd.hasOption("help") || cmd.valueCount() == 0 || cmd.hasErr())
    {
        if (cmd.hasErr())
        {
            dbgerr("%s\n", cmd.errMsg().c_str());
            cmd.dbgDump();
        }

        printf("Usage:\n"
            "    new <unit> [--exe or --lib] [--nogit]\n"
            "    build [<unit>] [<build type>]\n"
            "    run [<unit>] [<build type>]\n"
            "    clean <units> ...\n");
        printf("\nBuild types:\n"
            "    --debug\n"
            "    --release\n"
            "    --relwithdeb\n");
        printf("\nOther options:\n"
            "    --verbose\n"
            "    --cleanfirst\n"
            "    --hard\n"
            "    --checkdeps\n"
            "    --diagfiles\n"
            "    --savetemps\n"
            "    --config <configfile>\n"
            "    -?, --help\n");
        return 1;
    }

    if (cmd.hasOption("verbose"))
    {
        setFlag(base::sBaseGFlags, base::GFLAG_VERBOSE_SHELL | base::GFLAG_VERBOSE_FILESYS);
        sVerbose = true;
    }

    // Load and validate config
    sConfig.init(cmd.getOption("config"));
    if (!base::existDirectory(sConfig.getPath(Config::templatedir)))
    {
        dbgerr("Template directory '%s' not found\n", sConfig.getPath(Config::templatedir).c_str());
        return 1;
    }

    // Process commands
    if (cmd.isValue(0, "new"))
    {
        if ((cmd.hasOption("exe") || cmd.hasOption("lib")) && !cmd.getValue(1).empty())
        {
            builder.newUnit(cmd.hasOption("exe") ? UnitType::exe : UnitType::lib, cmd.getValue(1), cmd.hasOption("nogit"));
        }
        else
        {
            dbgerr("'new' command requires a unit name and an option (--exe or --lib)\n%s\n", cmd.errMsg().c_str());
            return 1;
        }
    }
    else if (cmd.isValue(0, "build"))
    {
        base::Path unitpath = cmd.getValue(1);
        PlBuilder::BldOptions opts;
        opts.diagfiles = true; // TODO: cmd.hasOption("diagfiles");
        opts.checkdeps = cmd.hasOption("checkdeps");
        opts.cleanfirst = cmd.hasOption("cleanfirst");
        opts.cleanhard = cmd.hasOption("hard");
        opts.savetemps = cmd.hasOption("savetemps");

        if (unitpath.empty())
        {
            base::currentDirectory(unitpath);
        }
        else
        {
            unitpath.makeAbs();
        }
        if (cmd.hasOption("release"))
        {
            opts.bldcfg = BuildConfig::Release;
        }
        else if (cmd.hasOption("relwithdeb"))
        {
            opts.bldcfg = BuildConfig::RelWithDebInfo;
        }
        else if (cmd.hasOption("debug"))
        {
            opts.bldcfg = BuildConfig::Debug;
        }

        if (!builder.buildUnit(unitpath, opts))
        {
            return 1;
        }
    }
    else if (cmd.isValue(0, "run"))
    {
        base::Path unitpath = cmd.getValue(1);
        PlBuilder::BldOptions opts;
        if (unitpath.empty())
        {
            base::currentDirectory(unitpath);
        }
        else
        {
            unitpath.makeAbs();
        }
        if (cmd.hasOption("release"))
        {
            opts.bldcfg = BuildConfig::Release;
        }
        else if (cmd.hasOption("relwithdeb"))
        {
            opts.bldcfg = BuildConfig::RelWithDebInfo;
        }
        else if (cmd.hasOption("debug"))
        {
            opts.bldcfg = BuildConfig::Debug;
        }

        if (!builder.runUnit(unitpath, opts))
        {
            return 1;
        }
    }
    else if (cmd.isValue(0, "clean"))
    {
        if (cmd.valueCount() > 1)
        {
            for (int i = 1; i < cmd.valueCount(); i++)
            {
                base::Path unitpath = cmd.getValue(i);
                bool hard = cmd.hasOption("hard");
                builder.cleanUnit(cmd.getValue(i), hard);
            }
        }
        else
        {
            dbgerr("'clean' command needs names for one or more units\n");
            return 1;
        }
    }
    else
    {
        dbgerr("Invalid command '%s'. Use --help\n%s\n", cmd.getValue(0).c_str(), cmd.errMsg().c_str());
        return 1;
    }

    return 0;
}
