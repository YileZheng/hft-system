#define __gmp_const const
//#include <gmp.h>
//#include <mpfr.h>
#include "suborder_book.hpp"
#include "profile.hpp"



void dut_suborder_book(
	orderMessage order_message,
	ap_uint<8> read_max,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
){
	
	static SubOrderBook<RANGE, CHAIN_LEVELS> dut(AS_SLOTSIZE, AS_UNIT);
	transMessage trans_message={order_message, 1};
	dut.suborderbook(trans_message, read_max, req_read_in, feed_stream_out);
}

