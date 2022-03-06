#include "order_book.hpp"
#include "order_book_core_IndexPop.hpp"

// #define __SYNTHESIS__
// order book for only one instrument
// searching by calculating address index in the book, price depth
// and a linked chain in each indexed slot

addr_index get_bookindex_offset(
	price_t &price_in,
	price_t &price_base,
	ap_uint<1> &bid
){	
	// price_t is unsigned, cannot be negetive
	float price1 = price_in,  price2 = price_base;
	float price_diff = bid? (price2-price1): (price1-price2);
	addr_index offset = hls::abs( hls::floor( hls::round( price_diff/(UNIT)) /SLOTSIZE ));
	// addr_index offset = hls::abs( hls::floor( hls::round(price_diff/(price_t)(UNIT)) /(price_t)SLOTSIZE));
	return offset;
}

bool is_after(
	price_t &price_in,
	price_t &price_onchain,
	ap_uint<1> &bid
){
	return bid? price_in<price_onchain: price_in>price_onchain;
}

void book_read(
	ap_uint<1> &req_read,
	price_depth_chain book[RANGE][2],
	price_depth_chain chain_stack[CHAIN_LEVELS],
	addr_index base_bookIndex[2],
	stream<price_depth> &feed_stream_out
){
	price_depth dummy;
	price_depth lvl_out;
	price_depth_chain *cur_block;
	int ind;

	if(req_read == 1){
		req_read = 0;
		READ_BOOK:
		for(int side=0; side<=1; side++){
			for (int i=0; i<RANGE; i++){
				ind = (base_bookIndex[side]+i>=RANGE)? base_bookIndex[side]+i-RANGE: base_bookIndex[side]+i;
				if(book[ind][side].price != 0){
					cur_block = &book[ind][side];
					for (int j=0; j<SLOTSIZE; j++){
						lvl_out.price = cur_block->price;
						lvl_out.size = cur_block->size;
						feed_stream_out.write(lvl_out);
#ifndef __SYNTHESIS__
	std::cout<<"DEBUG - ";
	std::cout<<" x: "<<ind<<" y: "<<j;
	std::cout<<std::endl;
#endif
						if (cur_block->next != INVALID_LINK)
							cur_block = &chain_stack[cur_block->next];
						else break;
					}
				}
			}
			feed_stream_out.write(dummy);
		}
	}
}

void store_stack_hole(
	link_t hole_fifo[CHAIN_LEVELS],
	int &hole_fifo_head, 
	link_t &stack_top,
	link_t &hole
){
	if (hole == stack_top-1){
		stack_top--;
	}else{
		hole_fifo[hole_fifo_head++] = hole;
		if (hole_fifo_head>=CHAIN_LEVELS) hole_fifo_head = 0;
	}
}

link_t get_stack_insert_index(
	link_t hole_fifo[CHAIN_LEVELS],
	int &hole_fifo_head, 
	int &hole_fifo_tail,
	link_t &stack_top
){
	link_t addr_out;
	if (hole_fifo_head!=hole_fifo_tail){
		addr_out = hole_fifo[hole_fifo_tail++];
		if (hole_fifo_tail>=CHAIN_LEVELS) hole_fifo_tail = 0;
	}else{
		addr_out = stack_top++;		// TODO: how to deal with overflow (default will not overflow)
	}
	return addr_out;
}

void update_optimal(
	price_depth_chain book[RANGE][2],
	price_t optimal_prices[2],
	addr_index base_bookIndex[2],
	unsigned int &bid_ask
){
	unsigned int bookIndex_tmp;
	SEARCH_NEXT_OPTIMAL:
	for (int i=1; i<RANGE; i++){		// search backward in the book starting with orginal base_bookIndex
		bookIndex_tmp = base_bookIndex[bid_ask]+i;
		bookIndex_tmp = (bookIndex_tmp>=RANGE)? bookIndex_tmp-RANGE: bookIndex_tmp;
		if (book[bookIndex_tmp][bid_ask].price != 0){		// find valid slot
			base_bookIndex[bid_ask] = bookIndex_tmp;
			optimal_prices[bid_ask] = (bid_ask)? (price_t)(optimal_prices[bid_ask]-(price_t)(UNIT*SLOTSIZE*i)):
												(price_t)(optimal_prices[bid_ask]+(price_t)(UNIT*SLOTSIZE*i));
			break;
		}
	}
}

void suborder_book(
	order &order_info,		// price size ID
	orderOp &direction,		// new change remove
	ap_uint<1> &bid,
	ap_uint<1> &req_read_in,
	stream<price_depth> &feed_stream_out
){
	
	static price_depth_chain book[RANGE][2];		// 0 bid 1 ask
	static price_depth_chain chain_stack[CHAIN_LEVELS];
	static link_t hole_fifo[CHAIN_LEVELS];
	static int hole_fifo_head = 0;
	static int hole_fifo_tail = 0;
	static link_t stack_top = 0;

	static price_t optimal_prices[2];
	static addr_index base_bookIndex[2] = {0, 0};
	addr_index base_bookIndex_tmp;
	link_t stack_insert_index;

	// side
	unsigned int bid_ask = bid;

	// read process
	book_read(req_read_in, book, chain_stack, base_bookIndex, feed_stream_out); 
	
	// base index update
		// first price write back
	if (optimal_prices[bid_ask] == 0) optimal_prices[bid_ask] = order_info.price;
		// when more optimal price comes in
	
	ap_uint<1> is_better_price = is_after(optimal_prices[bid_ask], order_info.price, bid);
	if (is_better_price){
		base_bookIndex_tmp = base_bookIndex[bid_ask] - get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
#ifndef __SYNTHESIS__
		std::cout<<"DEBUG - {better optimal price} ";
		std::cout<<" base: "<<base_bookIndex[bid_ask];
		std::cout<<" better base:"<<base_bookIndex_tmp;
		std::cout<<" offset from base: "<<get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
		std::cout<<" order price: "<<order_info.price;
		std::cout<<" optimal "<<optimal_prices[bid_ask];
		std::cout<<std::endl;
#endif
		optimal_prices[bid_ask] = (bid)? (price_t)(optimal_prices[bid_ask]+(price_t)(UNIT*SLOTSIZE)): (price_t)(optimal_prices[bid_ask]-(price_t)(UNIT*SLOTSIZE));
#ifndef __SYNTHESIS__
		std::cout<<"DEBUG - {better optimal price} ";
		std::cout<<" base: "<<base_bookIndex[bid_ask];
		std::cout<<" better base:"<<base_bookIndex_tmp;
		std::cout<<" offset from base: "<<get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
		std::cout<<" order price: "<<order_info.price;
		std::cout<<" optimal "<<optimal_prices[bid_ask];
		std::cout<<std::endl;
#endif
		// empty least optimal price levels
		for (int i=base_bookIndex_tmp; i<base_bookIndex[bid_ask]; i++){
			int i_spare = i<0? i+RANGE: i;  // TODO: what if i_spare still negetive
			if (book[i_spare][bid_ask].price != 0){
				book[i_spare][bid_ask].price = 0;
				// record holes obtained from clearing this chain
				price_depth_chain *cur_block = &book[i_spare][bid_ask];
				for (int j=0; j<SLOTSIZE; j++){
					if (cur_block->next != INVALID_LINK)
						store_stack_hole(hole_fifo, hole_fifo_head, stack_top, cur_block->next);
					else break;
					cur_block = &chain_stack[cur_block->next];
				}
			}
			// TODO: may also put into stack memory
		}
		base_bookIndex[bid_ask] =(base_bookIndex_tmp<0)? base_bookIndex_tmp+RANGE: base_bookIndex_tmp;  // TODO: still <0 ?
	}

	// calculate index
	addr_index bookIndex = base_bookIndex[bid_ask] + get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
	
#ifndef __SYNTHESIS__
	std::cout<< "DEBUG - price "<<order_info.price;
	std::cout<<" size "<<order_info.size;
	std::cout<<" index_offset "<<get_bookindex_offset(order_info.price, optimal_prices[bid_ask], bid);
	std::cout<<" index "<<bookIndex; 
	std::cout<<" ask: "<<optimal_prices[0]<<" - "<<base_bookIndex[0];
	std::cout<<" bid: "<<optimal_prices[1]<<" - "<<base_bookIndex[1];
	std::cout<<std::endl;
#endif

	// book update
	if (bookIndex < base_bookIndex[bid_ask]+RANGE){
		bookIndex = (bookIndex>=RANGE)? bookIndex-RANGE: bookIndex;
		if (direction == NEW){
			// when there is no valid price level in this slot
			if (book[bookIndex][bid_ask].price == 0){
				book[bookIndex][bid_ask].price = order_info.price;
				book[bookIndex][bid_ask].size = order_info.size;
				book[bookIndex][bid_ask].next = INVALID_LINK;
			// when slot valid
			}else{
				// if hit at the link head
				if (book[bookIndex][bid_ask].price == order_info.price)
					book[bookIndex][bid_ask].size += order_info.size; 
				else{
					stack_insert_index = get_stack_insert_index(hole_fifo, hole_fifo_head, hole_fifo_tail, stack_top);
					// when the price should be located somewhere after this block on this chain, 
					// search the chain: 
					// price hit: add up size; 
					// price miss: link a new block (1. already iterate to the price should be after this one; 2. run to the end of the chain)
					if (is_after(order_info.price, book[bookIndex][bid_ask].price, bid)){
						link_t next_blockIndex = book[bookIndex][bid_ask].next;
						price_depth_chain *last_block = &book[bookIndex][bid_ask];
						for (int i=0; i<SLOTSIZE; i++){
							// miss: run to the end
							if (next_blockIndex == INVALID_LINK){
								last_block->next = stack_insert_index;
								chain_stack[stack_insert_index].next = INVALID_LINK;
								chain_stack[stack_insert_index].price = order_info.price;
								chain_stack[stack_insert_index].size = order_info.size;
								break;
							// hit
							}else if (chain_stack[next_blockIndex].price == order_info.price){
								chain_stack[stack_insert_index].size += order_info.size;
								break;
							// miss: already iterate to the one should be behind
							}else if (is_after(chain_stack[next_blockIndex].price, order_info.price, bid)){
								last_block->next = stack_insert_index;
								chain_stack[stack_insert_index].next = next_blockIndex;
								chain_stack[stack_insert_index].price = order_info.price;
								chain_stack[stack_insert_index].size = order_info.size;
								break;
							}
							last_block = &chain_stack[next_blockIndex];
							next_blockIndex = chain_stack[next_blockIndex].next;
						}
					// the price should be in front of the head of the chain
					}else{				// the coming price better than the one in book(meaning no orders in this price level), replace it and put it into the stack and connect the link to it;
						chain_stack[stack_insert_index] = book[bookIndex][bid_ask];
						book[bookIndex][bid_ask].next = stack_insert_index;
						book[bookIndex][bid_ask].price = order_info.price;
						book[bookIndex][bid_ask].size = order_info.size;
					}
				}
			}
		}
		else if (direction == CHANGE)
		{
			price_depth_chain *last_block, *cur_block = &book[bookIndex][bid_ask];
			int i_block;
			// search block with the same price
			for (i_block=0; i_block<SLOTSIZE; i_block++){
#ifndef __SYNTHESIS__
		std::cout<<"DEBUG -";
		std::cout<<" "<<cur_block->price;
		std::cout<<std::endl;
#endif
				if (cur_block->price == order_info.price){
#ifndef __SYNTHESIS__
		std::cout<<"DEBUG -";
		std::cout<<" "<<cur_block->size;
		std::cout<<" "<<order_info.size;
		std::cout<<std::endl;
#endif
					cur_block->size += order_info.size;
#ifndef __SYNTHESIS__
	std::cout<<"DEBUG -";
	std::cout<<" "<<cur_block->size;
	std::cout<<" "<<order_info.size;
	std::cout<<std::endl;
#endif
					break;
				}
				last_block = cur_block;
				cur_block = &chain_stack[cur_block->next];
			}
			// if block is clear, delete from the chain
			if (cur_block->size <= 0){
				if (i_block == 0) {		// the block is the first block, meaning the chain will be empty
					if (cur_block->next == INVALID_LINK){	// the only block on chain, set price to 0 indicating no chain is available
						cur_block->price = 0;
						if (bookIndex == base_bookIndex[bid_ask])	// if the chain is also at the top of the book, need to update base index and optimal price
							update_optimal(book, optimal_prices, base_bookIndex, bid_ask);
					}
					else{									// other blocks behind, put the next block in to the book, delete its original block place in stack
						store_stack_hole(hole_fifo, hole_fifo_head, stack_top, cur_block->next);
						*cur_block = chain_stack[cur_block->next];
					}
				}else{					// block is not the head of the chain, change links
					store_stack_hole(hole_fifo, hole_fifo_head, stack_top, last_block->next);	// mark down the location of the hole
					last_block->next = cur_block->next;
				}
			}
		}
		else if (direction == REMOVE)  // remove the entire level not just the order
		{
			price_depth_chain *last_block, *cur_block = &book[bookIndex][bid_ask];
			int i_block;
			// search block with the same price
			for (i_block=0; i_block<SLOTSIZE; i_block++){
#ifndef __SYNTHESIS__
	std::cout<<"DEBUG -";
	std::cout<<" "<<cur_block->price;
	std::cout<<std::endl;
#endif
				if (cur_block->price == order_info.price){
//					cur_block->size = 0;
					break;
				}
				last_block = cur_block;
				cur_block = &chain_stack[cur_block->next];
			}
			// delete from the chain
			if (i_block == 0) {		// the block is the first block, meaning the chain will be empty
				if (cur_block->next == INVALID_LINK){	// the only block on chain, set price to 0 indicating no chain is available
					cur_block->price = 0;
					if (bookIndex == base_bookIndex[bid_ask])	// if the chain is also at the top of the book, need to update base index and optimal price
						update_optimal(book, optimal_prices, base_bookIndex, bid_ask);
				}
				else{									// other blocks behind, put the next block in to the book, delete its original block place in stack
					store_stack_hole(hole_fifo, hole_fifo_head, stack_top, cur_block->next);
					*cur_block = chain_stack[cur_block->next];
				}
			}else{					// block is not the head of the chain, change links
				store_stack_hole(hole_fifo, hole_fifo_head, stack_top, last_block->next);	// mark down the location of the hole
				last_block->next = cur_block->next;
			}
		}
// print hole fifo
#ifndef __SYNTHESIS__
std::cout<<"DEBUG - {hole_fifo}:";
std::cout<<" stack top: "<<stack_top;
std::cout<<" head: "<<hole_fifo_head;
std::cout<<" tail: "<<hole_fifo_tail;
std::cout<<" content: ";
int fifo_ptr = hole_fifo_tail;
int length = (hole_fifo_head> hole_fifo_tail)? hole_fifo_head-hole_fifo_tail: hole_fifo_head-hole_fifo_tail+CHAIN_LEVELS;
for (int ii=hole_fifo_tail; ii<hole_fifo_tail+length; ii++){
	fifo_ptr = (ii>=CHAIN_LEVELS)? ii-CHAIN_LEVELS: ii;
	std::cout<<" "<<hole_fifo[fifo_ptr];
}
std::cout<<std::endl;
#endif
	}
}
