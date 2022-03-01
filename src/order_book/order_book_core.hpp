
#include <hls_math.h>

#define RANGE 1000 		// RANGE level of price to track in order book 
#define UNIT 0.01

typedef signed int addr_index;

void suborder_book(
	order &order_info,		// price size ID
	orderOp &direction,		// new change remove
	ap_uint<1> &bid,
	ap_uint<1> &req_read_in,
	stream<price_depth> &feed_stream_out
);

void book_read(
	ap_uint<1> &req_read_in,
	price_depth book[RANGE][2],
	ap_uint<1> book_valid[RANGE][2],
	stream<price_depth> &feed_stream_out
	); 

addr_index get_bookindex_offset(
	price_t &price_in,
	price_t &price_base
);
	