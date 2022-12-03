#include "lstm.hpp"

pricebase_t wx[4*HIDDEN_SIZE][INPUT_SIZE] = {};
pricebase_t wh[4*HIDDEN_SIZE][HIDDEN_SIZE] = {};
pricebase_t b[4*HIDDEN_SIZE] = {};
pricebase_t d0[HIDDEN_SIZE][HIDDEN_SIZE_FC] = {};
pricebase_t d1[HIDDEN_SIZE_FC][OUTPUT_SIZE] = {};

