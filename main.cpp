#include <iostream>
#include <random>
#include <windows.h>
#include "btree.h"
#include "record.h"
#include <conio.h>
#include <algorithm>

using namespace std;
using RecordType = Record;

char selectOption(const vector<char> &options){
    char c;
    while(true){
        c = _getch();
        if(find(options.begin(), options.end(), c) != options.end()){
            return c;
        }
    }
}

void insert(Key key, vector<Key> &v, ofstream &file){
    RecordType record = RecordType::random(key);
    v.push_back(key);
    file << "INSERT " << record << "\n";
}

void search(vector<Key> &v, ofstream &file){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> idx(0, (int)v.size() - 1);
    int index = idx(gen);
    Key key = v[index];
    file << "SEARCH " << key << "\n";
}

void modify(vector<Key> &v, ofstream &file){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> idx(0, (int)v.size() - 1);
    int index = idx(gen);
    Key key = v[index];
    RecordType record = RecordType::random(key);
    file << "MODIFY " << record << "\n";
}

void remove(vector<Key> &v, ofstream &file){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> idx(0, (int)v.size() - 1);
    int index = idx(gen);
    Key key = v[index];
    v.erase(find(v.begin(), v.end(), key));
    file << "REMOVE " << key << "\n";
}

void generateTest(const string &filename){
    ofstream file(filename);

    int id = 0, testsPassed = 0, options = 4;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> opt(0, 1);
    vector<Key> v;

    vector<double> odds(4, 0);
    cout << "ODDS FOR INSERTING: ";
    cin >> odds[0];

    cout << "ODDS FOR REMOVING: ";
    cin >> odds[1];

    cout << "ODDS FOR MODYFING: ";
    cin >> odds[2];

    cout << "ODDS FOR SEARCHING: ";
    cin >> odds[3];

    for(int i = 1; i < (int)odds.size(); i++){
        odds[i] += odds[i - 1];
    }  
    int tests, startingPoint;

    cout << "NUMBER OF TESTS: ";
    cin >> tests;

    cout << "STARTING NUMBER OF RECORDS: ";
    cin >> startingPoint;

    BTree<RecordType> btree;

    vector<Key> keys;
    for(int i = 0; i < tests + startingPoint; i++){
        keys.push_back(i);
    }

    default_random_engine rng(rd());
    shuffle(keys.begin(), keys.end(), rng);
    int index = 0;

    for(int i = 0; i < startingPoint; i++){
        insert(keys[index++], v, file);
    }

    for(int i = 0; i < tests; i++){
        double option = opt(gen);
        if(option <= odds[0] || v.size() == 0){
            insert(keys[index++], v, file);
        }
        else if(option <= odds[1]){
            remove(v, file);
        }
        else if(option <= odds[2]){
            modify(v, file);
        }
        else{
            search(v, file);
        }
    }

    file.close();
}

void executeTest(const string &filename){
    fstream file(filename);
    string action;
    BTree<RecordType> btree;
    unordered_map<Key, RecordType> hashmap;
    int passed = 0;
    int counter = 0;
    double ratio = 0;
    while(file >> action){
        counter++;
        bool verdict;
        int reads = DiskManager::READS;
        int writes = DiskManager::WRITES;
        if(action == "INSERT"){
            RecordType record;
            file >> record;
            STATUS status = btree.insert(record);
            STATUS expected = hashmap.find(record.key) == hashmap.end() ? OK : ALREADY_EXISTS;
            if(expected == OK){
                hashmap[record.key] = record;
            }
            verdict = (status == expected);
            cout << action << ":  ";
            record.print();
            cout << "\n";
            cout << "EXPECTED: " << expected << "\n";
            cout << "GOT:      " << status << "\n";
        }
        else if(action == "REMOVE"){
            Key key;
            file >> key;
            STATUS status = btree.remove(key);
            STATUS expected = hashmap.find(key) != hashmap.end() ? OK : DOESNT_EXIST;
            if(expected == OK){
                hashmap.erase(key);
            }
            verdict = (status == expected);
            cout << action << ":   " << key << "\n";
            cout << "EXPECTED: " << expected << "\n";
            cout << "GOT:      " << status << "\n";
        }
        else if(action == "MODIFY"){
            RecordType record;
            file >> record;
            STATUS status = btree.modify(record);
            STATUS expected = hashmap.find(record.key) != hashmap.end() ? OK : DOESNT_EXIST;
            if(expected == OK){
                hashmap[record.key] = record;
            }
            cout << action << ":   ";
            record.print();
            cout << "\n";
            cout << "EXPECTED: " << expected << "\n";
            cout << "GOT:      " << status << "\n";
        }
        else{
            Key key;
            file >> key;
            cout << action << ":   " << key << "\n";
            auto result = btree.search(key);
            bool exists1 = (hashmap.find(key) != hashmap.end());
            bool exists2 = (result != nullopt);
            cout << "EXPECTED: ";
            if(exists1){
                hashmap[key].print();
            }
            else{
                cout << DOESNT_EXIST;
            }
            cout << "\nGOT:      ";
            if(exists2){
                (*result).print();
            }
            else{
                cout << DOESNT_EXIST;
            }
            cout << "\n";
            
            if(exists1){
                verdict = (exists2 && *result == hashmap[key]);
            }
            else{
                verdict = !exists2;
            }
        }
        double currRatio = btree.getRatio();
        ratio += currRatio;
        cout << "VERDICT : " << (verdict ? "PASSED" : "FAILED") << "\n";
        cout << "READS:    " << DiskManager::READS - reads << "\n";
        cout << "WRITE:    " << DiskManager::WRITES - writes << "\n";
        cout << "RATIO:    " << currRatio << "\n\n";
        if(verdict){
            passed++;
        }
    }

    cout << "PASSED:        " << passed << "/" << counter << "\n";
    cout << "READS:         " << DiskManager::READS << "\n";
    cout << "WRITES:        " << DiskManager::WRITES << "\n";
    cout << "AVERAGE RATIO: " << ratio / counter << "\n";



    file.close();
}

void test(){
    cout << "CHOOSE:\n";
    cout << "1. GENERATE A TEST\n";
    cout << "2. READ TEST FROM A FILE\n";
    char option = selectOption({'1', '2'});
    system("cls");
    string filename;
    if(option == '1'){
        filename = "test.txt";
        generateTest(filename);
    }
    else{
        cout << "\nNAME OF THE FILE: ";
        cin >> filename;
    }
    system("cls");
    executeTest(filename);
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
        char option = selectOption({'1', '2', '3', '4', '5', '6', '7'});

        int reads = DiskManager::READS;
        int writes = DiskManager::WRITES;
        
        if(option == '1'){
            cout << "\nINSERTING...\n";
            RecordType record = RecordType::input();
            STATUS status = btree.insert(record);
            if(status == OK){
                record.print();
                cout << " INSERTED SUCCESSFULLY\n";
                maxKey = max(maxKey, record.key + 1);
            }
            else{
                cout << "KEY " << record.key << " " << status << "\n";
            }
        }
        else if(option == '2'){
            cout << "INSERTING RANDOM RECORDS...\n";
            cout << "N: ";
            int n;
            cin >> n;
            cout << "PAUSE BEFORE EVERY INSERTION (y/n): ";
            bool pause = (selectOption({'y', 'n'}) == 'y');
            cout << "\n";
            vector<Key> keys;
            for(int i = 0; i < n; i++){
                keys.push_back(maxKey++);
            }
            random_device rd;
            default_random_engine rng(rd());
            shuffle(keys.begin(), keys.end(), rng);
            for(int i = 0; i < n; i++){
                RecordType record = RecordType::random(keys[i]);
                cout << "\n" << i + 1 << ". INSERT ";
                record.print();
                cout << "\n";
                if(pause){
                    btree.visualize();
                    cout << "CLICK ANY KEY TO RESUME\n";
                    _getch();
                }
                btree.insert(record);
                cout << "READS: " << DiskManager::READS - reads << "\n";
                cout << "WRITES: " << DiskManager::WRITES - writes << "\n";
                reads = DiskManager::READS;
                writes = DiskManager::WRITES;
            }
        }
        else if(option == '3'){
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
        else if(option == '4'){
            cout << "\nMODYFING...\n";
            RecordType record = RecordType::input();
            STATUS status = btree.modify(record);
            if(status == OK){
                record.print();
                cout << " MODIFIED SUCCESSFULLY\n";
            }
            else{
                cout << "KEY " << record.key << " " << status << "\n";
            }
        }
        else if(option == '5'){
            cout << "\nSEARCHING...\n";
            Key key;
            cout << "KEY: ";
            cin >> key;
            auto result = btree.search(key);
            if(result == nullopt){
                cout << "KEY: " << key << " " << DOESNT_EXIST << "\n";
            }
            else{
                (*result).print();
                cout << "\n";
            }
        }
        else if(option == '6'){
            cout << "\nPRINTING...\n";
            btree.printAll();
        }
        else{
            break;
        }
        if(option != '2'){
            cout << "READS: " << DiskManager::READS - reads << "\n";
            cout << "WRITES: " << DiskManager::WRITES - writes << "\n"; 
        }
        cout << "\n";
        btree.visualize();
    }

}

int main(){
    srand(time(NULL));

    cout << "SELECT MODE:\n";
    cout << "1. INTERACTIVE\n";
    cout << "2. TESTING\n";
    char option = selectOption({'1', '2'});
    system("cls");

    option == '1' ? interacive() : test();


    return 0;
}