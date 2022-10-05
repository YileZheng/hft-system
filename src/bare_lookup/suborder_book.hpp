#include <hls_math.h>
#include <hls_stream.h>
#include "common.hpp"
#include "utils.hpp"


#ifndef __SUBORDER_BOOK_H__
#define __SUBORDER_BOOK_H__
//#define __DEBUG__

#define LAST_CLOSE 585.3
#define AS_UNIT 0.01
#define BOOK_SIZE (int)(LAST_CLOSE/10/AS_UNIT*2)


void dut_suborder_book(
	orderMessage order_message,
	ap_uint<8> read_max,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
);

void read_book(
    price_depth book[BOOK_SIZE],
    addr_index topbid_bookIndex,
	ap_uint<8> read_max,
	hls::stream<price_depth> &feed_stream_out
);
//
//void read_book(
//    price_depth book[BOOK_SIZE],
//	hls::stream<price_depth> &feed_stream_out
//);


#endif
