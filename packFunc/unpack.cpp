
// For SHCreateDirectoryExA
#include <windows.h>
#include <shlobj.h>

#undef min
#undef max

#include "packFunc.h"
#include "../huffman/huffman.h"
#include "../quote/header/zlib.h"
#include "../quote/header/zstd.h"

#include <chrono>
#include <thread>
#include <future>
#include <list>

using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;

bool WriteToFile(const std::string& path, const void* data, size_t size)
{
    FILE* fp = fopen(path.c_str(), "wb");

    if (fp)
    {
        if (fwrite(data, size, 1, fp) == 1)
        {
            fclose(fp);
            return true;
        }

        fclose(fp);
    }

    return false;
}


/**
 * @brief 解压文件到指定目录
 * 
 * @param fp 封包文件
 * @param entries 文件索引
 * @param count 文件数量
 * @param compressionMethod 压缩方式
 * @param dirPath 输出目录路径
 */
bool ExtractEntry(FILE *fp, PackageEntry* entries, uint32_t count, uint32_t compressionMethod, const std::string& dirPath, bool closeFile)
{
    std::vector<uint8_t> uncompressedData;
    std::vector<uint8_t> compressedData;

    for (uint32_t i = 0; i < count; i++)
    {
        // printf("Extract %s\n", entries[i].Name);

        fseek(fp, entries[i].Position, SEEK_SET);

        if (compressionMethod != 0)
        {
            uncompressedData.resize(entries[i].OriginalSize);
            compressedData.resize(entries[i].CompressedSize);

            fread(compressedData.data(), entries[i].CompressedSize, 1, fp);

            if (compressionMethod == 4)
            {
                uLong sourceLen = entries[i].CompressedSize;
                uLongf destLen = entries[i].OriginalSize;
                
                int result = uncompress((Bytef*)uncompressedData.data(), &destLen, (Bytef*)compressedData.data(), sourceLen);

                if (result != Z_OK)
                {
                    printf("ERROR: Failed to uncompress %s with zlib.\n", entries[i].Name);
                    continue;
                }
            }
            else if (compressionMethod == 7)
            {
                size_t result = ZSTD_decompress(uncompressedData.data(), entries[i].OriginalSize, compressedData.data(), entries[i].CompressedSize);

                if (ZSTD_isError(result))
                {
                    printf("ERROR: Failed to uncompress %s with zstd(%s).\n", entries[i].Name, ZSTD_getErrorName(result));
                    continue;
                }
            }
        }
        else
        {
            uncompressedData.resize(entries[i].CompressedSize);
            fread(uncompressedData.data(), entries[i].CompressedSize, 1, fp);
        }
        
        std::string path = dirPath + "\\" + entries[i].Name;

        WriteToFile(path, uncompressedData.data(), uncompressedData.size());

    }

    if (closeFile)
    {
        fclose(fp);
    }

    return true;
}

bool ExtractEntryMT(const std::string& pacPath, PackageEntry* entries, uint32_t count, uint32_t compressionMethod, const std::string& dirPath)
{
    auto maxThreads = std::thread::hardware_concurrency();
    auto filesPerThread = (uint32_t) ceilf( (float)count / (float)maxThreads );

    uint32_t remaining = count;
    uint32_t j = 0;

    std::list<std::future<bool>> tasks;

    for (uint32_t i = 0; i < maxThreads; i++)
    {
        uint32_t processCount = std::min(remaining, filesPerThread);
        remaining -= processCount;

        printf("Start worker thread to processing [%d,%d]\n", j, j + processCount - 1);

        auto fp = fopen(pacPath.c_str(), "rb");

        if (fp)
        {
            auto startEntry = entries + j;
            auto task = std::async(std::launch::async, ExtractEntry, fp, startEntry, processCount, compressionMethod, dirPath, true);
            tasks.emplace_back(std::move(task));
        }

        j += processCount;
    }

    for (auto& t : tasks)
    {
        t.get();
    }

    return true;
}

/**
 * @brief 解包
 *
 * @param pacPath 封包文件路径
 * @param dirPath 输出目录路径
 */
bool ExtractPackage(const std::string& pacPath, const std::string& dirPath)
{
    FILE* fp = fopen(pacPath.c_str(), "rb");

    if (!fp)
    {
        printf("ERROR: Failed to open package file.");
        return false;
    }

    uint8_t magic[4];
    fread(magic, 4, 1, fp);

    if (magic[0] != 0x50 || magic[1] != 0x41 || magic[2] != 0x43)
    {
        fclose(fp);
        printf("ERROR: Invalid package file.");
        return false;
    }

    uint32_t entryCount;
    uint32_t compressionMethod;
    uint32_t compressedIndexSize;

    fread(&entryCount, 4, 1, fp);
    fread(&compressionMethod, 4, 1, fp);

    printf("Total %d files in the package.\n", entryCount);

    fseek(fp, -4, SEEK_END);
    fread(&compressedIndexSize, 4, 1, fp);

    std::vector<uint8_t> compressedIndex;
    compressedIndex.resize(compressedIndexSize);

    fseek(fp, -(compressedIndexSize + 4), SEEK_END);
    fread(compressedIndex.data(), compressedIndexSize, 1, fp);

    // Decrypt index
    for (uint32_t i = 0; i < compressedIndexSize; i++)
    {
        compressedIndex[i] = ~compressedIndex[i];
    }

    uint32_t indexSize = sizeof(PackageEntry) * entryCount;

    std::vector<uint8_t> index;
    index.resize(indexSize);

    // Decompress index
    Huffman* huffman = new Huffman(compressedIndex.data(), compressedIndexSize);
    uint32_t dummy = 0;
    huffman->decode(index.data(), indexSize, &dummy);
    delete huffman;

    // 偷懒方式创建文件夹
    SHCreateDirectoryExA(NULL, dirPath.c_str(), NULL);

    auto tp1 = steady_clock::now();

    //ExtractEntry(fp, (PackageEntry*)index.data(), entryCount, compressionMethod, dirPath, false);
    ExtractEntryMT(pacPath, (PackageEntry*)index.data(), entryCount, compressionMethod, dirPath);

    auto tp2 = steady_clock::now();

    auto ms = duration_cast<milliseconds>(tp2 - tp1).count();

    printf("Extracted %d files in %llu ms.\n", entryCount, ms);

    fclose(fp);

    return true;
}