#pragma once
#include <string>
#include "types.h"

using namespace std;

struct Node{

    bool leaf;
    int count;
    
    Key keys[2 * D];
    Address records[2 * D];
    Page children[2 * D + 1];

    static const size_t size = sizeof(leaf) + sizeof(count) + sizeof(keys) + sizeof(records) + sizeof(children);

    Node(){
        leaf = false;
        count = 0;
    }

    vector<uint8_t> serialize(){
        vector<uint8_t> data(size);
        size_t curr = 0;

        memcpy(data.data() + curr, &leaf, sizeof(leaf));
        curr += sizeof(leaf);

        memcpy(data.data() + curr, &count, sizeof(count));
        curr += sizeof(count);

        memcpy(data.data() + curr, keys, sizeof(keys));
        curr += sizeof(keys);

        memcpy(data.data() + curr, records, sizeof(records));
        curr += sizeof(records);

        memcpy(data.data() + curr, children, sizeof(children));

        return data;
    }
    
    static Node deserialize(vector<uint8_t> &data){
        Node node;
        size_t curr = 0;

        memcpy(&node.leaf, data.data() + curr, sizeof(node.leaf));
        curr += sizeof(node.leaf);

        memcpy(&node.count, data.data() + curr, sizeof(node.count));
        curr += sizeof(node.count);

        memcpy(node.keys, data.data() + curr, sizeof(node.keys));
        curr += sizeof(node.keys);

        memcpy(node.records, data.data() + curr, sizeof(node.records));
        curr += sizeof(node.records);

        memcpy(node.children, data.data() + curr, sizeof(node.children));

        return node;
    }
    
};