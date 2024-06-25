#include "packFunc.h"

#include "enc.hpp"
#include "huffman/huffmanDecoder.h"
#include "quote/header/zlib.h"
#include "quote/header/zstd.h"

// For SHCreateDirectoryExA
#include <windows.h>
#include <shlobj.h>
#include <chrono>
#include <thread>
#include <future>
#include <list>
#include <iostream>

#undef min
#undef max

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

/**
 * @brief 写入文件
 *
 * @param path 要写入的目标文件
 * @param data 源缓冲区
 * @param size 缓冲区size
 * @return 函数执行结果
 */
bool WriteToFile(const std::wstring &path, const void *data, size_t size)
{
    FILE *fp = _wfopen(path.c_str(), L"wb");

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
 * @param closeFile 是否关闭文件stream
 * @return 成功导出的文件数
 */
size_t ExtractEntry(FILE *fp, PackageEntry *entries, uint32_t count, uint32_t compressionMethod, const std::string &dirPath,int codePage ,bool closeFile)
{
    std::vector<uint8_t> uncompressedData;
    std::vector<uint8_t> compressedData;
    size_t extractCount = 0;

    for (uint32_t i = 0; i < count; i++)
    {
        // printf("Extract %s\n", entries[i].Name);

        fseek(fp, entries[i].Position, SEEK_SET);

        if (compressionMethod != 0) // 判断文件是否被压缩
        {
            uncompressedData.resize(entries[i].OriginalSize);
            compressedData.resize(entries[i].CompressedSize);

            if (entries[i].OriginalSize != entries[i].CompressedSize)
            {
                fread(compressedData.data(), entries[i].CompressedSize, 1, fp);

                if (compressionMethod == 4)
                {
                    uLong sourceLen = entries[i].CompressedSize;
                    uLongf destLen = entries[i].OriginalSize;

                    int result = uncompress((Bytef *)uncompressedData.data(), &destLen, (Bytef *)compressedData.data(), sourceLen);

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
                fread(uncompressedData.data(), entries[i].CompressedSize, 1, fp);
            }
        }
        else
        {
            uncompressedData.resize(entries[i].CompressedSize);
            fread(uncompressedData.data(), entries[i].CompressedSize, 1, fp);
        }

        std::wstring path = AnsiToUnicode(dirPath, CP_ACP) + L"\\" + AnsiToUnicode(entries[i].Name, codePage);

        if (WriteToFile(path, uncompressedData.data(), uncompressedData.size()))    extractCount++;
    }

    if (closeFile)
    {
        fclose(fp);
    }

    return extractCount;
}

/**
 * @brief 多线程导出文件
 *
 * @param pacPath 目标封包
 * @param entries 封包内文件信息list
 * @param count 封包文件计数
 * @param compressionMethod 封包压缩方式
 * @param dirPath 导出的目标文件夹
 * @return 成功导出的文件数
 */
size_t ExtractEntryMT(const std::string &pacPath, PackageEntry *entries, uint32_t count, uint32_t compressionMethod, const std::string &dirPath,int codePage)
{
    auto maxThreads = std::thread::hardware_concurrency();
    auto filesPerThread = (uint32_t)ceilf((float)count / (float)maxThreads); // 单线程处理的文件数，向上取整

    size_t extractCount = 0;    // 导出的文件数
    uint32_t remaining = count; // 未处理的文件数
    uint32_t j = 0;

    std::list<std::future<size_t>> tasks;

    for (uint32_t i = 0; i < maxThreads; i++)
    {
        uint32_t processCount = std::min(remaining, filesPerThread); // 当前线程要处理的文件数
        remaining -= processCount;

        // printf("Start worker thread to processing [%d,%d]\n", j, j + processCount - 1); // 打印当前线程处理的文件的index

        auto fp = fopen(pacPath.c_str(), "rb");

        if (fp)
        {
            auto startEntry = entries + j;
            auto task = std::async(std::launch::async, ExtractEntry, fp, startEntry, processCount, compressionMethod, dirPath, codePage ,true);
            tasks.emplace_back(std::move(task));
        }

        j += processCount;
    }

    for (auto &t : tasks) extractCount += t.get();

    return extractCount;
}

/**
 * @brief 解包
 *
 * @param pacPath 封包文件路径
 * @param dirPath 输出目录路径
 * @return 函数执行结果
 */
bool ExtractPackage(const std::string &pacPath, const std::string &dirPath,int codePage)
{
    FILE *fp = fopen(pacPath.c_str(), "rb");

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
    HuffmanDecoder huffmanDecoder(compressedIndex.data(),compressedIndexSize);
    huffmanDecoder.Decode(index.data(),indexSize);

    // 偷懒方式创建文件夹
    SHCreateDirectoryExA(NULL, dirPath.c_str(), NULL);

    auto tp1 = steady_clock::now();

    // ExtractEntry(fp, (PackageEntry*)index.data(), entryCount, compressionMethod, dirPath, false);
    size_t extractCount = ExtractEntryMT(pacPath, (PackageEntry *)index.data(), entryCount, compressionMethod, dirPath,codePage);

    auto tp2 = steady_clock::now();

    auto ms = duration_cast<milliseconds>(tp2 - tp1).count();

    printf("Extracted %d files in %llu ms.\n", extractCount, ms);

    fclose(fp);

    return true;
}