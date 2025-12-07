#pragma once
#define D   1
#define BLOCKING_FACTOR   100
#define NULL_PAGE   -1
#define NULL_KEY    -1
#include <vector>
#include <stdint.h>
#include "address.h"
using namespace std;
using Page = int;
using Data = vector<uint8_t>;
using Key = long long;