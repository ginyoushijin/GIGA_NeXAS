#ifndef NEXAS_HUFFMAN
#define NEXAS_HUFFMAN

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <map>

#include "../packFunc/packFunc.h"

//typedef struct PackageEntry_s PackageEntry;

typedef struct HuffmanNode
{
    BYTE symbol = NULL;                                            // 当前节点的值
    DWORD weight = 0;                                              // 当前节点的权重
    char* path=nullptr;                                            //节点在树中的路径，即前缀码
    struct HuffmanNode *leftNode = nullptr, *rightNode = nullptr; // 该节点的左右子节点

} HuffmanNode;

class Huffman
{
private:
    BYTE val = 0;                // 当前byte的值
    DWORD bitCount = 0;          // 当前byte剩余的可用bit数
    DWORD byteCount = 0;         // stream下一个将要读取的byte的index
    HuffmanNode *root = nullptr; // 构造的huffman树的根节点
    BYTE *buffer = nullptr;      // 缓冲区指针
    DWORD bufferSize = 0;        // 缓冲区size

    bool setBits(std::vector<BYTE>* buffer,DWORD setbit,DWORD val);

    bool setBits(std::vector<BYTE>* buffer,DWORD setbit,const char* const path);

    bool getBits(DWORD getBit, _Out_ DWORD *bitVal);

    bool huffmanTreeToBitStream(std::vector<BYTE>* buffer,HuffmanNode* node);

    bool bitStreamToHuffmanTree(HuffmanNode *node);

    bool destroyHuffmanNode(HuffmanNode* node);

public:
    Huffman() = default;

    Huffman(BYTE *buffer, DWORD bufferSize);

    virtual ~Huffman();

    bool decode(BYTE *decodeBuffer, DWORD decodeBufferSize, _Out_ uint32_t *readByte);

    bool encode(const uint8_t* input, size_t inputSize, std::vector<uint8_t>& encodeBuffer);
};

#endif // NEXAS_HUFFMAN