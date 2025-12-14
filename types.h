#pragma once

#define D                   2
#define NODES_CACHE_SIZE    5
#define BLOCKING_FACTOR     10
#define RECORDS_CACHE_SIZE  16
#define NULL_PAGE           -1
#define NULL_KEY            -1

#include <vector>
#include <stdint.h>
#include "address.h"
using namespace std;
using Page = int;
using Byte = uint8_t;
using Data = vector<Byte>;
using Key = long long;
enum STATUS { OK, ALREADY_EXISTS, DOESNT_EXIST };
ostream& operator<<(ostream& os, STATUS c){
    switch(c){
        case OK:
            os << "OK";
            break;
        case ALREADY_EXISTS:
            os << "ALREADY_EXISTS";
            break;
        case DOESNT_EXIST:
            os << "DOESNT_EXIST";
            break;
    }
    return os;
}