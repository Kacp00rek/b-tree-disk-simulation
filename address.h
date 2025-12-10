
#pragma once
#include <functional>
#include <string>

using namespace std;

struct Address{
    int page;
    int offset;

    bool operator==(const Address& other) const{
        return page == other.page && offset == other.offset;
    }
    bool operator<(const Address& other) const{
        if(page != other.page){
            return page < other.page;
        }
        return offset < other.offset;
    }
};

namespace std{
    template<>
    struct hash<Address>{
        size_t operator()(const Address& address) const{
            size_t h1 = hash<int>{}(address.page);
            size_t h2 = hash<int>{}(address.offset);
            return h1 ^ (h2 << 1);
        }
    };
}