
#include "order_book.hpp"
#include "suborder_book.hpp"

#define UNIT 0.01
#define SLOTSIZE 10


void suborder_book(
	orderMessage order_message,
	ap_uint<1> req_read_in,
	stream<price_depth> &feed_stream_out
){
	
	SubOrderBook<1000, 200> dut(SLOTSIZE, UNIT);
	dut.suborder_book(order_message, req_read_in, feed_stream_out);
}