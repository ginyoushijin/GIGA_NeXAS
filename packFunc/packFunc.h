#ifndef NEXAS_PACKFUNC
#define NEXAS_PACKFUNC

#include <vector>
#include <string>

struct PackageEntry
{
    char          Name[0x40];
    uint32_t      Position;
    uint32_t      OriginalSize;
    uint32_t      CompressedSize;
};

static_assert(sizeof(PackageEntry) == 0x4C, "The size of PackageEntry must be 4C");

bool CreatePackage(const std::string& pacPath, const std::string& dirPath, int compressionMethod,int codePage);
bool CreatePackageMT(const std::string& pacPath, const std::string& dirPath, int compressionMethod,int codePage);
bool ExtractPackage(const std::string& pacPath, const std::string& dirPath,int codePage);

#endif
