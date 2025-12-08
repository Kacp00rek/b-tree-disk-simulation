#include <iostream>
#include <random>
#include <windows.h>
#include "btree.h"
#include "record.h"

using namespace std;
using RecordType = Record;

unordered_map<Key, RecordType> hashmap;


bool insert(BTree<RecordType> &btree, int &id){
    int idToInsert;
    STATUS expected;
    if(id == 0 || rand() % 2 == 0){
        idToInsert = id++;
        expected = OK;
    }
    else{
        idToInsert = rand() % id;
        expected = ALREADY_EXISTS;
    }

    RecordType record = RecordType::random(idToInsert);
    STATUS status = btree.insert(record);

    if(expected == OK){
        hashmap[record.key] = record;
    }

    bool verdict = status == expected;
    cout << "INSERT: " << record << "\n";
    cout << "EXPECTED: " << expected << "\n";
    cout << "GOT       " << status << "\n";
    cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

bool search(BTree<RecordType> &btree, int id){
    bool verdict;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> k(0, (id - 1) * 2);

    Key key = k(gen);

    cout << "SEARCH: " << key << "\n";

    optional<RecordType> record = btree.search(key);
    
    if(hashmap.find(key) != hashmap.end()){
        cout << "EXPECTED: " << hashmap[key] << "\n";
        cout << "GOT:      ";
        if(record == nullopt){
            cout << "NULL" << "\n";
            verdict = false;
        }
        else{
            cout << *record << "\n";
            verdict = record == hashmap[key];
        }
    }
    else{
        cout << "EXPECTED: NULL\n";
        cout << "GOT:      ";
        if(record == nullopt){
            cout << "NULL" << "\n";
            verdict = true;
        }
        else{
            cout << *record << "\n";
            verdict = false;
        }
    }
    cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

bool modify(BTree<RecordType> &btree, int id){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> k(0, (id - 1) * 2);
    Key key = k(gen);
    RecordType record = RecordType::random(key);

    cout << "MODIFY: " << key << " -> " << record << "\n";
    STATUS status = btree.modify(record);
    STATUS expected;

    if(hashmap.find(key) != hashmap.end()){
        expected = OK;
        hashmap[record.key] = record;
    }
    else{
        expected = DOESNT_EXIST;
    }

    bool verdict = status == expected;
    cout << "EXPECTED: " << expected << "\n";
    cout << "GOT       " << status << "\n";
    cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

int main(){

    srand(time(NULL));

    int n = 100000, id = 0;
    int tests = 1, testsPassed = 0;
    BTree<RecordType> btree;

    if(insert(btree, id)){
        testsPassed++;
    }


    for(int i = 0; i < n - 1; i++){
        cout << "\n";
        int option = rand() % 3;

        if(option == 0){
            bool ok = insert(btree, id);
            tests++;
            if(ok){
                testsPassed++;
            }
            // btree.visualize();
            // Sleep(1000);
        }
        else if(option == 1){
            bool ok = search(btree, id);
            tests++;
            if(ok){
                testsPassed++;
            }
        }
        else{
            bool ok = modify(btree, id);
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