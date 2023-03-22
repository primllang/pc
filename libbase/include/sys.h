#pragma once

#include "util.h"
#include <vector>

namespace base
{

// Path class mantains a filesystem path
class Path : public std::string
{
public:
    Path()
    {
    }
    Path(std::string s) :
        std::string(s)
    {
    }
    Path(const char* p) :
        std::string(p)
    {
    }
    Path(const base::Path& p1, const base::Path& p2)
    {
        set(p1, p2);
    }

    // Sets the value to a join of paths, handling the delimiters properly.
    void set(base::Path p)
    {
        assign(p);
    }
    void set(base::Path p1, base::Path p2);
    void add(base::Path p);
    void makeRelTo(base::Path topath);
    void makeAbs();
    void modifyExt(const std::string ext);
    void ensureNativeSlash();

    bool isExt(const std::string ext)
    {
        return strieql(extPart().c_str(), ext.c_str());
    }
    Path extPart() const;
    Path namePart() const;
    Path dirPart() const;
    void split(std::vector<base::Path>& parts);
    bool isAbs() const;
    Path withFwdSlash();
};

// Filesystem functionality
bool existFile(const base::Path& filename);
bool copyFile(const base::Path& src, const base::Path& dest);
bool deleteFile(const base::Path& filename);
bool newerFile(const base::Path& file, const base::Path& newerthanfile);
bool modTimeFile(const base::Path& filename, time_t& tm);
bool getTempFilename(base::Path& tempfn);
bool getFileSize(const base::Path& filename, size_t& size);

bool existDirectory(const base::Path& dirname);
bool createDirectory(const base::Path& dir);
bool removeDirectory(const base::Path& dir);
bool copyDirectory(const base::Path& srcdir, const base::Path& destdir);
bool listDirectory(const base::Path& dir, std::vector<base::Path>* filenames, std::vector<base::Path>* dirnames);
bool currentDirectory(base::Path& dir);
bool changeDirectory(const base::Path& dir);

// Process functionality
int shellCmd(const std::string& cmd, bool output, const base::Path* outfile = nullptr);
uint getPid();
base::Path getProcPath(uint pid);

// Unicode and UTF8
uint32_t makeUnicode(const char* s, int maxlen, int* lenused = NULL);
std::string makeUTF8(uint32_t charcode);

} // base
