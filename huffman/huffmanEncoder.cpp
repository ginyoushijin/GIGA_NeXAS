#include "huffmanEncoder.h"

#include <map>
#include <deque>
#include <list>
#include <algorithm>

void HuffmanEncoder::SetBits(std::vector<uint8_t>& buffer,uint32_t setBit,uint32_t setValue)
{
    uint32_t writeBit = 0;

    while(setBit > 0)
    {
        if(!this->_bitCount)
        {
            buffer.emplace_back(this->_curValue);
            this->_bitCount = 8;
            this->_curValue = 0;
        }

        writeBit = this->_bitCount < setBit ? this->_bitCount : setBit;
        
        if(this->_bitCount < setBit)
        {
            this->_curValue |= (setValue>>(setBit-this->_bitCount));
            setValue &= (1<<(setBit-this->_bitCount)) -1 ; 
        }
        else
        {
            this->_curValue |= (setValue << (this->_bitCount-setBit));
        }

        setBit-= writeBit;
        this->_bitCount-= writeBit;

    }
}

void HuffmanEncoder::ParseHuffmanTreeToBitStream(std::vector<uint8_t> &streamBuffer, const std::shared_ptr<HuffmanNode> &node)
{
    if(node->leftNode==nullptr && node->rightNode==nullptr)
    {
        this->SetBits(streamBuffer,1,0);
        this->SetBits(streamBuffer,8,node->symbol);
    }
    else
    {
        this->SetBits(streamBuffer,1,1);
        if(node->leftNode!=nullptr) this->ParseHuffmanTreeToBitStream(streamBuffer,node->leftNode); 
        if(node->rightNode!=nullptr) this->ParseHuffmanTreeToBitStream(streamBuffer,node->rightNode);
    }
}

void HuffmanEncoder::ConstructionPath(std::shared_ptr<HuffmanNode> &node, const std::vector<bool> &parentPath, const uint32_t branch)
{

    node->path.assign(parentPath.begin(), parentPath.end());
    node->path.emplace_back(branch);

    if (node->leftNode != nullptr)
    {
        this->ConstructionPath(node->leftNode, node->path, 0);
    }
    if (node->rightNode != nullptr)
    {
        this->ConstructionPath(node->rightNode, node->path, 1);
    }
}

std::vector<uint8_t> HuffmanEncoder::Encode(uint8_t *const buffer, uint32_t bufferSize)
{

    std::deque<std::shared_ptr<HuffmanNode>> nodeDueqe;
    std::map<uint8_t, std::shared_ptr<HuffmanNode>> nodeMap;

    for (uint32_t i = 0; i < bufferSize; ++i)
    {
        uint8_t c = buffer[i];

        if (nodeMap[c] == nullptr)
        {
            nodeDueqe.emplace_back(std::make_shared<HuffmanNode>());

            nodeMap[c] = nodeDueqe.back();

            nodeMap[c]->symbol = c;

            nodeMap[c]->weight = 1;
        }
        else
        {
            nodeMap[c]->weight++;
        }
    }

    if (nodeDueqe.size() > 2)
        std::sort(nodeDueqe.begin(), nodeDueqe.end(),
                  [](std::shared_ptr<HuffmanNode> sun, std::shared_ptr<HuffmanNode> moon)
                  {
                      return sun->weight > moon->weight;
                  });

    std::shared_ptr<HuffmanNode> curNode = nullptr;

    while (nodeDueqe.size() > 1)
    {
        curNode = std::make_shared<HuffmanNode>();

        auto leftNode = nodeDueqe.back();
        nodeDueqe.pop_back();
        auto rightNode = nodeDueqe.back();
        nodeDueqe.pop_back();

        curNode->leftNode = leftNode;
        curNode->rightNode = rightNode;

        curNode->weight = leftNode->weight + rightNode->weight;

        if (nodeDueqe.size() > 1)
        {
            if (curNode->weight > (nodeDueqe[nodeDueqe.size() - 1]->weight + nodeDueqe[nodeDueqe.size() - 2]->weight))
            {
                nodeDueqe.insert(nodeDueqe.end() - 3, curNode);
                continue;
            }
        }

        nodeDueqe.emplace_back(curNode);
    }

    this->_root = curNode;

    if (this->_root->leftNode != nullptr)
        this->ConstructionPath(this->_root->leftNode, this->_root->path, 0);
    if (this->_root->rightNode != nullptr)
        this->ConstructionPath(this->_root->rightNode, this->_root->path, 1);

    std::vector<uint8_t> encodedBuffer;

    encodedBuffer.reserve(bufferSize);

    this->ParseHuffmanTreeToBitStream(encodedBuffer,this->_root);

    for(uint32_t i =0;i<bufferSize;i++)
    {
        uint8_t c = buffer[i];

        auto node = nodeMap[c];

        for(const auto& val : node->path) this->SetBits(encodedBuffer,1,val);
    }

    if(this->_bitCount) encodedBuffer.emplace_back(this->_curValue);

    return encodedBuffer;
}