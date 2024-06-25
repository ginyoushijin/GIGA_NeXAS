#ifndef NEXAS_HUFFMAN_ENCODER_H
#define NEXAS_HUFFMAN_ENCODER_H

#include "huffmanNode.h"

#include <vector>

class HuffmanEncoder
{
private:
    void SetBits(std::vector<uint8_t>& buffer,uint32_t setBit,uint32_t setValue);

    void ConstructionPath(std::shared_ptr<HuffmanNode>& node,const std::vector<bool>& nodePath,const uint32_t branch);

    void ParseHuffmanTreeToBitStream(std::vector<uint8_t>& streamBuffer,const std::shared_ptr<HuffmanNode>& node);

public:
    HuffmanEncoder() = default;

    ~HuffmanEncoder() = default;

    std::vector<uint8_t> Encode(uint8_t* const buffer,uint32_t bufferSize); 

private:
    std::shared_ptr<HuffmanNode> _root = nullptr;
    uint32_t _bitCount = 8;
    uint32_t _curValue = 0;
};

#endif // NEXAS_HUFFMAN_ENCODER_H