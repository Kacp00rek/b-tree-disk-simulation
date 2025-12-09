#include <iostream>
#include <random>
#include <windows.h>
#include "btree.h"
#include "record.h"

using namespace std;
using RecordType = Record;

unordered_map<Key, RecordType> hashmap;


bool insert(BTree<RecordType> &btree, int &id){
    Key key;
    STATUS expected;

    if(rand() % 2 == 0 || id == 0){
        key = id++;
    }
    else{
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<int> k(0, id - 1);
        key = k(gen);
    }

    RecordType record = RecordType::random(key);
    cout << "INSERT: " << record << "\n";

    if(hashmap.find(key) != hashmap.end()){
        expected = ALREADY_EXISTS;
    }
    else{
        expected = OK;
        hashmap[record.key] = record;
    }

    STATUS status = btree.insert(record);

    bool verdict = status == expected;
    cout << "EXPECTED: " << expected << "\n";
    cout << "GOT:      " << status << "\n";
    cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

bool search(BTree<RecordType> &btree, int id){
    bool verdict;

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> k(0, id - 1);

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
    uniform_int_distribution<int> k(0, id - 1);
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
    cout << "GOT:      " << status << "\n";
    cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

bool remove(BTree<RecordType> &btree, int id){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> k(0, id - 1);
    Key key = k(gen);

    cout << "REMOVE: " << key << "\n";
    STATUS status = btree.remove(key);
    STATUS expected;

    if(hashmap.find(key) != hashmap.end()){
        expected = OK;
        hashmap.erase(key);
    }
    else{
        expected = DOESNT_EXIST;
    }

    bool verdict = status == expected;
    cout << "EXPECTED: " << expected << "\n";
    cout << "GOT:      " << status << "\n";
    cout << "VERDICT: " << (verdict ? "PASSED" : "FAILED") << "\n";

    return verdict;
}

void tests(){
    int id = 0, testsPassed = 0, options = 4;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> opt(0, 1);

    vector<double> odds = {0.3, 0.2, 0.2, 0.3};
    int tests = 250000;
    int startingPoint = 10000;

    for(int i = 1; i < (int)odds.size(); i++){
        odds[i] += odds[i - 1];
    }   

    BTree<RecordType> btree;

    for(int i = 0; i < startingPoint; i++){
        insert(btree, id);
    }

    for(int i = 0; i < tests; i++){
        cout << "\n";
        double option = opt(gen);
        bool ok;
        if(option <= odds[0]){
            ok = insert(btree, id);
        }
        else if(option <= odds[1]){
            ok = search(btree, id);
        }
        else if(option <= odds[2]){
            ok = modify(btree, id);
        }
        else{
            ok = remove(btree, id);
        }
        if(ok){
            testsPassed++;
        }
    }

    cout << "\n";
    cout << testsPassed << " / " << tests;
}

void visualisation(){
    BTree<RecordType> btree;
    vector<Key> keys;
    int n = 1000;
    for(int i = 0; i < n; i++){
        keys.push_back(i);
    }
    random_device rd;
    default_random_engine rng(rd());
    shuffle(keys.begin(), keys.end(), rng);
    
    for(int i = 0; i < n; i++){
        RecordType record = RecordType::random(keys[i]);
        btree.insert(record);
    }
    btree.visualize();

    for(int i = 0; i < n; i++){
        cout << "REMOVING: " << keys[i] << "\n\n";
        getchar();
        btree.remove(keys[i]);
        btree.visualize();
    }


}

int main(){

    srand(time(NULL));

    bool visualisationMode = false;

    if(visualisationMode){
        visualisation();
    }
    else{
        tests();
    }


    return 0;
}