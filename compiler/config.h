#include "var.h"


// Config entries enum
#define ConfigEnumList(_en_)  \
    _en_(templatedir)         \
    _en_(ninjaconfig)         \
    _en_(unityamlfn)          \
    _en_(cmaketemplate)       
enummapdef(ConfigEnumList, Config, sConfigMap, 0);


class PlConfig
{
public:
    bool init(base::Path configfn);
    std::string getStr(Config keyenum);
    base::Path getPath(Config keyenum);
    base::Path cfgFn()
    {
        return mCfgFn;
    }
    bool isLinux()
    {
        return base::streql(mPlat, LINUXPLAT);
    }
    bool isWindows()
    {
        return base::streql(mPlat, WIN32PLAT);
    }
    
    static base::Path fixPath(std::string path);

public:
    struct Default
    {
        Config cfg;
        const char* defvalue;
    };
    
private:
    constMemb CONFIGFN = "plconfig.yaml";
    constMemb WIN32PLAT = "win32";
    constMemb LINUXPLAT = "linux";
    constMemb COMMON = "common";

    base::Path mCfgFn;
    base::Variant mCfg;
    std::string mPlat;
};


// Config default values
constexpr PlConfig::Default sCfgDefaultMap[] = 
{
    {Config::ninjaconfig, "Ninja Multi-Config"}
};

extern PlConfig sConfig;
extern bool sVerbose;
