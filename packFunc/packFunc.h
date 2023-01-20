#ifndef NEXAS_PACKFUNC
#define NEXAS_PACKFUNC

#include <stdio.h>
#include <Windows.h>
#include <vector>

#include "../huffman/huffman.h"
#include "../quote/header/zconf.h"
#include "../quote/header/zlib.h"
#include "../quote/header/zstd.h"

typedef struct pack_file
{
    char fileName[0x40] = {0};  //文件名
    DWORD offsetInPack = 0; //压缩文件在封包中的偏移
    DWORD fileSize = 0; //文件解压后的真实大小
    DWORD compSize = 0; //文件在封包中的压缩大小
} PackFile;

bool fileUnpack(char *packPath);

bool fileTopack(char *dirPath,DWORD compMethod=7);

#endif // NEXAS_PACKFUNC