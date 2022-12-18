
#ifndef __SUBORDER_BOOK_H__
#define __SUBORDER_BOOK_H__
//#define __DEBUG__
#include <hls_math.h>
#include <hls_stream.h>
#include "common.hpp"
#include "utils.hpp"


void dut_suborder_book(
	orderMessage order_message,
	ap_uint<8> read_max,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
);



#endif
