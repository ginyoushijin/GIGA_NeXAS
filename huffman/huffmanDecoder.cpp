#include "huffmanDecoder.h"

#include <stack>

uint32_t HuffmanDecoder::GetBits(uint32_t needBit)
{
    uint32_t result = 0;

    uint32_t readBit = 0;

    while (needBit > 0)
    {
        if (!this->_bitCount)
        {
            if (this->_byteCount >= this->_bufferSize)
            {
                return 0;
            }
            this->_curValue = this->_buffer[this->_byteCount++];
            this->_bitCount = 8;
        }

        readBit = needBit > this->_bitCount ? this->_bitCount : needBit;

        result <<= readBit;
        result |= this->_curValue >> (this->_bitCount - readBit);
        this->_curValue &= (1 << (this->_bitCount - needBit)) - 1;

        this->_bitCount -= readBit;
        needBit -= readBit;
    }

    return result;
}

void HuffmanDecoder::ParseBitStreamToHuffmanTree()
{
    std::stack<std::shared_ptr<HuffmanNode>> nodeStack;

    this->_root = std::make_shared<HuffmanNode>();

    nodeStack.push(this->_root);

    auto curNode = nodeStack.top();

    while (!nodeStack.empty())
    {

        if (this->GetBits(1))
        {

            if (curNode->leftNode == nullptr)
            {
                nodeStack.push(curNode);
                curNode->leftNode = std::make_shared<HuffmanNode>();
                curNode = curNode->leftNode;
            }
            else
            {
                nodeStack.push(curNode);
                curNode->rightNode = std::make_shared<HuffmanNode>();
                curNode = curNode->rightNode;
            }
        }
        else
        {
            curNode->symbol = static_cast<uint8_t>(this->GetBits(8));

            curNode = nodeStack.top();
            nodeStack.pop();

            if (curNode->rightNode == nullptr)
            {
                curNode->rightNode = std::make_shared<HuffmanNode>();
                curNode = curNode->rightNode;
            }
        }
    }
}

uint32_t HuffmanDecoder::Decode(uint8_t* const decodeBuffer, uint32_t decodeBufferSize)
{
    if (!decodeBuffer)
        return 0;

    this->ParseBitStreamToHuffmanTree();

    uint32_t decodeCount = 0;

    auto curNode = this->_root;

    while (true)
    {

        if (decodeCount >= decodeBufferSize)
            break;

        if (!this->GetBits(1))
        {
            curNode = curNode->leftNode;
        }
        else
        {
            curNode = curNode->rightNode;
        }

        if (curNode->leftNode == nullptr && curNode->rightNode == nullptr)
        {
            decodeBuffer[decodeCount++] = curNode->symbol;
            curNode = this->_root;
        }
    }

    return decodeCount;
}