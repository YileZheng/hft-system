#include <hls_math.h>
#include "order_book.hpp"


bool is_after(
	price_t &price_in,
	price_t &price_onchain,
	ap_uint<1> &bid
);
