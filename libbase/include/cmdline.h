#pragma once

#include <string>
#include <string.h>
#include <vector>
#include <map>
#include <initializer_list>

namespace base
{

// Header-only class to parse command line and provide options and values.  Options are switches
// that can be specified with a -O short char name format, or as --OptionName long name format. 
// Options can have value (ie --config specifying a file name) when a value is specified after the option.
// Short name matches are case sensitive, long name matches are case sensitive when for the calling 
// code (getOption() and hasOption()) but are case insensitive for command line usage.  CmdLine object 
// can be passed along as config to other parts of program. Basic errors are cought and reported 
// via false return from init() and details in errMsg()
// NOTE: A special case exists for an option, whoese char value is ' ' (space), it is not allowed to have a value.
//
//    CmdLine cmd;
//    cmd.init(argc, argv, {
//        {'O', "option"},
//        {'a', "add"},
//        {'d', "delete"},
//        {'r', "reboot"},
//        {'c', "config"},
//        {'?', "help"}
//    });
//
// C:> prog.exe subcmd --config filename0 --reboot -a -d filename1 filename2
//
//      errmsg = []
//      opt: [Config] = [filename0]
//      opt: [add] = []
//      opt: [delete] = [filename1]
//      opt: [reboot] = []
//      val: [subcmd]
//      val: [filename2]

class CmdLine
{ 
public:
    CmdLine() = default;
    ~CmdLine() = default;
    bool init(int argc, char** argv, std::initializer_list<std::pair<char, const char*>> optionslist);
    size_t valueCount() const
    {
        return mValues.size();
    }
    size_t optionCount() const
    {
        return mOptions.size();
    }
    std::string getValue(size_t index)
    {
        return (index < mValues.size()) ? mValues[index] : std::string();
    }
    bool isValue(size_t index, const char* val)
    {
        return strcasecmp((index < mValues.size()) ? mValues[index].c_str() : "", val) == 0;
    }
    std::string getOption(const char* name)
    {
        auto it = mOptions.find(name);
        return (it != mOptions.end()) ? it->second : "";
    }
    bool hasOption(const char* name)
    {
        auto it = mOptions.find(name);
        return (it != mOptions.end());
    }
    std::string errMsg()
    {
        return mErrMsg;
    }
    bool hasErr()
    {
        return !mErrMsg.empty();
    }

    void dbgDump()
    {
        for (const auto& [key, value] : mOptions) 
        {
            dbglog("Option ['%s']='%s'\n", key.c_str(), value.c_str());
        }
        for (size_t i = 0; i < mValues.size(); i++)
        {
            dbglog("Value %zu='%s'\n", i, mValues[i].c_str());
        }
    }

private:
    std::map<std::string, std::string> mOptions;
    std::vector<std::string> mValues;
    std::string mErrMsg;
};

inline bool CmdLine::init(int argc, char** argv, std::initializer_list<std::pair<char, const char*>> optionslist)
{
    const char* prevopt = nullptr;
    bool ret = true;
    for (int i = 1; i < argc; i++)
    {
        const char* param = argv[i];

        // Count number of dash
        int dashes = 0;
        while (param[dashes] == '-')
        {
            dashes++;
        }

        // If there are no dashses, add as option or a value based on prev param
        if (dashes == 0)
        {
            if (prevopt)
            {
                mOptions[prevopt] = std::string(param);
                prevopt = nullptr;
            }
            else
            {
                mValues.push_back(std::string(param));
            }
        }
        else
        {
            // Here, we have an option (starting with 1 or 2 dashes).
            bool found = false;
            for (auto opt : optionslist)
            {
                // If we have 1 dash, match the char values (case sensitve) and length must be 2 ex: "-C"
                // if we have 2 dashes, match the option string name (case insensitve) ex: --config
                // Special case: if the char value for option is a ' ', don't allow it to have an associated 
                // option value
                if ((dashes == 1 && opt.first == param[1] && strlen(param) == 2) || 
                    (dashes == 2 && strcasecmp(opt.second, param + 2) == 0))
                {
                    mOptions[opt.second] = std::string();
                   
                    if (opt.first != ' ')
                    {
                        prevopt = opt.second;
                    }
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                if (mErrMsg.empty())
                {
                    mErrMsg = "Invalid option(s): ";
                }
                mErrMsg += param;
                ret = false;
            }
        }
    }
    return ret;
}

} // base
