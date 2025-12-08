#pragma once
#include "node.h"
#include "record.h"
#include "buffer_manager.h"

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
        bufferNodes.writePage(childPageID, childNode.serialize());
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

    void leftRotate(Node &parent, Node &leftChild, Node &node, int parentIndex, Page leftPage){
        leftChild.entries.push_back(parent.entries[parentIndex]);
        if(!node.leaf){
            leftChild.children.push_back(node.children[0]);
            updateChildParent(node.children[0], leftPage);
        }
        parent.entries[parentIndex] = node.entries[0];
        node.pop_front();
    }

    void rightRotate(Node &parent, Node &rightChild, Node &node, int parentIndex, Page rightPage){
        rightChild.push_front(parent.entries[parentIndex]);
        if(!node.leaf){
            rightChild.push_front(node.children[node.children.size() - 1]);
            updateChildParent(node.children[node.children.size() - 1], rightPage);
        } 
        parent.entries[parentIndex] = node.entries[node.entries.size() - 1];
        node.pop_back();
    }

    void performCompensation(
        Node &node, Page page,
        Node &sibling, Page siblingPage, int siblingIndex,
        Node &parent, int parentIndex, bool isLeft
    ){
        int rotations = (2 * D + 1 - sibling.entries.size()) / 2;
        for(int i = 0; i < rotations; i++){
            if(isLeft){
                leftRotate(parent, sibling, node, parentIndex, siblingPage);
            }
            else{
                rightRotate(parent, sibling, node, parentIndex, siblingPage);
            }
        }
        bufferNodes.writePage(siblingPage, sibling.serialize());
        bufferNodes.writePage(node.parent, parent.serialize());
        bufferNodes.writePage(page, node.serialize());
        }

    bool compensation(Node &node, Page page){
        if(node.parent == NULL_PAGE){
            return false;
        }
        Node parent = Node::deserialize(bufferNodes.readPage(node.parent));
        int childIndex = parent.searchPlace(node.entries[0]);

        if(childIndex > 0){
            int leftIndex = childIndex - 1;
            Page leftSiblingPage = parent.children[leftIndex];
            Node leftSibling = Node::deserialize(bufferNodes.readPage(leftSiblingPage));
            if(leftSibling.entries.size() < 2 * D){
                performCompensation(node, page, leftSibling, leftSiblingPage, leftIndex, parent, leftIndex, true);
                return true;
            }
        }
        if(childIndex < parent.children.size() - 1){
            int rightIndex = childIndex + 1;
            Page rightSibingPage = parent.children[rightIndex];
            Node rightSibling = Node::deserialize(bufferNodes.readPage(rightSibingPage));
            if(rightSibling.entries.size() < 2 * D){
                performCompensation(node, page, rightSibling, rightSibingPage, rightIndex, parent, childIndex, false);
                return true;
            }
        }
        return false;
    }

    Node split(Node &node, Page page){ 
        Node sibling;
        sibling.leaf = node.leaf;

        int median = D;
        int maxEntries = 2 * D + 1;

        for(int i = median + 1; i < maxEntries; i++){
            sibling.entries.push_back(node.entries[i]);
        }
        if(!node.leaf){
            for(int i = median + 1; i < maxEntries + 1; i++){
                sibling.children.push_back(node.children[i]);
            }
        }
        Node parent;
        if(node.parent == NULL_PAGE){
            root = bufferNodes.writePage(parent.serialize());
            node.parent = root;
            parent.children.push_back(page);
        }
        else{
            parent = Node::deserialize(bufferNodes.readPage(node.parent));
        }
        sibling.parent = node.parent;
        Page newPage = bufferNodes.writePage(sibling.serialize());
        for(int i = 0; i < sibling.children.size(); i++){
            updateChildParent(sibling.children[i], newPage);
        }

        parent.addKey(node.entries[median], newPage);

        for(int i = 0; i < median + 1; i++){
            node.pop_back();
        }
        bufferNodes.writePage(page, node.serialize());

        return parent;
    }

    void print(ofstream &file, Page page, int &id){
        Node node = Node::deserialize(bufferNodes.readPage(page));
        
        file << "node" << id << "[label =\"<f0> ";
        for(int i = 0; i < node.entries.size(); i++){
            file << "|" << node.entries[i].key << "| <f" << i + 1 <<"> ";
        }
        file << "\"];\n";

        if(!node.leaf){
        int currentId = id;
            for(int i = 0; i < node.children.size(); i++){
                file << "\"node" << currentId << "\":f" << i <<" -> \"node" << ++id << "\"\n";
                print(file, node.children[i], id);
            }
        }
    }

    void generateDot(string &filename){
        ofstream file(filename);

        file << "digraph g {\n";
        file << "node [shape = record,height=.1];\n";

        if(root != NULL_PAGE){
            int id = 0;
            print(file, root, id);
        }

        file << "}";

        file.close();
    }

public:
    public:
    BTree() : 
        filenameNodes("nodes.txt"), 
        filenameMain("records.txt"),
        
        diskNodes(filenameNodes, Node::size),
        diskMain(filenameMain, T::size * BLOCKING_FACTOR),
        
        bufferNodes(&diskNodes, 16),
        bufferRecords(&diskMain, 16)
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
        Address address = *result;
        return T::deserialize(bufferRecords.readRecord(address, T::size));           
    }

    STATUS modify(T &record){
        if(root == NULL_PAGE){
            return DOESNT_EXIST;
        }
        auto result = search(record.key, root);
        if(result == nullopt){
            return DOESNT_EXIST;
        }

        Address address = *result;

        bufferRecords.writeRecord(address, record.serialize());

        return OK;
    }

    NodeEntry saveRecord(T &record){
        Address address = bufferRecords.writeRecord(record.serialize());
        return {record.key, address};
    }

    STATUS insert(T &record){
        if(root == NULL_PAGE){
            NodeEntry entry = saveRecord(record);
            Node node = Node();
            node.leaf = true;
            node.entries.push_back(entry);
            root = bufferNodes.writePage(node.serialize());
            return OK;
        }
        auto [status, currentPage] = searchPlace(record.key, root);

        if(status == ALREADY_EXISTS){
            return status;
        }
        NodeEntry entry = saveRecord(record);
        Node node = Node::deserialize(bufferNodes.readPage(currentPage));
        node.addKey(entry);

        while(true){
            if(node.entries.size() <= 2 * D){
                bufferNodes.writePage(currentPage, node.serialize());
                break;
            }
            if(compensation(node, currentPage)){
                break;
            }
            Node parent = split(node, currentPage);
            currentPage = node.parent;
            node = parent;
        }
        return OK;
    }   

    void visualize(){
        string filename = "btree.dot";
        generateDot(filename);

        string png = filename;
        size_t dotPos = png.rfind(".dot");

        if(dotPos != string::npos){
            png.replace(dotPos, 4, ".png");
        }

        string command = "dot -Tpng " + filename + " -o " + png;
        system(command.c_str());
    }
};