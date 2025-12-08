#pragma once
#define D   2
#define BLOCKING_FACTOR   256
#define NULL_PAGE   -1
#define NULL_KEY    -1
#include <vector>
#include <stdint.h>
#include "address.h"
using namespace std;
using Page = int;
using Byte = uint8_t;
using Data = vector<Byte>;
using Key = long long;