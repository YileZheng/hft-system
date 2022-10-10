//#define __gmp_const const
//#include <gmp.h>
//#include <mpfr.h>
#include "suborder_book.hpp"
// #define __DEBUG__


void dut_suborder_book(
	orderMessage order_message,
	ap_uint<8> read_max,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
){
    static price_depth book[BOOK_SIZE] = {0};
    static addr_index topbid_bookIndex = 0;
    static price_t topbid_price = 0;
    static price_t UNIT = AS_UNIT;
    static price_t base_price = LAST_CLOSE;

    order order_info = order_message.order_info;

    // read book
    if (req_read_in){
        // read_book(book, feed_stream_out);
        read_book(book, topbid_bookIndex, read_max, feed_stream_out);
    }

    // calculate location in the book
    addr_index bookIndex = hls::round((order_info.price - base_price) / UNIT);
    bookIndex = (bookIndex < 0)? BOOK_SIZE+bookIndex : bookIndex;

    // new base price
    if ((order_info.price > topbid_price)
        && (order_message.side == 1)){
        topbid_price = order_info.price;
        topbid_bookIndex = bookIndex;
#ifdef __DEBUG__        
        std::cout << "[OrderBook] - update_optimal:more optimal - Better bid price: " << topbid_price << " at " << topbid_bookIndex << std::endl;
#endif
    }

    // Update book
    price_depth cur_block = book[bookIndex];
    if (order_message.operation == REMOVE){
#ifdef __DEBUG__        
        std::cout << "[OrderBook] - update_book:REMOVE - branch to REMOVE" << std::endl;
#endif
        if (cur_block.price == order_info.price){
            book[bookIndex].price = 0;
            book[bookIndex].size = 0;
            if (cur_block.price == topbid_price){
                addr_index tmp_index = (topbid_bookIndex==0)? BOOK_SIZE-1: topbid_bookIndex - 1;
                // addr_index tmp_index = topbid_bookIndex;
                // tmp_index = tmp_index - 1;
                price_depth iter_block = book[tmp_index];
                REMOVE_UPDATE_BASE:
                for (int i=0; i<BOOK_SIZE; i++){
                    if (iter_block.price != 0){
                        topbid_price = iter_block.price;
                        topbid_bookIndex = tmp_index;
#ifdef __DEBUG__        
                        std::cout << "[OrderBook] - update_optimal:lower optimal - removed bid optimal; search for new optimal: " << topbid_price << " at " << topbid_bookIndex << std::endl;
#endif
                        break;
                    }
                    if (tmp_index == 0){
                        tmp_index = BOOK_SIZE-1;
                    }
                    else
                        tmp_index--;
                	iter_block = book[tmp_index];
                }
            }
        }
    }
    else if (order_message.operation == CHANGE){
#ifdef __DEBUG__        
        std::cout << "[OrderBook] - update_book:CHANGE - branch to CHANGE" << std::endl;
#endif
        if (cur_block.price == order_info.price){
            cur_block.size += order_info.size;
            // addr_index tmp_index_1;
            if (cur_block.size == 0){
                if (cur_block.price == topbid_price){
                    // tmp_index_1 = topbid_bookIndex; // - 1;
                    addr_index tmp_index_1 = (topbid_bookIndex==0)? BOOK_SIZE-1: topbid_bookIndex - 1;
                    price_depth iter_block_1 = book[tmp_index_1];
                    CHANGE_UPDATE_BASE:
                    for (int i=0; i<BOOK_SIZE; i++){
                        if (iter_block_1.price != 0){
                            topbid_price = iter_block_1.price;
                            topbid_bookIndex = tmp_index_1;
#ifdef __DEBUG__        
                            std::cout << "[OrderBook] - update_optimal:lower optimal - removed bid optimal; search for new optimal: " << topbid_price << " at " << topbid_bookIndex << std::endl;
#endif
                            break;
                        }
                        if (tmp_index_1 == 0){
                            tmp_index_1 = BOOK_SIZE-1;
                        }
                        else
                            tmp_index_1--;
                        iter_block_1 = book[tmp_index_1];
                    }
//                    addr_index tmp_index = topbid_bookIndex-1;
//                    price_depth iter_block = book[tmp_index];
//                    CHANGE_UPDATE_BASE:
//                    for (int i=0; i<BOOK_SIZE; i++){
//                        if (iter_block.price != 0){
//                            topbid_price = iter_block.price;
//                            topbid_bookIndex = tmp_index;
//                            break;
//                        }
//                        if (tmp_index == 0){
//                            tmp_index = BOOK_SIZE-1;
//                        }
//                        else{
//                        	tmp_index--;
//                        }
//                    	iter_block = book[tmp_index];
//                    }
               }
                // update bid top
            //    if (cur_block.price == topbid_price){
            //        addr_index tmp_index = topbid_bookIndex-1;
            //        CHANGE_UPDATE_BASE:
            //        for (int i=0; i<BOOK_SIZE; i++){
            //            price_depth iter_block = book[tmp_index];
            //            if (iter_block.price != 0){
            //                topbid_price = iter_block.price;
            //                topbid_bookIndex = tmp_index;
            //                break;
            //            }
            //            if (--tmp_index < 0){
            //                tmp_index += BOOK_SIZE;
            //            }
            //        }
            //    }

                cur_block.price = 0;
            }
            book[bookIndex] = cur_block;
        }
    }else if (order_message.operation == NEW){
#ifdef __DEBUG__        
        std::cout << "[OrderBook] - update_book:NEW - branch to NEW" << std::endl;
#endif
//        book[bookIndex] = {order_info.price, order_info.size};
    	book[bookIndex].price = order_info.price;
        book[bookIndex].size += order_info.size;
    }

}

// /*
void read_book(
    price_depth book[BOOK_SIZE],
    addr_index topbid_bookIndex,
	ap_uint<8> read_max,
	hls::stream<price_depth> &feed_stream_out
){
	price_depth dummy = {0,-1};
	READ_BOOK_SIDE:
    for (int side=0; side<2; side++){
        int read_cnt = read_max;
		addr_index index = topbid_bookIndex + 1 - side;
        index = (index == BOOK_SIZE)? 0: index;
		price_depth cur_block = book[index];
        READ_BOOK_SLOT:
        for (int i=0; i<(unsigned int)(BOOK_SIZE)>>1; i++){
            if (side == 0){
            	index++;
//                index = topbid_bookIndex + i + 1;
                index = (index>=BOOK_SIZE)? index-BOOK_SIZE : index;
            }else{
            	index--;
//            	index = topbid_bookIndex - i;
                index = (index<0)? index+BOOK_SIZE : index;
            }
            
            if (cur_block.price != 0){
                feed_stream_out.write(cur_block);
                if (read_cnt-- == 1) break;
            }
            cur_block = book[index];
        }
        feed_stream_out.write(dummy);
    }
}
// */
 /*
void read_book(
    price_depth book[BOOK_SIZE],
	hls::stream<price_depth> &feed_stream_out
){
    READ_BOOK:
    for (int i=0; i<BOOK_SIZE; i++){
        if (book[i].price != 0){
            feed_stream_out.write(book[i]);
        }
    }
}

// */
