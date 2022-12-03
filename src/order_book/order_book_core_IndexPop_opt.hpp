#include "common.hpp"
#include <hls_stream.h>
#include <hls_math.h>

#define RANGE 1000 		// RANGE level of price to track in order book 
#define UNIT 0.01
#define SLOTSIZE 10
#define CHAIN_LEVELS (LEVELS*2)  // the memory size of the link table which stores linked order depths
#define INVALID_LINK (RANGE*2+CHAIN_LEVELS)


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
	price_depth_chain book[RANGE*2+CHAIN_LEVELS],
	addr_index base_bookIndex[2],
	hls::stream<price_depth> &feed_stream_out,
	
	ap_uint<1> read_en,
	ap_uint<1> &read_DONE
);

void store_stack_hole(
	hls::stream<link_t> &hole_fifo,
	link_t &stack_top,
	link_t &hole
);

link_t get_stack_insert_index(
	hls::stream<link_t> &hole_fifo,
	link_t &stack_top
);

void update_optimal(
	price_depth_chain book[RANGE*2+CHAIN_LEVELS],
	price_t optimal_prices[2],
	addr_index base_bookIndex[2],
	unsigned int &bid_ask
);

addr_index get_maintain_bookIndex(
	order &order_info,
	ap_uint<1> &bid,
	orderOp &direction,		// new change remove
	price_depth_chain book[RANGE*2+CHAIN_LEVELS],		// 0 bid 1 ask

	addr_index base_bookIndex[2],
	price_t optimal_prices[2],

	hls::stream<link_t> &hole_fifo,
	link_t &stack_top
);

void update_book(
	order &order_info,
	ap_uint<1> &bid,
	orderOp &direction,		// new change remove
	price_depth_chain book[RANGE*2+CHAIN_LEVELS],		// 0 bid 1 ask

	hls::stream<link_t> &hole_fifo,
	link_t &stack_top,

	addr_index &bookIndex_in,
	addr_index base_bookIndex[2],
	price_t optimal_prices[2]
);

void subbook_controller(
	ap_uint<1> &req_read_in,
	ap_uint<1> &read_en,
	ap_uint<1> &read_DONE,
	ap_uint<1> &update_en
);

void book_maintain(
	hls::stream<orderMessage> &order_message,
	price_depth_chain book[RANGE*2+CHAIN_LEVELS],		// 0 bid 1 ask

	hls::stream<link_t> &hole_fifo,
	link_t &stack_top,

	addr_index base_bookIndex[2],
	price_t optimal_prices[2],

	ap_uint<1> &update_en
);

void suborder_book(
	hls::stream<orderMessage> &order_message,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
);