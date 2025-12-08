#include <iostream>
#include <random>
#include <windows.h>
#include "btree.h"
#include "record.h"

using namespace std;
using RecordType = Record;

unordered_map<Key, RecordType> hashmap;


void insert(BTree<RecordType> &btree, int &id){
    RecordType record = RecordType::random(id++);
    STATUS status = btree.insert(record);
    hashmap[record.key] = record;

    // cout << "INSERTED: " << record << "\n";
}

bool search(BTree<RecordType> &btree, int id){
    int size = hashmap.size();

    bool verdict;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> k(0, (id - 1) * 2);

    Key key = k(gen);

    // cout << "SEARCH: " << key << "\n";

    optional<RecordType> record = btree.search(key);
    
    if(hashmap.find(key) != hashmap.end()){
        // cout << "EXPECTED: " << hashmap[key] << "\n";
        // cout << "GOT:      ";
        if(record == nullopt){
            // cout << "NULL" << "\n";
            verdict = false;
        }
        else{
            // cout << *record << "\n";
            verdict = record == hashmap[key];
        }
    }
    else{
        // cout << "EXPECTED: NULL\n";
        // cout << "GOT:      ";
        if(record == nullopt){
            // cout << "NULL" << "\n";
            verdict = true;
        }
        else{
            // cout << *record << "\n";
            verdict = false;
        }
    }
    // cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

int main(){

    srand(time(NULL));

    int n = 1000000, id = 0;
    int tests = 0, testsPassed = 0;
    BTree<RecordType> btree;

    insert(btree, id);


    for(int i = 0; i < n - 1; i++){
        // cout << "\n";
        int option = rand() % 2;

        if(option == 0){
            insert(btree, id);
            // btree.visualize();
            // Sleep(1000);
        }
        else{
            bool ok = search(btree, id);
            tests++;
            if(ok){
                testsPassed++;
            }
        }
    }

    cout << "\n";
    cout << testsPassed << " / " << tests;

    return 0;
}