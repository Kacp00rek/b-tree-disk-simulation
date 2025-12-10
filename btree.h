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

    bool compensation(Node &node, Page page, bool insert){
        if(node.parent == NULL_PAGE){
            return false;
        }
        Node parent = Node::deserialize(bufferNodes.readPage(node.parent));
        int childIndex = parent.searchChild(page);

        if(childIndex > 0){
            int leftIndex = childIndex - 1;
            Page leftSiblingPage = parent.children[leftIndex];
            Node leftSibling = Node::deserialize(bufferNodes.readPage(leftSiblingPage));
            if(insert){
                if(leftSibling.entries.size() < 2 * D){
                    performCompensation(node, page, leftSibling, leftSiblingPage, leftIndex, parent, leftIndex, true);
                    return true;
                }
            }
            else{
                if(leftSibling.entries.size() > D){
                    performCompensation(leftSibling, leftSiblingPage, node, page, childIndex, parent, leftIndex, false);
                    return true;
                }
            }
        }
        if(childIndex < parent.children.size() - 1){
            int rightIndex = childIndex + 1;
            Page rightSibingPage = parent.children[rightIndex];
            Node rightSibling = Node::deserialize(bufferNodes.readPage(rightSibingPage));
            if(insert){
                if(rightSibling.entries.size() < 2 * D){
                    performCompensation(node, page, rightSibling, rightSibingPage, rightIndex, parent, childIndex, false);
                    return true;
                }
            }
            else{
                if(rightSibling.entries.size() > D){
                    performCompensation(rightSibling, rightSibingPage, node, page, childIndex, parent, childIndex, true);
                    return true;
                }
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

    void getStructureInfo(ofstream &file, Page page, int &id){
        Node node = Node::deserialize(bufferNodes.peekPage(page));
        
        file << "node" << id << " [label=<\n";
        file << "   <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";

        file << "       <TR><TD COLSPAN=\"" << node.entries.size() * 2 + 1 << "\">PAGE: " << page << "</TD></TR>\n";

        file << "       <TR>\n";
        
        for(int i = 0; i < (int)node.entries.size(); i++){
            file << "       <TD PORT=\"f" << i << "\"> </TD>\n";
            file << "       <TD>" << node.entries[i].key << "</TD>\n"; 
        }
        file << "       <TD PORT=\"f" << node.entries.size() << "\"> </TD>\n";
        file << "       </TR>\n";

        file << "   </TABLE>>];\n";

        if(!node.leaf){
            int currentId = id;
            for(int i = 0; i < (int)node.children.size(); i++){
                file << "   \"node" << currentId << "\":f" << i <<" -> \"node" << ++id << "\"\n";
                getStructureInfo(file, node.children[i], id);
            }
        }
    }

    void generateTree(string &filename){
        ofstream file(filename);

        file << "digraph g {\n";
        file << "node [shape = none,height=.1];\n";

        if(root != NULL_PAGE){
            int id = 0;
            getStructureInfo(file, root, id);
        }

        file << "}";

        file.close();
    }

    void generateTable(string &filename){
        ofstream file(filename);

        file << "digraph g {\n";
        file << "node [shape = none,height=.1];\n"; 
        Page pages = diskMain.getSize();
        if(pages > 0){
            file << "DB_VIEW [label=<<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";
            
            size_t recordSize = T::size;

            for(int i = 0; i < pages; i++){
                file << "<TR><TD COLSPAN=\"4\">PAGE: " << i << "</TD></TR>\n";
                file << "<TR><TD>OFFSET</TD>";
                T::getHeader(file);
                file << "</TR>";
                for(int j = 0; j < diskMain.getPageSize(); j += recordSize){
                    file << "<TR><TD>" << j << "</TD>";
                    if(!diskMain.isEmpty({i, j})){
                        T record = T::deserialize(bufferRecords.peekRecord({i, j}));
                        record.getDot(file);
                    }
                    else{
                        file << "<TD>-</TD><TD>-</TD><TD>-</TD>";
                    }
                    file << "</TR>";
                }
            }
            file << "</TABLE>>];\n";
        }
        file << "}";
    }

    Page findSuccessor(Page page){
        Node node = Node::deserialize(bufferNodes.readPage(page));
        if(node.leaf){
            return page;
        }
        return findSuccessor(node.children[0]);
    }

    void printAll(Page page){
        Node node = Node::deserialize(bufferNodes.readPage(page));

        for(int i = 0; i < (int)node.entries.size() + 1; i++){
            if(!node.leaf){
                printAll(node.children[i]);
            }
            if(i < (int)node.entries.size()){
                cout << T::deserialize(bufferRecords.readRecord(node.entries[i].address)) << "\n";
            }
        }

    }

public:
    public:
    BTree() : 
        filenameNodes("nodes.txt"), 
        filenameMain("records.txt"),
        
        diskNodes(filenameNodes, Node::size),
        diskMain(filenameMain, T::size * BLOCKING_FACTOR),
        
        bufferNodes(&diskNodes, 5),
        bufferRecords(&diskMain, 5, T::size)
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
            if(compensation(node, currentPage, true)){
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
            if(compensation(node, currentPage, false)){
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

    void printAll(){
        if(root != NULL_PAGE){
            printAll(root);
        }
    }

    void visualize(){
        string filename1 = "btree.dot";
        string filename2 = "table.dot";
        generateTree(filename1);
        generateTable(filename2);

        string png1 = filename1;
        string png2 = filename2;
        size_t dotPos1 = png1.rfind(".dot");
        size_t dotPos2 = png2.rfind(".dot");

        if(dotPos1 != string::npos){
            png1.replace(dotPos1, 4, ".png");
        }
        if(dotPos2 != string::npos){
            png2.replace(dotPos2, 4, ".png");
        }

        string command = "dot -Tpng " + filename1 + " -o " + png1;
        system(command.c_str());
        command = "dot -Tpng " + filename2 + " -o " + png2;
        system(command.c_str());
    }
};