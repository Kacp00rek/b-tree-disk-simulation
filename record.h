#pragma once
#include "types.h"
#include <string>

using namespace std;

struct Record{
    Key key;
    double angle;
    double radius;

    static const size_t size = sizeof(Key) + sizeof(angle) + sizeof(radius);

    Record(Key k = 0, double a = 0, double r = 0){
        key = k;
        angle = a;
        radius = r;
    }

    Data serialize(){
        Data data(size);
        size_t curr = 0;

        memcpy(data.data() + curr, &key, sizeof(key));
        curr += sizeof(key);

        memcpy(data.data() + curr, &angle, sizeof(angle));
        curr += sizeof(angle);

        memcpy(data.data() + curr, &radius, sizeof(radius));

        return data;
    }
    
    static Record deserialize(Data &data){
        Record record;
        size_t curr = 0;

        memcpy(&record.key, data.data() + curr, sizeof(key));
        curr += sizeof(key);

        memcpy(&record.angle, data.data() + curr, sizeof(angle));
        curr += sizeof(angle);

        memcpy(&record.radius, data.data() + curr, sizeof(radius));

        return record;
    }
};