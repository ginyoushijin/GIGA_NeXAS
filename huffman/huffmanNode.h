#ifndef NEXAS_HUFFMAN_NODE_H
#define NEXAS_HUFFMAN_NODE_H

#include <vector>
#include <stdint.h>
#include <memory>

struct HuffmanNode
{
    uint8_t symbol = NULL;                                    // 当前节点的值
    uint32_t weight = 0;                                      // 当前节点的权重
    std::vector<bool> path;                                // 节点在树中的路径，即前缀码
    std::shared_ptr<HuffmanNode> leftNode = nullptr, rightNode = nullptr; // 该节点的左右子节点
};


#endif // NEXAS_HUFFMAN_NODE_H