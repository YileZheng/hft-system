#include "common.hpp"
#include <hls_stream.h>

#define CAPACITY 1024   // up to 4096, number of orders in each side
#define LEVELS 100       // up to 128, number of price levels in the order book
#define STOCKS 10       // up to 4096 (related to bookIndex, symbol_list)
