// #define __gmp_const const
// #include <gmp.h>
// #include <mpfr.h>
#include "suborder_book.hpp"


template <int RANGE, int CHAIN_LEVELS>
class SubOrderBook{

	int SLOTSIZE;
	price_t UNIT;
	price_t UNIT_SLOT;
	int READ_CNT = 10;

	int INVALID_LINK = (RANGE*2+CHAIN_LEVELS);
	// book
	price_t optimal_prices[2] = {0, 0};

	// hole management
	hls::stream<link_t, CHAIN_LEVELS> hole_fifo;//("hole fifo");
	link_t stack_top = RANGE*2;

	// orderbook update
	addr_index get_bookindex_offset(
		price_t &price_in,
		price_t &price_base,
		ap_uint<1> &bid
	);

	addr_index get_maintain_bookIndex(
		order &order_info,
		ap_uint<1> &bid,
		orderOp &direction,
		price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
	);
		
	void update_optimal(
		unsigned int &bid_ask,
		price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
	);

	void update_book(
		order &order_info,
		ap_uint<1> &bid,
		orderOp &direction,		// new change remove

		addr_index &bookIndex_in,
		price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
	);

	// fifo operation
	void store_stack_hole(
		link_t &hole
	);

	link_t get_stack_insert_index();


	public:
	// price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS];		// 0 bid 1 ask
	addr_index base_bookIndex[2] = {0, 0};
	// constructor
	SubOrderBook(){
		SLOTSIZE = 10;
		UNIT = (price_t)0.01;
		UNIT_SLOT = SLOTSIZE*UNIT;
	}

	SubOrderBook(int SLOTSIZE_T, float UNIT_T){
		SLOTSIZE = SLOTSIZE_T;
		UNIT = (price_t)UNIT_T;
		UNIT_SLOT = SLOTSIZE*UNIT;
	}

	void config(int SLOTSIZE_T, float UNIT_T){
		SLOTSIZE = SLOTSIZE_T;
		UNIT = (price_t)UNIT_T;
		UNIT_SLOT = SLOTSIZE*UNIT;
	}

	// orderbook update
	void book_maintain(
		orderMessage Order_message,
		price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
	);

};


void dut_suborder_book(
	orderMessage order_message,
	price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS],
	addr_index &book_head_b,
	addr_index &book_head_a
){
	

#pragma HLS INTERFACE m_axi depth=4096 max_read_burst_length=4 max_write_burst_length=4  bundle=gmem0 port=book offset=slave 

// #pragma HLS INTERFACE m_axi depth=4096 bundle=gmem0 port=book offset=slave 

#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=order_message 	bundle=control
#pragma HLS INTERFACE s_axilite port=book 			bundle=control
#pragma HLS INTERFACE s_axilite port=book_head_b 	bundle=control
#pragma HLS INTERFACE s_axilite port=book_head_a 	bundle=control

	static SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> dut(AS_SLOTSIZE, AS_UNIT);
	dut.book_maintain(order_message, book);
	book_head_b = dut.base_bookIndex[0];
	book_head_a = dut.base_bookIndex[1];
}



// fifo operations
template <int RANGE, int CHAIN_LEVELS>
link_t SubOrderBook<RANGE, CHAIN_LEVELS>::get_stack_insert_index(){
	link_t addr_out;
	if (!hole_fifo.empty()){
		addr_out = hole_fifo.read();
	}else{
#ifndef __SYNTHESIS__
if (stack_top == INVALID_LINK){
	printf("Order book maximum stack size reached !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
}
#endif
		addr_out = stack_top++;		// TODO: how to deal with overflow (default will not overflow)
	}
	return addr_out;
}

template <int RANGE, int CHAIN_LEVELS>
void SubOrderBook<RANGE, CHAIN_LEVELS>::store_stack_hole(
	link_t &hole
){
#pragma HLS inline
	if (hole == stack_top-1){
		stack_top--;
	}else{
		hole_fifo.write(hole);
	}
}

// orderbook update
template <int RANGE, int CHAIN_LEVELS>
addr_index SubOrderBook<RANGE, CHAIN_LEVELS>::get_bookindex_offset(
	price_t &price_in,
	price_t &price_base,
	ap_uint<1> &bid
){	

#pragma HLS INLINE off
	// price_t is unsigned, cannot be negetive, but price difference can be negetive
	price_delta_t price1 = price_in,  price2 = price_base;
	price_delta_t price_diff = bid? (price2-price1): (price1-price2);
	// float price1 = price_in,  price2 = price_base;
	// float price_diff = bid? (price2-price1): (price1-price2);
	addr_index offset = hls::abs( hls::floor( hls::round( price_diff/(UNIT)) /SLOTSIZE ));
	// addr_index offset = hls::abs( hls::floor( hls::round(price_diff/(price_t)(UNIT)) /(price_t)SLOTSIZE));
	return offset;
}


template <int RANGE, int CHAIN_LEVELS>
void SubOrderBook<RANGE, CHAIN_LEVELS>::update_optimal(
	unsigned int &bid_ask,
	price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
){
	price_t cur_price; 
	addr_index book_side_offset = bid_ask? RANGE: 0;
	unsigned int bookIndex_tmp;//, bookIndex_tmp_get;
	bookIndex_tmp = base_bookIndex[bid_ask]+1;
	bookIndex_tmp = (bookIndex_tmp>=RANGE)? bookIndex_tmp-RANGE: bookIndex_tmp;
	bookIndex_tmp += book_side_offset;
	cur_price = book[bookIndex_tmp].price;
	int i;
#ifdef __DEBUG__
	std::cout<<"DEBUG - {search new optimal backward} ";
	std::cout<<" origin base: "<<base_bookIndex[bid_ask];
	std::cout<<" origin optimal: "<<optimal_prices[bid_ask];
#endif
	SEARCH_NEXT_OPTIMAL:
	for (i=2; i<=RANGE; i++){		// search backward in the book starting with orginal base_bookIndex
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN
		if (cur_price != 0){		// find valid slot
			base_bookIndex[bid_ask] = bookIndex_tmp;
			// optimal_prices[bid_ask] = (bid_ask)? (price_t)(optimal_prices[bid_ask]-(price_t)(UNIT*SLOTSIZE*(i-1))):
			// 									(price_t)(optimal_prices[bid_ask]+(price_t)(UNIT*SLOTSIZE*(i-1)));
			optimal_prices[bid_ask] = (bid_ask)? (price_t)(optimal_prices[bid_ask]-(price_t)(UNIT_SLOT*(i-1))):
												(price_t)(optimal_prices[bid_ask]+(price_t)(UNIT_SLOT*(i-1)));
			break;
		}
		bookIndex_tmp = base_bookIndex[bid_ask]+i;
		bookIndex_tmp = (bookIndex_tmp>=RANGE)? bookIndex_tmp-RANGE: bookIndex_tmp;
		bookIndex_tmp += book_side_offset;
		cur_price = book[bookIndex_tmp].price;
	}
#ifdef __DEBUG__
	std::cout<<" new base:"<<base_bookIndex[bid_ask];
	std::cout<<" new price: "<<optimal_prices[bid_ask];
	std::cout<<std::endl;
#endif
}

template <int RANGE, int CHAIN_LEVELS>
addr_index SubOrderBook<RANGE, CHAIN_LEVELS>::get_maintain_bookIndex(
	order &order_info,
	ap_uint<1> &bid,
	orderOp &direction,
	price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
){
	unsigned int bid_ask = bid;
	addr_index book_side_offset = bid? RANGE: 0;
	addr_index base_bookIndex_tmp;
	ap_uint<1> is_better_price;

	// base index update
		// first price write back
	if (optimal_prices[bid_ask] == 0) optimal_prices[bid_ask] = order_info.price;
		// when more optimal price comes in
	
	if (direction == NEW){
		is_better_price = is_after(optimal_prices[bid_ask], order_info.price, bid);
		if (is_better_price){
			addr_index optimal_offset = get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
			base_bookIndex_tmp = base_bookIndex[bid_ask] - optimal_offset;
	#ifdef __DEBUG__
			std::cout<<"DEBUG - {better optimal price} ";
			std::cout<<" origin base: "<<base_bookIndex[bid_ask];
			std::cout<<" better base:"<<base_bookIndex_tmp;
			std::cout<<" offset from origin base: "<<get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
			std::cout<<" order price: "<<order_info.price;
			std::cout<<" origin optimal "<<optimal_prices[bid_ask];
			std::cout<<std::endl;
	#endif
			// optimal_prices[bid_ask] = (bid)? (price_t)(optimal_prices[bid_ask]+(price_t)(optimal_offset*UNIT*SLOTSIZE)): (price_t)(optimal_prices[bid_ask]-(price_t)(optimal_offset*UNIT*SLOTSIZE));
			optimal_prices[bid_ask] = (bid)? (price_t)(optimal_prices[bid_ask]+(price_t)(optimal_offset*UNIT_SLOT)): (price_t)(optimal_prices[bid_ask]-(price_t)(optimal_offset*UNIT_SLOT));
	#ifdef __DEBUG__
			std::cout<<"DEBUG - {better optimal price} ";
			std::cout<<" origin base: "<<base_bookIndex[bid_ask];
			std::cout<<" better base:"<<base_bookIndex_tmp;
			std::cout<<" offset from origin base: "<<get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
			std::cout<<" order price: "<<order_info.price;
			std::cout<<" new optimal "<<optimal_prices[bid_ask];
			std::cout<<std::endl;
	#endif
			// empty least optimal price levels
			UPDATE_OPTIMAL_CLEAR_LEVEL_TAIL:
			for (int i=0; i<RANGE; i++){
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN
				int i_spare = base_bookIndex_tmp+i;
				i_spare = i_spare<0? (i_spare+RANGE): i_spare; // TODO: what if i_spare still negetive
				if (i_spare == base_bookIndex[bid_ask]) 
					break;
				i_spare += book_side_offset;

				price_depth_chain cur_block = book[i_spare];
				if (cur_block.price != 0){
					book[i_spare].price = 0;
					// record holes obtained from clearing this chain
					UPDATE_OPTIMAL_CLEAR_STORE_HOLES:
					for (int j=0; j<SLOTSIZE; j++){
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN
						if (cur_block.next != INVALID_LINK)
							store_stack_hole(cur_block.next);
						else break;
						cur_block = book[cur_block.next];
					}
				}
				// TODO: may also put into stack memory
			}
			base_bookIndex[bid_ask] = (base_bookIndex_tmp<0)? base_bookIndex_tmp+RANGE: base_bookIndex_tmp;  // TODO: still <0 ?
		}
	}

	// calculate index
	addr_index bookIndex = base_bookIndex[bid_ask] + get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
	return bookIndex;
}


template <int RANGE, int CHAIN_LEVELS>
void SubOrderBook<RANGE, CHAIN_LEVELS>::update_book(
	order &order_info,
	ap_uint<1> &bid,
	orderOp &direction,		// new change remove

	addr_index &bookIndex_in,
	price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
){
	unsigned int bid_ask = bid;
	addr_index book_side_offset = bid? RANGE: 0;
	
	addr_index bookIndex = bookIndex_in;
	price_depth_chain chain_new, chain_head;
	link_t stack_insert_index;

	// book update
	chain_new.price = order_info.price;
	chain_new.size = order_info.size;
	
	if (bookIndex < base_bookIndex[bid_ask]+RANGE){
		bookIndex = (bookIndex>=RANGE)? bookIndex-RANGE: bookIndex;
		bookIndex += book_side_offset;
		chain_head = book[bookIndex];

		if (direction == NEW){
			// when there is no valid price level in this slot
			if (chain_head.price == 0){
#ifdef __DEBUG__
	std::cout<<"DEBUG - NEW - chain head = 0, insert to a new chain ";
	std::cout<<std::endl;
#endif
				chain_new.next = INVALID_LINK;
				book[bookIndex] = chain_new;
			// when slot valid
			}else{
				// if hit at the link head
				if (chain_head.price == order_info.price){
#ifdef __DEBUG__
	std::cout<<"DEBUG - NEW - hit at the chain head ";
	std::cout<<std::endl;
#endif
					book[bookIndex].size = chain_head.size + order_info.size; 
				}else{
					// when the price should be located somewhere after this block on this chain, 
					// search the chain: 
					// price hit: add up size; 
					// price miss: link a new block (1. already iterate to the price should be after this one; 2. run to the end of the chain)
					if (is_after(order_info.price, chain_head.price, bid)){
						link_t last_bookIndex = bookIndex;
						link_t cur_bookIndex = chain_head.next;
						price_depth_chain cur_block = book[cur_bookIndex];
						BOOK_NEW_SEARCH_LOC:
						for (int i=0; i<SLOTSIZE; i++){
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN
							if (cur_bookIndex == INVALID_LINK){
								// miss: run to the end
#ifdef __DEBUG__
	std::cout<<"DEBUG - NEW - insert to the chain tail ";
	std::cout<<std::endl;
#endif
								stack_insert_index = get_stack_insert_index();
								book[last_bookIndex].next = stack_insert_index;
								chain_new.next = INVALID_LINK;
								book[stack_insert_index] = chain_new;
								break;
							}else{
								if (cur_block.price == order_info.price){
									// hit
//									chain_new.size += cur_block.size;
//									chain_new.next = cur_block.next;
#ifdef __DEBUG__
	std::cout<<"DEBUG - NEW - hit in the chain ";
	std::cout<<std::endl;
#endif
									book[cur_bookIndex].size += order_info.size;
									break;
								}else if (is_after(cur_block.price, order_info.price, bid)){
#ifdef __DEBUG__
	std::cout<<"DEBUG - NEW - insert to the middle of the chain ";
	std::cout<<std::endl;
#endif
									// miss: already iterate to the one should be behind
									stack_insert_index = get_stack_insert_index();
									book[last_bookIndex].next = stack_insert_index;
									chain_new.next = cur_bookIndex;
									book[stack_insert_index] = chain_new;
									break;
								}
							}
							last_bookIndex = cur_bookIndex; 
							cur_bookIndex = cur_block.next;
							cur_block = book[cur_bookIndex];
						}
					// the price should be in front of the head of the chain
					}else{				// the coming price better than the one in book(meaning no orders in this price level), replace it and put it into the stack and connect the link to it;
#ifdef __DEBUG__
	std::cout<<"DEBUG - NEW - insert to the front of the chain head ";
	std::cout<<std::endl;
#endif
						stack_insert_index = get_stack_insert_index();
						book[stack_insert_index] = chain_head;
						chain_new.next = stack_insert_index;
						book[bookIndex] = chain_new; 
					}
				}
			}
		}
		else if (direction == CHANGE)
		{
			price_depth_chain cur_block = book[bookIndex];
			link_t cur_bookIndex = bookIndex;
			link_t last_bookIndex = INVALID_LINK;
			int i_block;
			// search block with the same price
			BOOK_CHANGE_SEARCH_LOC:
			for (i_block=0; i_block<SLOTSIZE; i_block++){
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN
// #ifdef __DEBUG__
// 		std::cout<<"DEBUG -";
// 		std::cout<<" "<<cur_block.price;
// 		std::cout<<std::endl;
// #endif
				if (cur_block.price == order_info.price){
#ifdef __DEBUG__
		std::cout<<"DEBUG - ";
		std::cout<<"CHANGE - hit price "<<cur_block.price;
		std::cout<<" origin size "<<cur_block.size;
		std::cout<<" order size "<<order_info.size;
		std::cout<<" at index "<<cur_bookIndex;
		std::cout<<std::endl;
#endif
					cur_block.size += order_info.size;
// #ifdef __DEBUG__
// 	std::cout<<"DEBUG -";
// 	std::cout<<" "<<cur_block.size;
// 	std::cout<<" "<<order_info.size;
// 	std::cout<<std::endl;
// #endif
					break;
				}
				last_bookIndex = cur_bookIndex;
				cur_bookIndex = cur_block.next;
#ifdef __SYNTHESIS__
				cur_block = book[cur_block.next];
#else
				if (cur_block.next != INVALID_LINK)
					cur_block = book[cur_block.next];
				else
					printf("Price not found in the book!!!!");  // TODO
#endif
			}
			// if block is clear, delete from the chain
			if (cur_block.size <= 0){
#ifdef __DEBUG__
		std::cout<<"DEBUG - CHANGE - remove empty block ";
#endif
				if (i_block == 0) {		// the block is the first block, meaning the chain will be empty
#ifdef __DEBUG__
		std::cout<<"at the head of the chain ";
#endif
					if (cur_block.next == INVALID_LINK){	// the only block on chain, set price to 0 indicating no chain is available
#ifdef __DEBUG__
		std::cout<<"& remove the empty chain ";
#endif
						book[bookIndex].price = 0;
						if (bookIndex == base_bookIndex[bid_ask]){	// if the chain is also at the top of the book, need to update base index and optimal price
#ifdef __DEBUG__
		std::cout<<"& search new optimal backward ";
#endif
							update_optimal(bid_ask, book);
						}
					}
					else{									// other blocks behind, put the next block in to the book, delete its original block place in stack
						store_stack_hole(cur_block.next);
						book[cur_bookIndex] = book[cur_block.next];
					}
#ifdef __DEBUG__
		std::cout<<std::endl;
#endif
				}
				else{					// block is not the head of the chain, change links
#ifdef __DEBUG__
		std::cout<<"in the middle or tail of the chain ";
		std::cout<<std::endl;
#endif
					store_stack_hole(cur_bookIndex);	// mark down the location of the hole
					book[last_bookIndex].next = cur_block.next;
				}
			}
			else{
				book[cur_bookIndex] = cur_block;
			}
	//*
		}
		else if (direction == REMOVE)  // remove the entire level not just the order
		{
			price_depth_chain cur_block = book[bookIndex];
			link_t cur_bookIndex = bookIndex;
			link_t last_bookIndex = INVALID_LINK;
			int i_block;
			// search block with the same price
			BOOK_REMOVE_SEARCH_LOC:
			for (i_block=0; i_block<SLOTSIZE; i_block++){
#pragma HLS PIPELINE off
#pragma HLS LOOP_FLATTEN
				if (cur_block.price == order_info.price)
#ifdef __DEBUG__
		std::cout<<"DEBUG - ";
		std::cout<<"REMOVE - hit price "<<cur_block.price;
		std::cout<<" origin size "<<cur_block.size;
		std::cout<<" order size "<<order_info.size;
		std::cout<<" at index "<<cur_bookIndex;
		std::cout<<std::endl;
#endif
					break;
				
				last_bookIndex = cur_bookIndex;
				cur_bookIndex = cur_block.next;
#ifdef __SYNTHESIS__
				cur_block = book[cur_block.next];
#else
				if (cur_block.next != INVALID_LINK)
					cur_block = book[cur_block.next];
				else
					printf("Price not found in the book!!!!");  // TODO
#endif
			}

#ifdef __DEBUG__
		std::cout<<"DEBUG - REMOVE - remove block ";
#endif
			// delete from the chain
			if (i_block == 0) {		// the block is the first block, meaning the chain will be empty
#ifdef __DEBUG__
		std::cout<<"at the head of the chain ";
#endif
				if (cur_block.next == INVALID_LINK){	// the only block on chain, set price to 0 indicating no chain is available
#ifdef __DEBUG__
		std::cout<<"& remove the empty chain ";
#endif
					book[bookIndex].price = 0;
					if (bookIndex == base_bookIndex[bid_ask]){	// if the chain is also at the top of the book, need to update base index and optimal price
#ifdef __DEBUG__
		std::cout<<"& search new optimal backward ";
#endif
						update_optimal(bid_ask, book);
					}
				}
				else{									// other blocks behind, put the next block in to the book, delete its original block place in stack
					store_stack_hole(cur_block.next);
					book[cur_bookIndex] = book[cur_block.next];
				}
#ifdef __DEBUG__
std::cout<<std::endl;
#endif
			}
			else{					// block is not the head of the chain, change links
#ifdef __DEBUG__
		std::cout<<"in the middle or tail of the chain ";
		std::cout<<std::endl;
#endif
				store_stack_hole(cur_bookIndex);	// mark down the location of the hole
				book[last_bookIndex].next = cur_block.next;
			}
//*/
		}
// print hole fifo
#ifdef __DEBUG__
std::cout<<"DEBUG - {hole_fifo}:";
std::cout<<" stack top: "<<stack_top;
// std::cout<<" head: "<<hole_fifo_head;
// std::cout<<" tail: "<<hole_fifo_tail;
// std::cout<<" content: ";
// int fifo_ptr = hole_fifo_tail;
// int length = (hole_fifo_head> hole_fifo_tail)? hole_fifo_head-hole_fifo_tail: hole_fifo_head-hole_fifo_tail+CHAIN_LEVELS;
// for (int ii=hole_fifo_tail; ii<hole_fifo_tail+length; ii++){
// 	fifo_ptr = (ii>=CHAIN_LEVELS)? ii-CHAIN_LEVELS: ii;
// 	std::cout<<" "<<hole_fifo[fifo_ptr];
// }
std::cout<<std::endl;
#endif
	}
#ifdef __DEBUG__
	std::cout<< "DEBUG - price "<<order_info.price;
	std::cout<<" size "<<order_info.size;
	std::cout<<" index_offset "<<get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
	std::cout<<" index "<<bookIndex; 
	std::cout<<" ask: "<<optimal_prices[0]<<" - "<<base_bookIndex[0];
	std::cout<<" bid: "<<optimal_prices[1]<<" - "<<base_bookIndex[1];
	std::cout<<std::endl;
#endif
}

template <int RANGE, int CHAIN_LEVELS>
void SubOrderBook<RANGE, CHAIN_LEVELS>::book_maintain(
	orderMessage order_message,
	price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS]
){

	// if (update_en){
	addr_index bookIndex;
	order order_info;
	ap_uint<1> bid;
	orderOp direction;		// new change remove

	order_info = order_message.order_info;
	bid = order_message.side;
	direction = order_message.operation;

	bookIndex = get_maintain_bookIndex(
			order_info,
			bid,
			direction,		// new change remove
			book
		);

	update_book(
			order_info,
			bid,
			direction,		// new change remove

			bookIndex,
			book
		);
	
	// }
}
