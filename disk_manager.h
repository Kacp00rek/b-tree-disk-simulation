#pragma once
#include <fstream>
#include <unordered_set>
#include <iostream>
#include <optional>
#include "types.h"

using namespace std;

class DiskManager{
    fstream file;
    size_t pageSize;
    unordered_set<Page> emptyPages;
    unordered_set<Address> emptyPositions;
    Page pages;

public:
    static int READS;
    static int WRITES;
    DiskManager(){
        
    }
    DiskManager(string &filename, size_t pageSize){
        file.open(filename, ios::out | ios::trunc);
        file.close();
        file.open(filename, ios::in | ios::out | ios::binary);
        if(!file.is_open()){
            cerr << "Filed to open file " << filename << "\n";
            return;
        }
        this->pageSize = pageSize;
        file.seekg(0, ios::end);
        pages = 0;
    }

    void writePage(Page page, Data &data){
        if(page < 0 || page >= pages){
            throw std::out_of_range("DiskManager::writePage: Invalid page number");
        }
        if(data.size() != pageSize){
            throw std::invalid_argument("DiskManager::writePage: Invalid data size");
        }
        WRITES++;
        file.seekp(page * pageSize);
        file.write(reinterpret_cast<char*>(data.data()), pageSize);
    }

    Page allocatePage(){
        Page page;
        if(!emptyPages.empty()){
            page = *emptyPages.begin();
            emptyPages.erase(page);
        }
        else{
            page = pages++;
        }
        return page;
    }

    void removePage(Page page){
        if(page < 0 || page >= pages){
            throw std::out_of_range("DiskManager::writePage: Invalid page number");
            return;
        }
        emptyPages.insert(page);
    }

    void markEmpty(Address &address){
        emptyPositions.insert(address);
    }

    optional<Address> getEmptyPosition(){
        if(emptyPositions.empty()){
            return nullopt;
        }
        Address address = *emptyPositions.begin();
        emptyPositions.erase(address);

        return address;
    }

    void addFreeSlot(Address &address){
        emptyPositions.insert(address);
    }

    Data readPage(Page page){
        if(page < 0 || page >= pages){
            throw std::out_of_range("DiskManager::readPage: Invalid page number");
        }
        if(emptyPages.find(page) != emptyPages.end()){
            throw std::runtime_error("DiskManager::readPage: Attempted to read en empty page");
        }
        READS++;
        Data data(pageSize);

        file.seekg(page * pageSize);
        file.read(reinterpret_cast<char*>(data.data()), pageSize);

        return data;
    }

    size_t getPageSize(){
        return pageSize;
    }

    ~DiskManager(){
        file.close();
    }

};
int DiskManager::READS = 0;
int DiskManager::WRITES = 0;