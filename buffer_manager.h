#pragma once
#include <vector>
#include <list>
#include <string>
#include <unordered_map>
#include "disk_manager.h"
using namespace std;
using Data = vector<uint8_t>;
using Page = int;

struct Item{
    Data data;
    bool dirty = false;
    list<Page>::iterator it;
};

class BufferManager{
    vector<DiskManager*> diskManagers;
    vector<list<Page>> queues;
    vector<unordered_map<Page, Item>> pageCaches;
    unordered_map<string, int> files;
    vector<int> capacities;


    void removeLastElement(DiskManager* diskManager, unordered_map<Page, Item> &pageCache, list<Page> &queue){
        Page p = queue.back();
        Item& item = pageCache[p];
        if(item.dirty){
            diskManager->writePage(p, item.data);
        }
        queue.pop_back();
        pageCache.erase(p);
    }

public:
    BufferManager(){

    }

    void addFile(string filename, DiskManager *diskManager, int capacity){
        files[filename] = (int)files.size();
        diskManagers.push_back(diskManager);
        queues.push_back(list<Page>());
        pageCaches.push_back(unordered_map<Page, Item>());
        capacities.push_back(capacity);
    }

    void writePage(string filename, Page page, Data &data){
        int index = files[filename];
        DiskManager* diskManager = diskManagers[index];
        list<Page>& queue = queues[index];
        unordered_map<Page, Item>& pageCache = pageCaches[index];
        int capacity = capacities[index];

        if(pageCache.find(page) != pageCache.end()){
            Item& item = pageCache[page];
            item.data = data;
            item.dirty = true;
            queue.splice(queue.begin(), queue, item.it);
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement(diskManager, pageCache, queue);
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
        DiskManager* diskManager = diskManagers[index];
        list<Page>& queue = queues[index];
        unordered_map<Page, Item>& pageCache = pageCaches[index];
        int capacity = capacities[index];

        if(pageCache.find(page) != pageCache.end()){
            Item& item = pageCache[page];
            Data data = item.data;
            queue.splice(queue.begin(), queue, item.it);
            return data;
        }
        else{
            if((int)queue.size() >= capacity){
                removeLastElement(diskManager, pageCache, queue);
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
        DiskManager* diskManager = diskManagers[index];
        list<Page>& queue = queues[index];
        unordered_map<Page, Item>& pageCache = pageCaches[index];

        if(pageCache.find(page) != pageCache.end()){
            queue.remove(page);
            pageCache.erase(page);
        }
        diskManager->removePage(page);
    }

};