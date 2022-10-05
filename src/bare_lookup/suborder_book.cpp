//c#define __gmp_const const
//#include <gmp.h>
//#include <mpfr.h>
#include "suborder_book.hpp"


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
    }

    // Update book
    price_depth cur_block = book[bookIndex];
    if (order_message.operation == REMOVE){
        if (cur_block.price == order_info.price){
            book[bookIndex].price = 0;
            book[bookIndex].size = 0;
            if (cur_block.price == topbid_price){
                addr_index tmp_index = topbid_bookIndex;
                price_depth iter_block;
                REMOVE_UPDATE_BASE:
                for (int i=0; i<BOOK_SIZE; i++){
                    if (--tmp_index < 0){
                        tmp_index += BOOK_SIZE;
                    }
                	iter_block = book[tmp_index];
                    if (iter_block.price != 0){
                        topbid_price = iter_block.price;
                        topbid_bookIndex = tmp_index;
                        break;
                    }
                }
            }
        }
    }
    else if (order_message.operation == CHANGE){
        if (cur_block.price == order_info.price){
            cur_block.size += order_info.size;
            if (cur_block.size == 0){
                if (cur_block.price == topbid_price){
                    addr_index tmp_index = topbid_bookIndex;
                    price_depth iter_block;
                    CHANGE_UPDATE_BASE:
                    for (int i=0; i<BOOK_SIZE; i++){
                        if (--tmp_index < 0){
                            tmp_index += BOOK_SIZE;
                        }
                    	iter_block = book[tmp_index];
                        if (iter_block.price != 0){
                            topbid_price = iter_block.price;
                            topbid_bookIndex = tmp_index;
                            break;
                        }
                    }
                }
                // update bid top
//                if (cur_block.price == topbid_price){
//                    addr_index tmp_index = topbid_bookIndex-1;
//                    CHANGE_UPDATE_BASE:
//                    for (int i=0; i<BOOK_SIZE; i++){
//                        price_depth iter_block = book[tmp_index];
//                        if (iter_block.price != 0){
//                            topbid_price = iter_block.price;
//                            topbid_bookIndex = tmp_index;
//                            break;
//                        }
//                        if (--tmp_index < 0){
//                            tmp_index += BOOK_SIZE;
//                        }
//                    }
//                }
//
                cur_block.price = 0;
            }
            book[bookIndex] = cur_block;
        }
    }else if (order_message.operation == NEW){
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
        READ_BOOK_SLOT:
        for (int i=0; i<(unsigned int)(BOOK_SIZE)>>1; i++){
            addr_index index;
            if (side == 0){
                index = topbid_bookIndex + i + 1;
                index = (index>=BOOK_SIZE)? index-BOOK_SIZE : index;
            }else{
                index = topbid_bookIndex - i;
                index = (index<0)? index+BOOK_SIZE : index;
            }
            
            price_depth cur_block = book[index];
            if (cur_block.price != 0){
                feed_stream_out.write(cur_block);
                if (--read_cnt == 0) break;
            }
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
