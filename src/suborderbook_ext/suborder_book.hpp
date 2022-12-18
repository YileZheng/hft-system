
#ifndef __SUBORDER_BOOK_H__
#define __SUBORDER_BOOK_H__
//#define __DEBUG__
#include <hls_math.h>
#include <hls_stream.h>
#include "common.hpp"
#include "utils.hpp"
#include "profile.hpp"


void dut_suborder_book(
	orderMessage order_message,
	price_depth_chain book[AS_RANGE*2+AS_CHAIN_LEVELS],
	addr_index &book_head_b,
	addr_index &book_head_a
);


#endif
