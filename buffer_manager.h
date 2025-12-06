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

struct FileInfo{
    DiskManager* diskManager;
    LRUlist& queue;
    CacheMap& cache;
    int capacity;
};

class BufferManager{
    vector<DiskManager*> diskManagers;
    vector<LRUlist> queues;
    vector<CacheMap> pageCaches;
    unordered_map<string, int> files;
    vector<int> capacities;

    void removeLastElement(int index){
        auto [diskManager, queue, pageCache, capacity] = getFileInfo(index);

        Page p = queue.back();
        Item& item = pageCache[p];
        if(item.dirty){
            diskManager->writePage(p, item.data);
        }
        queue.pop_back();
        pageCache.erase(p);
    }

    Item& promoteAngGetItem(int index, Page page){
        auto& pageCache = pageCaches[index];
        auto& queue = queues[index];
    
        Item& item = pageCache[page];
        queue.splice(queue.begin(), queue, item.it);

        return item;
    }

    FileInfo getFileInfo(int index){
        return FileInfo{
            diskManagers[index],
            queues[index],
            pageCaches[index],
            capacities[index]
        };
    }

public:
    BufferManager(){

    }

    void addFile(string filename, DiskManager *diskManager, int capacity){
        files[filename] = (int)files.size();
        diskManagers.push_back(diskManager);
        queues.push_back(list<Page>());
        pageCaches.push_back(CacheMap());
        capacities.push_back(capacity);
    }

    void writePage(string filename, Page page, Data &data){
        int index = files[filename];
        auto [diskManager, queue, pageCache, capacity] = getFileInfo(index);

        if(pageCache.find(page) != pageCache.end()){
            Item& item = promoteAngGetItem(index, page);
            item.data = data;
            item.dirty = true;
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement(index);
            }
            Item newItem;
            newItem.data = data;
            newItem.dirty = true;

            queue.push_front(page);
            newItem.it = queue.begin();
            pageCache[page] = move(newItem); 
        }
    }
    
    Page writePage(string filename, Data &data){
        int index = files[filename];
        DiskManager* diskManager = diskManagers[index];
        Page page = diskManager->allocatePage();
        writePage(filename, page, data);
        return page;
    }

    Data readPage(string filename, Page page){
        int index = files[filename];
        auto [diskManager, queue, pageCache, capacity] = getFileInfo(index);

        if(pageCache.find(page) != pageCache.end()){
            Item& item = promoteAngGetItem(index, page);
            return item.data;
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement(index);
            }
            Item newItem;
            newItem.data = diskManager->readPage(page);
           
            queue.push_front(page);
            newItem.it = queue.begin();
            pageCache[page] = move(newItem); 
            
            return pageCache[page].data;
        }
    }
    
    void removePage(string filename, Page page){
        int index = files[filename];
        auto [diskManager, queue, pageCache, capacity] = getFileInfo(index);

        if(pageCache.find(page) != pageCache.end()){
            queue.remove(page);
            pageCache.erase(page);
        }
        diskManager->removePage(page);
    }

    void modifyPage(Item &item, int offset, Data &data){
        memcpy(item.data.data() + offset, data.data(), data.size());
        item.dirty = true;
    }

    void writeRecord(string filename, Address address, Data &data){
        int index = files[filename];
        auto [diskManager, queue, pageCache, capacity] = getFileInfo(index);
        auto [page, offset] = address;

        if(pageCache.find(page) != pageCache.end()){
            Item& item = promoteAngGetItem(index, page);
            modifyPage(item, offset, data);
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement(index);
            }

            Item newItem;
            newItem.data = diskManager->readPage(page);
            modifyPage(newItem, offset, data);

            queue.push_front(page);
            newItem.it = queue.begin();
            pageCache[page] = move(newItem); 
        }
    }

    Address writeRecord(string filename, Data &data){
        int index = files[filename];
        DiskManager* diskManager = diskManagers[index];

        optional<Address> address = diskManager->getEmptyPosition();
        if(address != nullopt){
            writeRecord(filename, *address, data);
            return *address;
        }else{
            size_t pageSize = diskManager->getPageSize();
            Data tombstone(pageSize, 0);
            memcpy(tombstone.data(), data.data(), data.size());
            Page page = writePage(filename, tombstone);

            for(int i = data.size(); i < pageSize; i += data.size()){
                Address temp = {page, i};
                diskManager->addFreeSlot(temp);
            }

            return {page, 0};
        }
    }

    Data readRecord(string filename, Address address, size_t recordSize){
        Data temp = readPage(filename, address.page);
        Data data(recordSize);
        memcpy(data.data(), temp.data() + address.offset, data.size());
        return data;
    }

    void removeRecord(string filename, Address address){
        int index = files[filename];
        DiskManager* diskManager = diskManagers[index];

        diskManager->addFreeSlot(address);
    }

};