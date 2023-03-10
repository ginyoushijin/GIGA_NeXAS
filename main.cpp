#include <cstdio>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include "packFunc/packFunc.h"

bool IsDirectoryPath(const std::string& path)
{
    struct stat s;
    stat(path.c_str(), &s);
    return (s.st_mode & S_IFDIR) != 0;
}

int GetCompressionMethod(const char* name)
{
    if (stricmp(name, "zlib") == 0)
        return 4;
    else if (stricmp(name, "zstd") == 0)
        return 7;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        printf("NeXAS Pack Tool\n");
        printf("Usage:\n");
        printf("  Create Package  : Tool -c <no|zlib|zstd> <package.pac> <path/to/folder>\n");
        printf("  Extract Package : Tool -x <package.pac> <path/to/folder>\n");
        return 1;
    }

    // Replaces with std::string_view in C++17
    std::string cmd(argv[1]);

    if (cmd == "-c")
    {
        if (argc != 5)
        {
            printf("ERROR: Required 4 arguments.");
            return 1;
        }

        std::string pacPath(argv[3]);
        std::string dirPath(argv[4]);

        if (!IsDirectoryPath(dirPath))
        {
            printf("ERROR: Path is not a directory.");
            return 1;
        }

        int compressionMethod = GetCompressionMethod(argv[2]);

        CreatePackageMT(pacPath, dirPath, compressionMethod);
    }
    else if (cmd == "-x")
    {
        std::string pacPath(argv[2]);
        std::string dirPath(argv[3]);

        ExtractPackage(pacPath, dirPath);
    }
    else
    {
        printf("ERROR: Unknown command.");
        return 1;
    }

    return 0;
}
