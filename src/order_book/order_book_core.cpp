#include "order_book.hpp"

#define RANGE 1000 		// RANGE level of price to track in order book 
#define UNIT 0.01

// searching by calculating address index in the book, price depth
void suborder_book(
	order order_info,
	orderOp direction,
	bool bid=true

){
	typedef unsigned int addr_index;
	static price_depth book[RANGE][2];
	static ap_uint<1> book_valid[RANGE][2];

	static price_t optimal_prices[2];
	price_t optimal_price;
	static addr_index base_bookIndex = 0;
	addr_index base_bookIndex_tmp;

	unsigned int bid_ask = bid? 0: 1;

	// base index update
	if (order_info.price < optimal_price){
		optimal_price = order_info.price;
		base_bookIndex_tmp = base_bookIndex - (optimal_price - order_info.price)/UNIT;
		// empty least optimal price levels
		for (int i=base_bookIndex_tmp; i<base_bookIndex; i++){
			i_spare = i<0? i+RANGE: i;
			book_valid[i_spare][bid_ask] = 0;
			// TODO: may also put into stack memory
		}
		base_bookIndex = base_bookIndex_tmp < 0? base_bookIndex_tmp+RANGE: base_bookIndex_tmp;  // TODO: still <0 ?

	}

	// calculate index
	addr_index bookIndex = base_bookIndex + (order_info.price - optimal_price) / UNIT;
	

	// book update
	if (bookIndex < base_bookIndex+RANGE){
		bookIndex = bookIndex >= RANGE? bookIndex-RANGE: bookIndex;
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
				price_info_tmp.size -= order_info.size; 
			if (price_info_tmp.size <= 0){
				book_valid[bookIndex][bid_ask] = 0;

				// update base index
				if (bookIndex == base_bookIndex){
					SEARCH_NEXT_OPTIMAL_CHANGE:
					for (int i=0; i<RANGE; i++){
						unsigned int bookIndex_tmp = base_bookIndex+i;
						bookIndex_tmp =  bookIndex_tmp> RANGE? bookIndex_tmp-RANGE: bookIndex_tmp;
						if (book_valid[bookIndex_tmp][bid_ask] == 1){
							base_bookIndex = bookIndex_tmp;
							optimal_price = book[bookIndex_tmp][bid_ask].price;
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
			if (bookIndex == base_bookIndex){
				SEARCH_NEXT_OPTIMAL_REMOVE:
				for (int i=0; i<RANGE; i++){
					unsigned int bookIndex_tmp = base_bookIndex+i;
					bookIndex_tmp =  bookIndex_tmp> RANGE? bookIndex_tmp-RANGE: bookIndex_tmp;
					if (book_valid[bookIndex_tmp][bid_ask] == 1){
						base_bookIndex = bookIndex_tmp;
						optimal_price = book[bookIndex_tmp][bid_ask].price;
						break;
					}
				}
			}
		}
		
		
	}

}