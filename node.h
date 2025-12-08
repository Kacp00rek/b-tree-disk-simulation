#pragma once
#include <string>
#include "types.h"
#include <algorithm>

using namespace std;

struct NodeEntry{
    Key key;
    Address address;

    static const size_t size = sizeof(key) + sizeof(address.page) + sizeof(address.offset);

    void serialize(Data &data, size_t &offset){
        memcpy(data.data() + offset, &key, sizeof(key));
        offset += sizeof(key);

        memcpy(data.data() + offset, &address.page, sizeof(address.page));
        offset += sizeof(address.page);

        memcpy(data.data() + offset, &address.offset, sizeof(address.offset));
        offset += sizeof(address.offset);
    }

    static NodeEntry deserialize(const Data &data, size_t &offset) {
        NodeEntry entry;
        
        memcpy(&entry.key, data.data() + offset, sizeof(entry.key));
        offset += sizeof(entry.key);
        
        memcpy(&entry.address.page, data.data() + offset, sizeof(entry.address.page));
        offset += sizeof(entry.address.page);
        
        memcpy(&entry.address.offset, data.data() + offset, sizeof(entry.address.offset));
        offset += sizeof(entry.address.offset);

        return entry;
    }

    bool operator<(const NodeEntry& other) const{
        return key < other.key;
    }
};

struct Node{

    bool leaf = false;
    Page parent = NULL_PAGE;
    vector<NodeEntry> entries;
    vector<Page> children;

    static const size_t size = sizeof(leaf) + sizeof(parent) + sizeof(int) + 2 * D * NodeEntry::size + (2 * D + 1) * sizeof(Page);

    int searchPlace(const NodeEntry &entry){
        return upper_bound(entries.begin(), entries.end(), entry) - entries.begin();
    }

    void pop_front(){
        entries.erase(entries.begin());
        if(!leaf){
            children.erase(children.begin()); 
        }
    }

    void push_front(const NodeEntry &entry){
        entries.insert(entries.begin(), entry);
    }

    void push_front(Page child){
        children.insert(children.begin(), child);
    }

    void pop_back(){
        entries.pop_back();
        if(!leaf){
            children.pop_back();
        }
    }

    int addKey(const NodeEntry &entry){
        int index = searchPlace(entry);
        entries.insert(entries.begin() + index, entry);
        return index;
    }

    void addKey(const NodeEntry &entry, Page child){
        int index = addKey(entry);
        children.insert(children.begin() + index + 1, child);
    }

    Data serialize(){
        Data data(size, 0);
        size_t offset = 0;
        
        memcpy(data.data() + offset, &parent, sizeof(parent));
        offset += sizeof(parent);

        memcpy(data.data() + offset, &leaf, sizeof(leaf));
        offset += sizeof(leaf);

        int count = (int)entries.size(); 
        memcpy(data.data() + offset, &count, sizeof(count));
        offset += sizeof(count);

        size_t temp = offset;

        for(int i = 0; i < entries.size(); i++){
            entries[i].serialize(data, offset); 
        }
        
        offset = temp + (2 * D) * NodeEntry::size;        
        for(int i = 0; i < children.size(); i++){
            memcpy(data.data() + offset, &children[i], sizeof(children[i]));
            offset += sizeof(children[i]);
        }
        
        return data;
    }

    static Node deserialize(const Data &data){
        Node node;
        size_t offset = 0;

        memcpy(&node.parent, data.data() + offset, sizeof(node.parent));
        offset += sizeof(node.parent);

        memcpy(&node.leaf, data.data() + offset, sizeof(node.leaf));
        offset += sizeof(node.leaf);

        int count;
        memcpy(&count, data.data() + offset, sizeof(count));
        offset += sizeof(count);

        size_t temp = offset;
        for(int i = 0; i < count; i++){
            NodeEntry entry = NodeEntry::deserialize(data, offset);
            node.entries.push_back(entry);
        }
        
        offset = temp + (2 * D) * NodeEntry::size;

        if(!node.leaf){
            for(int i = 0; i < count + 1; i++){
                Page child;
                memcpy(&child, data.data() + offset, sizeof(child));
                offset += sizeof(child);
                node.children.push_back(child);
            }
        }

        return node;
    }

};