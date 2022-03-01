#include "order_book.hpp"
#include "order_book_core.hpp"

// order book for only one instrument
// searching by calculating address index in the book, price depth

// unsigned int fixed_shift_int(
// 	price_t price
// ){
// 	static float scale = 1/UNIT;
// 	return (unsigned int)(price*scale);
// }

addr_index get_bookindex_offset(
	price_t &price_in,
	price_t &price_base
){
	return (addr_index)hls::abs(hls::round((price_in - price_base) / (price_t)(UNIT)));
}

void book_read(
		ap_uint<1> &req_read_in,
		price_depth book[RANGE][2],
		ap_uint<1> book_valid[RANGE][2],
		stream<price_depth> &feed_stream_out
	){
		static ap_uint<1> req_read;
		price_depth dummy;

		req_read = req_read_in;
		if(req_read == 1){
			req_read = 0;
			READ_BOOK:
			for(int side=0; side<=1; side++){
				for (int i=0; i<RANGE; i++){
					if(book_valid[i][side] == 1) feed_stream_out.write(book[i][side]);
				}
				feed_stream_out.write(dummy);
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
	
	static price_depth book[RANGE][2];		// 0 bid 1 ask
	static ap_uint<1> book_valid[RANGE][2];

	static price_t optimal_prices[2];
	static addr_index base_bookIndex[2] = {0, 0};
	addr_index base_bookIndex_tmp;

	// side
	unsigned int bid_ask = ~bid;

	// read process
	book_read(req_read_in, book, book_valid, feed_stream_out); 
	
	// base index update
		// first price write back
	if (optimal_prices[bid_ask] == 0) optimal_prices[bid_ask] = order_info.price;
		// when more optimal price comes in
	
	ap_uint<1> is_better_price = bid? (order_info.price>optimal_prices[bid_ask]):(order_info.price<optimal_prices[bid_ask]); 
	if (is_better_price){
		optimal_prices[bid_ask] = order_info.price;
		cout<<(addr_index)hls::abs(hls::round((order_info.price-optimal_prices[bid_ask]) / (price_t)(UNIT)))<<endl;
		base_bookIndex_tmp = base_bookIndex[bid_ask] - get_bookindex_offset(order_info.price, optimal_prices[bid_ask]);
		// empty least optimal price levels
		for (int i=base_bookIndex_tmp; i<base_bookIndex[bid_ask]; i++){
			int i_spare = i<0? i+RANGE: i;  // TODO: what if i_spare still negetive
			book_valid[i_spare][bid_ask] = 0;
			// TODO: may also put into stack memory
		}
		base_bookIndex[bid_ask] =(base_bookIndex_tmp<0)? base_bookIndex_tmp+RANGE: base_bookIndex_tmp;  // TODO: still <0 ?
	}

	// calculate index
	addr_index bookIndex = base_bookIndex[bid_ask] + get_bookindex_offset(order_info.price, optimal_prices[bid_ask]);
	
#ifndef __SYNTHESIS__
	std::cout<< "DEBUG - price "<<order_info.price<<" size "<<order_info.size<<" index_offset "<<(addr_index)hls::abs((order_info.price-optimal_prices[bid_ask])/(price_t)(UNIT))<<" index "; 
	std::cout<<bookIndex<<" bid: "<<optimal_prices[0]<<"-"<<base_bookIndex[0]<<" ask: "<<optimal_prices[1]<<"-"<<base_bookIndex[1]<<std::endl;
#endif

	// book update
	if (bookIndex < base_bookIndex[bid_ask]+RANGE){
		bookIndex = (bookIndex>=RANGE)? bookIndex-RANGE: bookIndex;
		if (direction == NEW){
			book[bookIndex][bid_ask].price = order_info.price;
			if (book_valid[bookIndex][bid_ask] == 1)
				book[bookIndex][bid_ask].size += order_info.size; 
			else{
				book[bookIndex][bid_ask].size = order_info.size;
				book_valid[bookIndex][bid_ask] = 1;
			}
		}
		else if (direction == CHANGE)
		{
			price_depth price_info_tmp = book[bookIndex][bid_ask];
			if (price_info_tmp.price == order_info.price)
				price_info_tmp.size += order_info.size; 
			if (price_info_tmp.size <= 0){
				book_valid[bookIndex][bid_ask] = 0;
				// update base index
				if (bookIndex == base_bookIndex[bid_ask]){
					SEARCH_NEXT_OPTIMAL_CHANGE:
					for (int i=0; i<RANGE; i++){
						unsigned int bookIndex_tmp = base_bookIndex[bid_ask]+i;
						bookIndex_tmp =  bookIndex_tmp> RANGE? bookIndex_tmp-RANGE: bookIndex_tmp;
						if (book_valid[bookIndex_tmp][bid_ask] == 1){
							base_bookIndex[bid_ask] = bookIndex_tmp;
							optimal_prices[bid_ask] = book[bookIndex_tmp][bid_ask].price;
							break;
						}
					}
				}
			}
			book[bookIndex][bid_ask] = price_info_tmp;
		}
		else if (direction == REMOVE)
		{
			price_depth price_info_tmp = book[bookIndex][bid_ask];
			if (price_info_tmp.price == order_info.price)
				book_valid[bookIndex][bid_ask] = 0; 
			
			// update base index
			if (bookIndex == base_bookIndex[bid_ask]){
				SEARCH_NEXT_OPTIMAL_REMOVE:
				for (int i=0; i<RANGE; i++){
					unsigned int bookIndex_tmp = base_bookIndex[bid_ask]+i;
					bookIndex_tmp =  bookIndex_tmp> RANGE? bookIndex_tmp-RANGE: bookIndex_tmp;
					if (book_valid[bookIndex_tmp][bid_ask] == 1){
						base_bookIndex[bid_ask] = bookIndex_tmp;
						optimal_prices[bid_ask] = book[bookIndex_tmp][bid_ask].price;
						break;
					}
				}
			}
		}
	}
}