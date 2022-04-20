#include "order_book.hpp"
#include "order_book_core_IndexPop_opt.hpp"


bool is_after(
	price_t &price_in,
	price_t &price_onchain,
	ap_uint<1> &bid
){
	return bid? price_in<price_onchain: price_in>price_onchain;
}
