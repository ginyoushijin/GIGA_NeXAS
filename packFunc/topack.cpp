#include "packFunc.h"

#include "enc.hpp"
#include "huffman/huffmanEncoder.h"
#include "quote/header/zlib.h"
#include "quote/header/zstd.h"

#include <thread>
#include <future>
#include <chrono>
#include <list>
#include <io.h>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

/**
 * @brief 遍历目标文件夹以及子文件夹,获取文件目录
 * 
 * @param[in] path 目标文件夹路径
 * @return std::list<std::string> 文件名list 
 */
std::list<std::string> GetFilesInDirectory(const std::string &path)
{
    // Result file paths
    std::list<std::string> filePaths;
    // Directory paths to search
    std::list<std::string> directoryPaths;  //需要遍历的文件夹list,用list的方式来避免递归

    // Add root path to start
    directoryPaths.emplace_back(path);  //添加目标文件夹

    while (!directoryPaths.empty())
    {
        // Get path
        std::string currentPath = std::move(directoryPaths.front());
        directoryPaths.pop_front();

        // Make full path search pattern
        std::string pattern;
        pattern.reserve(currentPath.size() + 4);    //预先设置容量，防止再分配
        pattern.append(currentPath);
        pattern.append("\\*.*");

        // Start search
        _finddata_t data;
        intptr_t handle = _findfirst(pattern.c_str(), &data);

        if (handle == -1)
        {
            // Ignore invalid path
            continue;
        }

        do
        {
            size_t nameLength = strlen(data.name);

            if (data.attrib & _A_SUBDIR)
            {
                // We need ignore special directory e.g "." and ".."
                int j = 0;

                while (j < nameLength)
                {
                    if (data.name[j] != '.')    //文件夹名不能带.
                    {
                        // Should be a valid subdirectory name
                        break;
                    }
                    j++;
                }

                if (j < nameLength)
                {
                    // Append subdirectory path to the list and search later
                    std::string newPath;
                    newPath.reserve(currentPath.size() + 1 + nameLength);
                    newPath.append(currentPath);
                    newPath.push_back('\\');
                    newPath.append(data.name);
                    directoryPaths.emplace_back(std::move(newPath));
                }
            }
            else
            {
                // Append file path to the result
                std::string newPath;
                newPath.reserve(currentPath.size() + 1 + nameLength);
                newPath.append(currentPath);
                newPath.push_back('\\');
                newPath.append(data.name);
                filePaths.emplace_back(std::move(newPath));
            }

        } while (_findnext(handle, &data) == 0);

        _findclose(handle);
    }

    return filePaths;
}

/**
 * @brief 根据文件路径获取文件名
 * 
 * @param path 文件路径
 * @return std::string 文件名
 */
std::string GetFileName(const std::string &path)
{
    size_t pos;

    //判断分隔符为\或/的两种情况
    pos = path.find_last_of('\\');

    if (pos != std::string::npos)
    {
        return path.substr(pos + 1);
    }

    pos = path.find_last_of('/');

    if (pos != std::string::npos)
    {
        return path.substr(pos + 1);
    }

    return path;
}

/**
 * @brief 读取文件
 * @param[in] 目标文件
 * 
 * @return std::vector<uint8_t> 读取后的缓冲区
 */
std::vector<uint8_t> ReadFileData(const std::string &path)
{
    FILE *fp = fopen(path.c_str(), "rb");

    if (fp) //判断io
    {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (size > 0)
        {
            std::vector<uint8_t> buffer;
            buffer.resize(size);

            if (fread(buffer.data(), size, 1, fp) == 1)
            {
                fclose(fp);
                return buffer;
            }
        }

        fclose(fp);
    }

    return {};
}

//////////////////////////////////////////////////////////////////
// 单线程处理
//////////////////////////////////////////////////////////////////

bool CreatePackage(const std::string &pacPath, const std::string &dirPath, int compressionMethod,int codePage)
{
    auto tp1 = steady_clock::now();

    auto files = GetFilesInDirectory(dirPath);  //声明时调用函数初始化，编译器自动传入目标变量指针来优化，防止拷贝

    if (files.empty())
    {
        printf("ERROR: No any files to pack.");
        return false;
    }

    std::vector<PackageEntry> entries;
    entries.resize(files.size());

    auto fp = fopen(pacPath.c_str(), "wb");

    if (!fp)
    {
        printf("ERROR: Failed to create package file.");
        return false;
    }

    uint8_t magic[] = {0x50, 0x41, 0x43, 0x75};
    fwrite(magic, 4, 1, fp);

    // 这里先不写数量，因为读取或压缩的时候可能会发生错误，跳过一些文件，所以后面再更新数量
    uint32_t entryCount = 0;

    fwrite(&entryCount, 4, 1, fp);
    fwrite(&compressionMethod, 4, 1, fp);

    // 实际处理了的文件数量
    int i = 0;

    std::vector<uint8_t> compressedData;

    for (auto &path : files)    //遍历所有文件
    {
        auto name = GetFileName(path);

        if(codePage==CP_UTF8) AnsiToUTF8(name);
 
        if (name.size() >= sizeof(PackageEntry::Name))  //防止文件名过长
        {
            printf("WARNING: File name '%s' too long.", name.c_str());
            continue;
        }

        auto data = ReadFileData(path);

        if (data.empty())  //防止读取失败
        {
            printf("ERROR: Failed to read file '%s'.", path.c_str());
            continue;
        }

        uint32_t position = ftell(fp);
        uint32_t originalSize = data.size();
        uint32_t compressedSize = originalSize;

        //根据文件扩展名，排除掉一些文件，不进行压缩
        auto extension = name.substr(name.find_last_of('.'));
        if (extension == ".ogg" || extension == ".png" || extension == ".wav" || extension == ".fnt")
        {
            fwrite(data.data(), originalSize, 1, fp);
        }
        else
        {

            if (compressionMethod == 4)
            {
                uLong sourceLen = data.size();
                uLong destLen = compressBound(sourceLen);

                compressedData.resize(destLen);

                // int result = compress2(compressedData.data(), &destLen, data.data(), sourceLen, Z_BEST_COMPRESSION);
                int result = compress(compressedData.data(), &destLen, data.data(), sourceLen);

                if (result != Z_OK)
                {
                    printf("ERROR: Failed to compress file '%s' with zlib.", path.c_str());
                    continue;
                }

                compressedData.resize(destLen);
                compressedSize = destLen;

                fwrite(compressedData.data(), compressedSize, 1, fp);
            }
            else if (compressionMethod == 7)
            {
                size_t srcSize = data.size();
                size_t dstSize = ZSTD_compressBound(srcSize);

                compressedData.resize(dstSize);

                size_t result = ZSTD_compress(compressedData.data(), dstSize, data.data(), srcSize, ZSTD_maxCLevel());

                if (ZSTD_isError(result))
                {
                    printf("ERROR: Failed to compress file '%s' with zstd.", path.c_str());
                    continue;
                }

                compressedData.resize(result);
                compressedSize = result;

                fwrite(compressedData.data(), compressedSize, 1, fp);
            }
            else
            {
                fwrite(data.data(), originalSize, 1, fp);
            }
        }
        auto &entry = entries[i];

        strcpy_s(entry.Name, name.c_str());
        entry.Position = position;
        entry.OriginalSize = originalSize;
        entry.CompressedSize = compressedSize;

        i++;
    }

    entryCount = i;

    uint8_t* const index = reinterpret_cast<uint8_t*>(entries.data());
    auto indexSize = sizeof(PackageEntry) * entryCount;

    HuffmanEncoder huffmanEncoder;
    std::vector<uint8_t> compressedIndex = huffmanEncoder.Encode(index,indexSize);

    uint32_t compressedIndexSize = compressedIndex.size();

    // Encrypt index
    for (uint32_t i = 0; i < compressedIndexSize; i++)
    {
        compressedIndex[i] = ~compressedIndex[i];
    }

    fwrite(compressedIndex.data(), compressedIndexSize, 1, fp);
    fwrite(&compressedIndexSize, 4, 1, fp);

    // 回去更新文件数量
    fseek(fp, 4, SEEK_SET);
    fwrite(&entryCount, 4, 1, fp);

    fflush(fp); //刷新缓冲区
    fclose(fp);

    auto tp2 = steady_clock::now();

    auto ms = duration_cast<milliseconds>(tp2 - tp1).count();

    printf("[ST] Packed %d files in %llu ms.\n", entryCount, ms);

    return true;
}

//////////////////////////////////////////////////////////////////
// 多线程处理
//////////////////////////////////////////////////////////////////

struct FileData
{
    std::string Name;   //文件名
    std::vector<uint8_t> Data;  //压缩后数据的缓冲区
    uint32_t OriginalSize = 0;  //原始size
    uint32_t CompressedSize = 0;    //压缩后size
};

/**
 * @brief 
 * @param[in] path  目标文件
 * @param[in] compressionMethod 压缩方式  
 * 
 * @return FileDate 压缩后写入封包需要的信息
 */
FileData ReadAndCompressFile(const std::string &path, int compressionMethod,int codePage)
{
    auto name = GetFileName(path);

    if(codePage==CP_UTF8) name = AnsiToUTF8(name); 

    if (name.size() >= sizeof(PackageEntry::Name))
    {
        printf("WARNING: File name '%s' too long.", name.c_str());
        return {};
    }

    auto data = ReadFileData(path);

    if (data.empty())
    {
        printf("ERROR: Failed to read file '%s'.", path.c_str());
        return {};
    }

    std::vector<uint8_t> compressedData;

    //根据文件扩展名，排除掉一些文件，不进行压缩
    auto extension = name.substr(name.find_last_of('.'));

    if (extension == ".ogg" || extension == ".png" || extension == ".wav" || extension == ".fnt")
    {
        size_t srcSize = data.size();

        return {std::move(name), std::move(data), (uint32_t)srcSize, (uint32_t)srcSize};
    }
    else
    {
        if (compressionMethod == 4)
        {
            uLong sourceLen = data.size();
            uLong destLen = compressBound(sourceLen);

            compressedData.resize(destLen);

            int result = compress2(compressedData.data(), &destLen, data.data(), sourceLen, Z_BEST_COMPRESSION);

            if (result != Z_OK)
            {
                printf("ERROR: Failed to compress file '%s' with zlib.", path.c_str());
            }

            compressedData.resize(destLen);

            return {std::move(name), std::move(compressedData), (uint32_t)data.size(), destLen};
        }
        else if (compressionMethod == 7)
        {
            size_t srcSize = data.size();
            size_t dstSize = ZSTD_compressBound(srcSize);

            compressedData.resize(dstSize);

            size_t result = ZSTD_compress(compressedData.data(), dstSize, data.data(), srcSize, ZSTD_maxCLevel());

            if (ZSTD_isError(result))
            {
                printf("ERROR: Failed to compress file '%s' with zstd.", path.c_str());
            }

            compressedData.resize(result);

            return {std::move(name), std::move(compressedData), (uint32_t)data.size(), (uint32_t)result};
        }
        else
        {
            size_t srcSize = data.size();

            return {std::move(name), std::move(data), (uint32_t)srcSize,(uint32_t)srcSize};
        }
    }
}

/**
 * @brief 多线程压缩
 * 
 * @param pacPath 要写入的目标封包
 * @param dirPath 源文件夹
 * @param compressionMethod 压缩方式 
 * @return 函数执行结果
 */
bool CreatePackageMT(const std::string &pacPath, const std::string &dirPath, int compressionMethod,int codePage)
{
    auto tp1 = steady_clock::now();

    auto files = GetFilesInDirectory(dirPath);

    if (files.empty())
    {
        printf("ERROR: No any files to pack.");
        return false;
    }

    // 创建索引数据块，内存连续
    std::vector<PackageEntry> entries;
    entries.resize(files.size());

    FILE *fp = fopen(pacPath.c_str(), "wb");

    if (!fp)
    {
        printf("ERROR: Failed to create package file.");
        return false;
    }

    printf("Total %d files to pack.\n", files.size());

    // 写文件头
    uint8_t magic[] = {0x50, 0x41, 0x43, 0x75};
    fwrite(magic, 4, 1, fp);

    // 这里先不写数量，因为读取或压缩的时候可能会发生错误，跳过一些文件，所以后面再更新数量
    uint32_t entryCount = 0;

    fwrite(&entryCount, 4, 1, fp);
    fwrite(&compressionMethod, 4, 1, fp);

    // 实际处理了的文件数量
    uint32_t i = 0;

    // 取CPU线程数量
    uint32_t maxThreads = std::thread::hardware_concurrency();

    std::vector<std::future<FileData>> tasks;  //任务池
    tasks.reserve(maxThreads);

    while (!files.empty())
    {
        // 创建线程并行读取和压缩

        tasks.clear();

        for (uint32_t n = 0; n < maxThreads; n++)
        {
            if (files.empty())
                break;

            // 取出一个文件名然后创建线程来读取
            auto &path = files.front();
            auto task = std::async(std::launch::async, ReadAndCompressFile, std::move(path), compressionMethod,codePage);    //std::launch::async 强制创建新线程执行
            tasks.emplace_back(std::move(task));
            files.pop_front();
        }

        // 获取文件数据

        for (auto &task : tasks)
        {
            auto result = task.get();

            if (result.Data.empty())
                continue; // 读取或者压缩失败了

            auto &entry = entries[i];

            strcpy_s(entry.Name, result.Name.c_str());
            entry.Position = ftell(fp);
            entry.OriginalSize = result.OriginalSize;
            entry.CompressedSize = result.CompressedSize;

            fwrite(result.Data.data(), result.Data.size(), 1, fp);

            i++;
        }
    }

    entryCount = i;

    uint8_t* const index = reinterpret_cast<uint8_t*>(entries.data());
    auto indexSize = sizeof(PackageEntry) * entryCount;

    // 压缩索引
    HuffmanEncoder huffmanEncoder;
    std::vector<uint8_t> compressedIndex = huffmanEncoder.Encode(index,indexSize);

    uint32_t compressedIndexSize = compressedIndex.size();

    // 加密
    for (uint32_t i = 0; i < compressedIndexSize; i++)
    {
        compressedIndex[i] = ~compressedIndex[i];
    }

    fwrite(compressedIndex.data(), compressedIndexSize, 1, fp);
    fwrite(&compressedIndexSize, 4, 1, fp);

    // 回去更新文件数量
    fseek(fp, 4, SEEK_SET);
    fwrite(&entryCount, 4, 1, fp);

    fflush(fp);
    fclose(fp);

    auto tp2 = steady_clock::now();

    auto ms = duration_cast<milliseconds>(tp2 - tp1).count();

    printf("[MT] Packed %d files in %llu ms.\n", entryCount, ms);

    return true;
}
