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
    int tests = 1000000;
    int startingPoint = 10;

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
        //btree.visualize();
        //getchar();
    }

    cout << "\n";
    cout << testsPassed << " / " << tests;
}

void interacive(){
    BTree<RecordType> btree;
    btree.visualize();
    Key maxKey = 0;

    while(true){
        cout << "Choose an action:\n";
        cout << "1. INSERT\n";
        cout << "2. INSERT RANDOM RECORDS\n";
        cout << "3. REMOVE\n";
        cout << "4. MODIFY\n";
        cout << "5. SEARCH\n";
        cout << "6. PRINT ALL RECORDS\n";
        cout << "7. END\n";
        int option;
        cin >> option;

        int reads = DiskManager::READS;
        int writes = DiskManager::WRITES;
        
        if(option == 1){
            cout << "\nINSERTING...\n";
            RecordType record;
            cin >> record;
            STATUS status = btree.insert(record);
            if(status == OK){
                cout << record << " INSERTED SUCCESSFULLY\n";
                maxKey = max(maxKey, record.key + 1);
            }
            else{
                cout << "KEY " << record.key << " " << status << "\n";
            }
        }
        else if(option == 2){
            cout << "INSERTING RANDOM RECORDS...\n";
            cout << "N: ";
            int n;
            cin >> n;
            vector<Key> keys;
            for(int i = 0; i < n; i++){
                keys.push_back(maxKey++);
            }
            random_device rd;
            default_random_engine rng(rd());
            shuffle(keys.begin(), keys.end(), rng);
            for(int i = 0; i < n; i++){
                RecordType record = RecordType::random(keys[i]);
                btree.insert(record);
            }
        }
        else if(option == 3){
            cout << "\nREMOVING...\n";
            Key key;
            cout << "KEY: ";
            cin >> key;
            STATUS status = btree.remove(key);
            if(status == OK){
                cout << key << " REMOVED SUCCESSFULLY\n";
            }
            else{
                cout << "KEY " << key << " " << status << "\n";
            }
        }
        else if(option == 4){
            cout << "\nMODYFING...\n";
            RecordType record;
            cin >> record;
            STATUS status = btree.modify(record);
            if(status == OK){
                cout << record << " MODIFIED SUCCESSFULLY\n";
            }
            else{
                cout << "KEY " << record.key << " " << status << "\n";
            }
        }
        else if(option == 5){
            cout << "\nSEARCHING...\n";
            Key key;
            cout << "KEY: ";
            cin >> key;
            auto result = btree.search(key);
            if(result == nullopt){
                cout << "KEY: " << key << " " << DOESNT_EXIST << "\n";
            }
            else{
                cout << *result << "\n";
            }
        }
        else if(option == 6){
            cout << "\nPRINTING...\n";
            btree.printAll();
        }
        else{
            break;
        }
        cout << "READS: " << DiskManager::READS - reads << "\n";
        cout << "WRITES: " << DiskManager::WRITES - writes << "\n";
        cout << "\n";
        btree.visualize();
    }

}

void visualisation(){
    BTree<RecordType> btree;
    vector<Key> keys;
    int n = 20;
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
        int currentREADS = DiskManager::READS;
        int currentWRITES = DiskManager::WRITES;
        btree.remove(keys[i]);
        btree.visualize();

        cout << "READS: " << DiskManager::READS - currentREADS << "\n";
        cout << "WRITES: " << DiskManager::WRITES - currentWRITES << "\n";
    }


}

int main(){

    srand(time(NULL));

    tests();


    return 0;
}