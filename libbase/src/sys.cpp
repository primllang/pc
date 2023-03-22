#include "util.h"
#include "sys.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <direct.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#ifdef _WIN32
    constexpr char PATH_SEP = '\\';
#else
    constexpr char PATH_SEP = '/';
#endif

#ifdef _WIN32
#undef  PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#ifndef PATH_MAX
#define PATH_MAX    1024
#endif

constexpr long long EPOCH_DIFFERENCE = 11644473600LL;

namespace base
{

// Path::

void Path::set(base::Path p1, base::Path p2)
{
    assign(p1);
    if (!empty() && back() != PATH_SEP)
    {
        push_back(PATH_SEP);
    }
    append(p2);
}

void Path::add(base::Path p)
{
    if (!empty() && back() != PATH_SEP)
    {
        push_back(PATH_SEP);
    }
    append(p);
}

Path Path::dirPart() const
{
    size_t pos = find_last_of(PATH_SEP);
    if (pos == npos)
    {
        return Path();
    }
    return Path(substr(0, pos));
}

Path Path::namePart() const
{
    auto pos = find_last_of(PATH_SEP);
    if (pos == npos)
    {
        return *this;
    }
    return Path(substr(pos + 1));
}

Path Path::extPart() const
{
    auto lastseppos = find_last_of(PATH_SEP);
    auto pos = find_last_of('.');
    if (pos == npos || (lastseppos != npos && pos < lastseppos))
    {
        return Path();
    }
    return Path(substr(pos + 1));
}

void Path::modifyExt(const std::string ext)
{
    auto lastseppos = find_last_of(PATH_SEP);
    auto pos = find_last_of('.');
    if (pos == npos || (lastseppos != npos && pos < lastseppos))
    {
        if (ext.empty())
        {
            return;
        }
        append(".");
    }
    else
    {
        if (ext.empty())
        {
            erase(pos);
            return;
        }
        erase(pos + 1);
    }
    append(ext);
}

Path Path::withFwdSlash()
{
    Path p(*this);
    for (size_t i = 0; i < p.size(); i++)
    {
        if (p[i] == '\\')
        {
            p[i] = '/';
        }
    }
    return p;
}

void Path::ensureNativeSlash()
{
    const char nonnative = (PATH_SEP == '/' ? '\\' : '/');
    for (size_t i = 0; i < size(); i++)
    {
        if (std::string::operator[](i) == nonnative)
        {
            std::string::operator[](i) = PATH_SEP;
        }
    }
}

bool Path::isAbs() const
{
    size_t l = length();
    const char* p = c_str();
#ifdef _WIN32
    return (l > 2 && ((p[0] == PATH_SEP && p[1] == PATH_SEP) || (p[1] == ':' && p[2] == PATH_SEP)) );
#else
    return (l > 0 && p[0] == PATH_SEP);
#endif
}

void Path::split(std::vector<Path>& parts)
{
    Splitter sp(c_str(), PATH_SEP);
    while (!sp.eof())
    {
        parts.push_back(Path(sp.get()));
    }
}

void Path::makeAbs()
{
#ifdef _WIN32
    char buffer[PATH_MAX];
	if ((_fullpath(buffer, c_str(), PATH_MAX) != NULL))
    {
        assign(buffer);
    }
#else
    char* bufptr = realpath(c_str(), NULL);
    if(bufptr != NULL)
    {
        assign(bufptr);
        free(bufptr);
    }
#endif
}

void Path::makeRelTo(Path topath)
{
    std::vector<base::Path> vto;
    std::vector<base::Path> vfrom;
    topath.split(vto);
    split(vfrom);

    size_t shortest = (vto.size() < vfrom.size()) ? vto.size() : vfrom.size();
    size_t match = 0;
    for (size_t i = 0; i < shortest; i++)
    {
        if (vto[i] != vfrom[i])
        {
            break;
        }
        match++;
    }
    if (match > 0)
    {
        Path relpath;
        for (size_t i = 0; i < (vto.size() - match); i++)
        {
            relpath.add("..");
        }
        for (size_t i = match; i < vfrom.size(); i++)
        {
            relpath.add(vfrom[i]);
        }
        assign(relpath);
    }
}


// Unicode and UTF8
std::string makeUTF8(uint32_t charcode)
{
    std::string s;

    s.reserve(8);
    if (charcode < 0x80)
    {
        s = (char)charcode;
    }
    else
    {
        int firstbits = 6;
        const int otherbits = 6;
        int firstval = 0xC0;
        int t = 0;
        while ((int)charcode >= (1 << firstbits))
        {
            t = 0x80 | (charcode & ((1 << otherbits)-1));
            charcode >>= otherbits;
            firstval |= 1 << (firstbits);
            firstbits--;
            s.insert(0, 1, (char)t);
        }
        t = firstval | charcode;
        s.insert(0, 1, (char)t);
    }
    return s;
}

uint32_t makeUnicode(const char* s, int maxlen, int* lenused /*= NULL*/)
{
    assert(s);

    int charcode = 0;
    int ofs = 0;
    int t = (unsigned char)s[ofs];
    ofs++;

    if (t < 0x80)
    {
        charcode = t;
    }
    else
    {
        int highbitmask = (1 << 6) -1;
        int highbitshift = 0;
        int totalbits = 0;
        const int otherbits = 6;
        while ( ((t & 0xC0) == 0xC0) && (ofs < maxlen) )
        {
            t <<= 1;
            t &= 0xff;
            totalbits += 6;
            highbitmask >>= 1;
            highbitshift++;
            charcode <<= otherbits;

            charcode |= (int)s[ofs] & ((1 << otherbits)-1);
            ofs++;
        }
        charcode |= ((t >> highbitshift) & highbitmask) << totalbits;
    }

    if (lenused)
    {
        *lenused = ofs;
    }

    return charcode;
}

// File functions
bool existFile(const base::Path& filename)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(filename.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
	struct stat buf;
	if (stat(filename.c_str(), &buf) != 0)
    {
        return false;
    }
    return (buf.st_mode & S_IFDIR) == 0;
#endif
}

bool deleteFile(const base::Path& filename)
{
#ifdef _WIN32
    if (!DeleteFile(filename.c_str()))
    {
        return false;
    }
#else
    if (::unlink(filename.c_str()) != 0)
    {
        return false;
    }
#endif
    return true;
}



bool modTimeFile(const base::Path& filename, time_t& tm)
{
#ifdef _WIN32
    WIN32_FIND_DATA wfd;
    HANDLE hf = FindFirstFileExA(filename.c_str(), FindExInfoBasic, &wfd, FindExSearchNameMatch, NULL, 0);
    if (hf == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    tm = ((ULARGE_INTEGER&)wfd.ftLastWriteTime).QuadPart / 10000LL - EPOCH_DIFFERENCE * 1000LL;
    FindClose(hf);
    return true;
#else
    struct stat buf;
    if(stat(filename.c_str(), &buf) != 0)
    {
        return false;
    }
    tm = ((long long)buf.st_mtim.tv_sec) * 1000LL + ((long long)buf.st_mtim.tv_nsec) / 1000000LL;
    return true;
#endif
}

bool newerFile(const base::Path& file, const base::Path& newerthanfile)
{
    // Returns true if both files found, and modified time of 'file' is later than
    // the modified time of the 'newerthanfile'.
    // If one or both files are not found, returns true.
    // False otherwise.

    time_t filet;
    time_t newerthanfilet;

    if (modTimeFile(file, filet) && modTimeFile(newerthanfile, newerthanfilet))
    {
        return (filet > newerthanfilet);
    }
    return true;
}

bool copyFile(const base::Path& src, const base::Path& dest)
{
#ifdef _WIN32
    return ::CopyFileA(src.c_str(), dest.c_str(), false);
#else
    std::string cmd = formatr("cp \"%s\" \"%s\"", src.c_str(), dest.c_str());
    return base::shellCmd(cmd, false) == 0;
#endif
}

// Directory functions
bool createDirectory(const base::Path& dir)
{
    //dbglog("createDirectory(%s)\n", dir.c_str());
    if (existDirectory(dir))
    {
        return true;
    }
#ifdef _WIN32
    return CreateDirectoryA(dir.c_str(), NULL);
#else
	return mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
}

bool removeDirectory(const base::Path& dir)
{
    //dbglog("removeDirectory %s\n", dir.c_str());
    if (!existDirectory(dir))
    {
        return true;
    }
#ifdef _WIN32
    std::string cmd = formatr("rd /s /q \"%s\"", dir.c_str());
#else
    std::string cmd = formatr("rm -rf \"%s\"", dir.c_str());
#endif

    if (base::shellCmd(cmd, false) != 0)
    {
        //dbgerr("Failed to remove %s\n", dir.c_str());
        return false;
    }
    return true;
}

bool existDirectory(const base::Path& dirname)
{
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(dirname.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	struct stat buf;
	if (stat(dirname.c_str(), &buf) != 0)
    {
        return false;
    }
    return (buf.st_mode & S_IFDIR) != 0;
#endif
}

bool copyDirectory(const base::Path& srcdir, const base::Path& destdir)
{
#ifdef _WIN32
    std::string cmd = formatr("xcopy /S /F /I /Y /Q \"%s\" \"%s\"", srcdir.c_str(), destdir.c_str());
#else
    std::string cmd = formatr("cp -r \"%s\" \"%s\"", srcdir.c_str(), destdir.c_str());
#endif
    return base::shellCmd(cmd, false) == 0;
}

int shellCmd(const std::string& cmd, bool output, const base::Path* outfile)
{
    if (isFlagSet(sBaseGFlags, GFLAG_VERBOSE_SHELL))
    {
        output = true;
    }

    std::string ecmd(cmd);

    if (outfile)
    {
        ecmd += " > ";
        ecmd += *outfile;
        ecmd += " 2>&1";
    }
    else if (!output)
    {
#ifdef _WIN32
    ecmd += " > nul";
#else
    ecmd += " > /dev/null";
#endif
    }

    if (isFlagSet(sBaseGFlags, GFLAG_VERBOSE_SHELL))
    {
        printf("Shell [%s]\n", ecmd.c_str());
    }

    int exitcode = system(ecmd.c_str());

    if (isFlagSet(sBaseGFlags, GFLAG_VERBOSE_SHELL) && exitcode != 0)
    {
        dbgerr("Exitcode = %d\n", exitcode);
    }
    return exitcode;
}

bool listDirectory(const base::Path& dir, std::vector<base::Path>* filenames, std::vector<base::Path>* dirnames)
{
#ifdef _WIN32
    WIN32_FIND_DATAA ffd;
    base::Path ffdir;
    ffdir.set(dir, "*");

    HANDLE hfind = FindFirstFileA(ffdir.c_str(), &ffd);
    if (hfind == INVALID_HANDLE_VALUE)
    {
        dbgerr("Failed to open directory %s, err=%lu\n", ffdir.c_str(), GetLastError());
        return false;
    }
    do
    {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
        {
            continue;
        }

        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            if (filenames)
            {
                filenames->push_back(ffd.cFileName);
            }
        }
        else
        {
            if (dirnames)
            {
                dirnames->push_back(ffd.cFileName);
            }
        }
   }
   while (FindNextFileA(hfind, &ffd) != 0);
   FindClose(hfind);
   return true;

#else
    DIR* dd;
    struct dirent* fil;
    dd = opendir(dir.c_str());
    if (dd == NULL)
    {
        dbgerr("Failed to open directory %s\n", dir.c_str());
        closedir(dd);
        return false;
    }
    while ((fil = readdir(dd)) != NULL)
    {
        if (strcmp(fil->d_name, ".") == 0 || strcmp(fil->d_name, "..") == 0)
        {
            continue;
        }
        if (fil->d_type == DT_REG)
        {
            if (filenames)
            {
                filenames->push_back(fil->d_name);
            }
        }
        else if (fil->d_type == DT_DIR)
        {
            if (dirnames)
            {
                dirnames->push_back(fil->d_name);
            }
        }
    }
    closedir(dd);
    return true;
#endif
}

base::Path getProcPath(uint pid)
{
    char path[PATH_MAX + 1];
#ifdef _WIN32
    HANDLE han = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)pid);
    if (han)
    {
        if (GetModuleFileNameExA(han, 0, path, PATH_MAX))
        {
            //(PathFindFileName()
            return base::Path(path);
        }
        CloseHandle(han);
    }
#else
    char tmp[64];
    sprintf(tmp, "/proc/%d/exe", pid);
    ssize_t l = readlink(tmp, path, PATH_MAX);
    if (l != -1)
    {
        path[l] = '\0';
        return base::Path(path);
    }
#endif
    return base::Path();
}

uint getPid()
{
#ifdef _WIN32
    return (uint)GetCurrentProcessId();
#else
    return (uint)getpid();
#endif
}

bool currentDirectory(base::Path& dir)
{
    char buffer[PATH_MAX];
#ifdef _WIN32
	if ((_getcwd(buffer, PATH_MAX) != 0))
#else
	if ((getcwd(buffer, PATH_MAX) != 0))
#endif
    {
        dir.assign(buffer);
        return true;
    }
    return false;
}

bool changeDirectory(const base::Path& dir)
{
#ifdef _WIN32
    return _chdir(dir.c_str()) == 0;
#else
    return chdir(dir.c_str()) == 0;
#endif
}

bool getTempFilename(base::Path& tempfn)
{
#ifdef _WIN32
    char buffer[L_tmpnam_s];
    if (!tmpnam_s(buffer, L_tmpnam_s))
    {
        tempfn.assign(buffer);
        return true;
    }
#else
    for (int i = 0; i < 8; i++)
    {
        tempfn.assign(base::formatr("/tmp/ytf_%x", randInt() % 10000));
        if (!base::existFile(tempfn))
        {
            return true;
        }
    }
#endif
    return false;
}

bool getFileSize(const base::Path& filename, size_t& size)
{
	struct stat buf;
    size = 0;
	if (stat(filename.c_str(), &buf) != 0)
    {
        return false;
    }
    size = buf.st_size;
    return true;
}

} // base
