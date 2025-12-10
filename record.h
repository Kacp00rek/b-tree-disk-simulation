#pragma once
#include "types.h"
#include <string>
#include <random>
#include <fstream>

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

    void getDot(ofstream &file){
        file << "<TD>" << key << "</TD>" << "<TD>" << angle << "</TD>" << "<TD>" << radius << "</TD>";
    }

    static void getHeader(ofstream &file){
        file << "<TD>KEY</TD> <TD>ANGLE</TD> <TD>RADIUS</TD>";
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
    
    static Record deserialize(const Data &data){
        Record record;
        size_t curr = 0;

        memcpy(&record.key, data.data() + curr, sizeof(key));
        curr += sizeof(key);

        memcpy(&record.angle, data.data() + curr, sizeof(angle));
        curr += sizeof(angle);

        memcpy(&record.radius, data.data() + curr, sizeof(radius));

        return record;
    }

    static Record random(Key key){
        Record record;
        record.key = key;
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<double> ang(0, 360);
        uniform_real_distribution<double> rad(0, 100);
        record.angle = ang(gen);
        record.radius = rad(gen);
        
        return move(record);
    }

    friend istream& operator>>(istream& in, Record& r){
        cout << "KEY: ";
        in >> r.key;
        cout << "ANGLE: ";
        in >> r.angle;
        cout << "RADIUS: ";
        in >> r.radius;
        return in;
    }

    friend ostream& operator<<(ostream &os, const Record &r){
        os << "[ " << r.key << ", " << r.angle << ", " << r.radius << " ]";
        return os;
    }

    bool operator==(const Record &other) const {
        return key == other.key && angle == other.angle && radius == other.radius;
    }
};