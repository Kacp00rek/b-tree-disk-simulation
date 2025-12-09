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
            return {ALREADY_EXISTS, page};
        }
        if(node.leaf){
            return {DOESNT_EXIST, page};
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
        int rotations = abs((int)(node.entries.size() - sibling.entries.size())) / 2;
        // cout << rotations << "\n";
        // cout << "DIRECTION LEFT: " << isLeft << "\n";
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

    bool compensationInsert(Node &node, Page page){
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

    bool compensationRemove(Node &node, Page page){
        if(node.parent == NULL_PAGE){
            return false;
        }
        Node parent = Node::deserialize(bufferNodes.readPage(node.parent));
        int childIndex = parent.searchChild(page);
       //  cout << "CHECKPOINT1";
        if(childIndex > 0){
            int leftIndex = childIndex - 1;
            Page leftSiblingPage = parent.children[leftIndex];
            Node leftSibling = Node::deserialize(bufferNodes.readPage(leftSiblingPage));
            // cout << "CHECKPOINT2";
            if(leftSibling.entries.size() > D){
                // cout << "COMPENSATION LEFT\n";
                performCompensation(leftSibling, leftSiblingPage, node, page, childIndex, parent, leftIndex, false);
                return true;
            }
        }
        if(childIndex < parent.children.size() - 1){
            int rightIndex = childIndex + 1;
            Page rightSibingPage = parent.children[rightIndex];
            Node rightSibling = Node::deserialize(bufferNodes.readPage(rightSibingPage));
            // cout << "CHECKPOINT3";
            if(rightSibling.entries.size() > D){
                // cout << "COMPENSATION RIGHT\n";
                performCompensation(rightSibling, rightSibingPage, node, page, childIndex, parent, childIndex, true);
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

        if(!node.leaf){
            for(int i = 0; i < sibling.children.size(); i++){
                updateChildParent(sibling.children[i], newPage);
            }
        }

        parent.addKey(node.entries[median], newPage);

        for(int i = 0; i < median + 1; i++){
            node.pop_back();
        }
        bufferNodes.writePage(page, node.serialize());

        return parent;
    }

    Node merge(Node &node, Page page){
        // cout << "MERGE\n";
        Node parent = Node::deserialize(bufferNodes.readPage(node.parent));
        int index = parent.searchChild(page);
        int parentEntry;
        Page siblingPage;
        Node sibling;
        if(index > 0){
            siblingPage = parent.children[index - 1];
            sibling = Node::deserialize(bufferNodes.readPage(siblingPage));
            parentEntry = index - 1;
        }
        else{
            siblingPage = parent.children[index + 1];
            sibling = Node::deserialize(bufferNodes.readPage(siblingPage));
            swap(node, sibling);
            swap(page, siblingPage);
            parentEntry = index;
        }
        
        sibling.entries.push_back(parent.entries[parentEntry]);

        for(int i = 0; i < (int)node.entries.size(); i++){
            sibling.entries.push_back(node.entries[i]);
        }

        if(!node.leaf){
            for(int i = 0; i < (int)node.children.size(); i++){
                sibling.children.push_back(node.children[i]);
                updateChildParent(node.children[i], siblingPage);
            }
        }
        parent.removeKey(parent.entries[parentEntry].key);

        bufferNodes.removePage(page);
        bufferNodes.writePage(siblingPage, sibling.serialize());

        return move(parent);
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

    Page findSuccessor(Page page){
        Node node = Node::deserialize(bufferNodes.readPage(page));
        if(node.leaf){
            return page;
        }
        return findSuccessor(node.children[0]);
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
        return T::deserialize(bufferRecords.readRecord(*result, T::size));           
    }

    STATUS modify(T &record){
        if(root == NULL_PAGE){
            return DOESNT_EXIST;
        }
        auto result = search(record.key, root);
        if(result == nullopt){
            return DOESNT_EXIST;
        }

        bufferRecords.writeRecord(*result, record.serialize());

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
            if(compensationInsert(node, currentPage)){
                break;
            }
            Node parent = split(node, currentPage);
            currentPage = node.parent;
            node = parent;
        }
        return OK;
    }   

    STATUS remove(Key key){
        if(root == NULL_PAGE){
            return DOESNT_EXIST;
        }
        auto [status, currentPage] = searchPlace(key, root);

        if(status == DOESNT_EXIST){
            return status;
        }

        // cout << currentPage << "\n";

        Node node = Node::deserialize(bufferNodes.readPage(currentPage));
        int index = node.searchPlace({key, {0, 0}});

        bufferRecords.removeRecord(node.entries[index - 1].address);
        if(node.leaf){
            node.removeKey(key);
        }
        else{
            Page successorPage = findSuccessor(node.children[index]);
            Node successor = Node::deserialize(bufferNodes.readPage(successorPage));
            node.entries[index - 1] = successor.entries[0];
            successor.pop_front();

            bufferNodes.writePage(currentPage, node.serialize());
            currentPage = successorPage;
            node = move(successor);
        }


        while(true){
            if((int)node.entries.size() >= D){
                bufferNodes.writePage(currentPage, node.serialize());
                break;
            }
            if(currentPage == root){
                if((int)node.entries.size() < 1){
                    bufferNodes.removePage(currentPage);
                    if(!node.leaf){
                        root = node.children[0];
                        node = Node::deserialize(bufferNodes.readPage(root));
                        node.parent = NULL_PAGE;
                        currentPage = root;
                    }
                    else{
                        root = NULL_PAGE;
                        break;
                    }
                }
                bufferNodes.writePage(currentPage, node.serialize());
                break;
            }
            if(compensationRemove(node, currentPage)){
                break;
            }
            Node parent = merge(node, currentPage);
            if((int)parent.entries.size() >= D){
                bufferNodes.writePage(node.parent, parent.serialize());
                break;
            }
            currentPage = node.parent;
            node = move(parent);
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