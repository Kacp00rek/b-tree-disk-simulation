#pragma once
#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include "disk_manager.h"
using namespace std;

using LRUlist = list<Page>;

struct Item{
    Data data;
    bool dirty = false;
    LRUlist::iterator it;
};

using CacheMap = unordered_map<Page, Item>;


class BufferManager{
    DiskManager* diskManager;
    LRUlist queue;
    CacheMap pageCache;
    int capacity;

    void removeLastElement(){
        Page p = queue.back();
        Item& item = pageCache[p];
        if(item.dirty){
            diskManager->writePage(p, item.data);
        }
        queue.pop_back();
        pageCache.erase(p);
    }

    Item& promoteAngGetItem(Page page){
        Item& item = pageCache[page];
        queue.splice(queue.begin(), queue, item.it);

        return item;
    }

public:
    BufferManager(){

    }

    BufferManager(DiskManager *diskManager, int capacity){
        this->diskManager = diskManager;
        this->capacity = capacity;
    }

    void writePage(Page page, const Data &data){
        if(pageCache.find(page) != pageCache.end()){
            Item& item = promoteAngGetItem(page);
            item.data = data;
            item.dirty = true;
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement();
            }
            Item newItem;
            newItem.data = data;
            newItem.dirty = true;

            queue.push_front(page);
            newItem.it = queue.begin();
            pageCache[page] = move(newItem); 
        }
    }
    
    Page writePage(const Data &data){
        Page page = diskManager->allocatePage();
        writePage(page, data);
        return page;
    }

    Data readPage(Page page){
        if(pageCache.find(page) != pageCache.end()){
            Item& item = promoteAngGetItem(page);
            return item.data;
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement();
            }
            Item newItem;
            newItem.data = diskManager->readPage(page);
           
            queue.push_front(page);
            newItem.it = queue.begin();
            pageCache[page] = move(newItem); 
            
            return pageCache[page].data;
        }
    }
    
    void removePage(Page page){
        if(pageCache.find(page) != pageCache.end()){
            queue.remove(page);
            pageCache.erase(page);
        }
        diskManager->removePage(page);
    }

    void modifyPage(Item &item, int offset, const Data &data){
        memcpy(item.data.data() + offset, data.data(), data.size());
        item.dirty = true;
    }

    void writeRecord(Address &address, const Data &data){
        auto [page, offset] = address; 
        if(pageCache.find(page) != pageCache.end()){
            Item& item = promoteAngGetItem(page);
            modifyPage(item, offset, data);
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement();
            }

            Item newItem;
            newItem.data = diskManager->readPage(page);
            modifyPage(newItem, offset, data);

            queue.push_front(page);
            newItem.it = queue.begin();
            pageCache[page] = move(newItem); 
        }
    }

    Address writeRecord(const Data &data){
        optional<Address> address = diskManager->getEmptyPosition();
        if(address != nullopt){
            writeRecord(*address, data);
            return *address;
        }else{
            size_t pageSize = diskManager->getPageSize();
            Data tombstone(pageSize, 0);
            memcpy(tombstone.data(), data.data(), data.size());
            Page page = writePage(tombstone);

            for(int i = data.size(); i < pageSize; i += data.size()){
                Address temp = {page, i};
                diskManager->addFreeSlot(temp);
            }

            return {page, 0};
        }
    }

    Data readRecord(const Address &address, size_t recordSize){
        Data temp = readPage(address.page);
        Data data(recordSize);
        memcpy(data.data(), temp.data() + address.offset, data.size());
        return data;
    }

    void removeRecord(const Address &address){
        diskManager->addFreeSlot(address);
    }

};