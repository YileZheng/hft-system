#define __gmp_const const
//#include <gmp.h>
//#include <mpfr.h>
#include "suborder_book.hpp"


#define AS_UNIT 0.01
#define AS_SLOTSIZE 10
#define RANGE 1000
#define CHAIN_LEVELS 200


void dut_suborder_book(
	orderMessage order_message,
	ap_uint<8> read_max,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
){
	
	static SubOrderBook<RANGE, CHAIN_LEVELS> dut(AS_SLOTSIZE, AS_UNIT);
	transMessage trans_message={order_message, 1};
	dut.suborder_book(trans_message, read_max, req_read_in, feed_stream_out);
}

