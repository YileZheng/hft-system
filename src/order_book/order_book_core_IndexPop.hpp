#include <hls_math.h>

#define RANGE 1000 		// RANGE level of price to track in order book 
#define UNIT 0.01
#define SLOTSIZE 10
#define CHAIN_LEVELS (LEVELS*2)  // the memory size of the link table which stores linked order depths
#define INVALID_LINK CHAIN_LEVELS


typedef signed int addr_index;

addr_index get_bookindex_offset(
	price_t &price_in,
	price_t &price_base,
	ap_uint<1> &bid
);

bool is_after(
	price_t &price_in,
	price_t &price_onchain,
	ap_uint<1> &bid
);

void book_read(
	ap_uint<1> &req_read,
	price_depth_chain book[RANGE][2],
	addr_index base_bookIndex[2],
	stream<price_depth> &feed_stream_out
);

void store_stack_hole(
	link_t hole_fifo[CHAIN_LEVELS],
	int &hole_fifo_head, 
	link_t &stack_top,
	link_t &hole
);

link_t get_stack_insert_index(
	link_t hole_fifo[CHAIN_LEVELS],
	int &hole_fifo_head, 
	int &hole_fifo_tail,
	link_t &stack_top
);

void update_optimal(
	price_depth_chain book[RANGE][2],
	price_t optimal_prices[2],
	addr_index base_bookIndex[2],
	unsigned int &bid_ask
);

void suborder_book(
	order &order_info,		// price size ID
	orderOp &direction,		// new change remove
	ap_uint<1> &bid,
	ap_uint<1> &req_read_in,
	stream<price_depth> &feed_stream_out
);