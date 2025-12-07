#pragma once
#include "node.h"
#include "record.h"
#include "buffer_manager.h"

enum STATUS { OK, ALREADY_EXISTS };

struct SearchResult{
    STATUS status;
    Page page;
};

template <typename T>
class BTree{
    
    Page root;
    string filenameNodes;
    string filenameMain;
    DiskManager diskNodes;
    DiskManager diskMain;
    BufferManager bufferNodes;
    BufferManager bufferRecords;

    void updateChildParent(Page childPageID, Page newParentID) {
        Node childNode = Node::deserialize(bufferNodes.readPage(childPageID));
        childNode.parent = newParentID;
        Data data = childNode.serialize();
        bufferNodes.writePage(childPageID, data);
    }

    optional<Address> search(Key key, Page page){
        Node node = Node::deserialize(bufferNodes.readPage(page));
        int index = node.searchPlace({key, {0, 0}});
        if(index > 0 && node.entries[index - 1].key == key){
            return node.entries[index - 1].address;
        }
        if(node.leaf){
            return nullopt;
        }
        return search(key, node.children[index]);
    }

    SearchResult searchPlace(Key key, Page page){
        Node node = Node::deserialize(bufferNodes.readPage(page));
        int index = node.searchPlace({key, {0, 0}});

        if(index > 0 && node.entries[index - 1].key == key){
            return {ALREADY_EXISTS, NULL_PAGE};
        }
        if(node.leaf){
            return {OK, page};
        }
        return searchPlace(key, node.children[index]);
    }

    void leftRotate(Node &parent, Node &leftChild, Node &node, int parentIndex){
        leftChild.entries.push_back(parent.entries[parentIndex]);
        if(!node.leaf){
            leftChild.children.push_back(node.children[0]);
        }
        
        parent.entries[parentIndex] = node.entries[0];

        node.pop_front();
    }

    void rightRotate(Node &parent, Node &rightChild, Node &node, int parentIndex){
        rightChild.push_front(parent.entries[parentIndex]);
        if(!node.leaf){
            rightChild.push_front(node.children[node.children.size() - 1]);
        }
        
        parent.entries[parentIndex] = node.entries[node.entries.size() - 1];

        node.pop_back();
    }

    bool compensation(Node &node, Page page){
        if(node.parent == NULL_PAGE){
            return false;
        }
        Node parent = Node::deserialize(bufferNodes.readPage(node.parent));
        int childIndex = parent.searchPlace(node.entries[0]);

        if(childIndex > 0){
            int leftIndex = childIndex - 1;
            Node leftChild = Node::deserialize(bufferNodes.readPage(parent.children[leftIndex]));
            if(leftChild.entries.size() < 2 * D){
                int rotations = (2 * D + 1 - leftChild.entries.size()) / 2;
                for(int i = 0; i < rotations; i++){
                    if(!node.leaf){
                        updateChildParent(node.children[0], parent.children[leftIndex]);
                    }
                    leftRotate(parent, leftChild, node, leftIndex);
                }
                Data data = leftChild.serialize();
                bufferNodes.writePage(parent.children[leftIndex], data);
                data = parent.serialize();
                bufferNodes.writePage(node.parent, data);
                data = node.serialize();
                bufferNodes.writePage(page, data);
                return true;
            }
        }
        if(childIndex < parent.children.size() - 1){
            int rightIndex = childIndex + 1;
            Node rightChild = Node::deserialize(bufferNodes.readPage(parent.children[rightIndex]));
            if(rightChild.entries.size() < 2 * D){
                int rotations = (2 * D + 1 - rightChild.entries.size()) / 2;
                for(int i = 0; i < rotations; i++){
                    if(!node.leaf){
                        updateChildParent(node.children[node.children.size() - 1], parent.children[rightIndex]);
                    }
                    rightRotate(parent, rightChild, node, childIndex);
                }
                Data data = rightChild.serialize();
                bufferNodes.writePage(parent.children[rightIndex], data);
                data = parent.serialize();
                bufferNodes.writePage(node.parent, data);
                data = node.serialize();
                bufferNodes.writePage(page, data);
                return true;
            }
        }
        return false;
    }

    Node split(Node &node, Page page){ 
        Node sibling;
        sibling.leaf = node.leaf;

        for(int i = D + 1; i < 2 * D + 1; i++){
            sibling.entries.push_back(node.entries[i]);
        }
        if(!node.leaf){
            for(int i = D + 1; i < 2 * D + 2; i++){
                sibling.children.push_back(node.children[i]);
            }
        }
        Node parent;
        if(node.parent == NULL_PAGE){
            Data data = parent.serialize();
            root = bufferNodes.writePage(data);
            node.parent = root;
            parent.children.push_back(page);
        }
        else{
            parent = Node::deserialize(bufferNodes.readPage(node.parent));
        }
        sibling.parent = node.parent;
        Data data = sibling.serialize();
        Page newPage = bufferNodes.writePage(data);
        for(int i = 0; i < sibling.children.size(); i++){
            updateChildParent(sibling.children[i], newPage);
        }

        parent.addKey(node.entries[D], newPage);

        for(int i = 0; i < D + 1; i++){
            node.pop_back();
        }
        data = node.serialize();
        bufferNodes.writePage(page, data);

        return parent;
    }

public:
    public:
    BTree() : 
        filenameNodes("nodes.txt"), 
        filenameMain("records.txt"),
        
        diskNodes(filenameNodes, Node::size),
        diskMain(filenameMain, T::size * BLOCKING_FACTOR),
        
        bufferNodes(&diskNodes, 5),
        bufferRecords(&diskMain, 5)
    {
        root = NULL_PAGE;
    }

    optional<T> search(Key key){
        if(root == NULL_PAGE){
            return nullopt;
        }
        auto result = search(key, root);
        if(result == nullopt){
            return nullopt;
        }
        return T::deserialize(bufferRecords.readRecord(*result));           
    }

    NodeEntry saveRecord(T &record){
        Data data = record.serialize();
        Address address = bufferRecords.writeRecord(data);
        return {record.key, address};
    }

    STATUS insert(T record){
        if(root == NULL_PAGE){
            NodeEntry entry = saveRecord(record);
            Node node = Node();
            node.leaf = true;
            node.entries.push_back(entry);
            Data data = node.serialize();
            root = bufferNodes.writePage(data);
            return OK;
        }
        auto [status, currentPage] = searchPlace(record.key, root);

        if(status == ALREADY_EXISTS){
            return status;
        }
        NodeEntry entry = saveRecord(record);
        Node node = Node::deserialize(bufferNodes.readPage(currentPage));
        node.addKey(entry);
        if(node.entries.size() <= 2 * D){
            Data data = node.serialize();
            bufferNodes.writePage(currentPage, data);
            return OK;
        }

        while(node.entries.size() > 2 * D){
            if(compensation(node, currentPage)){
                break;
            }
            Node parent = split(node, currentPage);
            if(parent.entries.size() <= 2 * D){
                Data data = parent.serialize();
                bufferNodes.writePage(node.parent, data);
            }
            currentPage = node.parent;
            node = parent;
        }
        return OK;
    }   

    void print(){
        if(root != NULL_PAGE){
            print(root, 0);
        }
    }

    void print(Page page, int depth){
        Node node = Node::deserialize(bufferNodes.readPage(page));

        for(int i = 0; i < node.entries.size(); i++){
            if(!node.leaf){
                print(node.children[i], depth + 1);
            }
            // cout << "KEY: " << node.entries[i].key << "\n";
            // cout << "PAGE: " << page << "\n";
            // cout << "PARENT PAGE " << node.parent << "\n";
            // cout << "DEPTH: " << depth << "\n\n";
            cout << node.entries[i].key << " ";
        }
        if(!node.leaf){
            print(node.children[node.entries.size()], depth + 1);
        }
    }


};