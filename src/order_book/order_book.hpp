#include "suborder_book.hpp"
#include <hls_stream.h>

#define CAPACITY 1024   // up to 4096, number of orders in each side
#define LEVELS 100       // up to 128, number of price levels in the order book
#define STOCKS 10       // up to 4096 (related to bookIndex, symbol_list)

#define AS_RANGE 1000
#define AS_CHAIN_LEVELS 200
#define AS_UNIT 0.01
#define AS_SLOTSIZE 10

#ifndef __ORDER_BOOK_H__
#define __ORDER_BOOK_H__

int symbol_mapping(
	symbol_t symbol_map[STOCKS],
	symbol_t symbol
);


void order_book(
	// data
	Message *stream_in,
	price_depth *stream_out,

	// configuration inputs
	symbol_t axi_read_symbol,
	ap_uint<8> axi_read_max,
	int axi_size,

	// control input
	char axi_instruction  // void, run, halt, read book, clear, config symbol map | read_max 

);

#endif
