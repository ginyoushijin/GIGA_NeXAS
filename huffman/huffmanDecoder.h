#ifndef NEXAS_HUFFMAN_DECODER_H
#define NEXAS_HUFFMAN_DECODER_H

#include "huffmanNode.h"

class HuffmanDecoder
{
private:
    HuffmanDecoder() = delete;

    void ParseBitStreamToHuffmanTree();

    uint32_t GetBits(uint32_t getCount);

public:
    HuffmanDecoder(uint8_t const * const buffer, uint32_t bufferSize) : _buffer(buffer), _bufferSize(bufferSize){}
    
    ~HuffmanDecoder() = default;

    uint32_t Decode(uint8_t* const decodeBuffer, uint32_t decodeBufferSize);

private:
    uint8_t const * const _buffer = nullptr;
    uint32_t _bufferSize = 0;
    uint32_t _bitCount = 0;
    uint32_t _byteCount = 0;  
    uint32_t _curValue = 0;
    std::shared_ptr<HuffmanNode> _root = nullptr;
};


#endif // NEXAS_HUFFMAN_DECODER_H