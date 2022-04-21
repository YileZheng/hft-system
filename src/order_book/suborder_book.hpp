#include <hls_math.h>
#include "order_book.hpp"
#include "utils.hpp"

//#define __DEBUG__
#define RANGE 1000
#define CHAIN_LEVELS 200
// #define UNIT 0.01
// #define SLOTSIZE 10



void dut_suborder_book(
	orderMessage order_message,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
);

 
class SubOrderBook{

	float UNIT;
	int SLOTSIZE;

	int INVALID_LINK = (RANGE*2+CHAIN_LEVELS);
	// book
	price_depth_chain book[RANGE*2+CHAIN_LEVELS];		// 0 bid 1 ask
	price_t optimal_prices[2] = {0, 0};
	addr_index base_bookIndex[2] = {0, 0};

	// hole management
	hls::stream<link_t, CHAIN_LEVELS> hole_fifo;//("hole fifo");
	hls::stream<orderMessage, 20> order_message_fifo;//("order fifo");		//TODO
	link_t stack_top = RANGE*2;

	// control signal
	ap_uint<1> read_en = 0;
	ap_uint<1> read_DONE = 0;
	ap_uint<1> update_en = 1;

	// orderbook update
	addr_index get_bookindex_offset(
		price_t &price_in,
		price_t &price_base,
		ap_uint<1> &bid
	);

	addr_index get_maintain_bookIndex(
		order &order_info,
		ap_uint<1> &bid,
		orderOp &direction
	);
		
	void update_optimal(
		unsigned int &bid_ask
	);

	void update_book(
		order &order_info,
		ap_uint<1> &bid,
		orderOp &direction,		// new change remove

		addr_index &bookIndex_in
	);

	// fifo operation
	void store_stack_hole(
		link_t &hole
	);

	link_t get_stack_insert_index();


	public:
	// constructor
	SubOrderBook(int SLOTSIZE_T, float UNIT_T){
		SLOTSIZE = SLOTSIZE_T;
		UNIT = UNIT_T;
	}

	// orderbook read
	void book_read(
		hls::stream<price_depth> &feed_stream_out
	);
	
	// controller
	void subbook_controller(
		ap_uint<1> &req_read_in
	);

	// orderbook update
	void book_maintain(
		orderMessage order_message
	);

	void suborder_book(
		orderMessage order_message,
		ap_uint<1> req_read_in,
		hls::stream<price_depth> &feed_stream_out
	);

};
