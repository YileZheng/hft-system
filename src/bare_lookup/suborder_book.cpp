#define __gmp_const const
#include <gmp.h>
#include <mpfr.h>
#include "suborder_book.hpp"


#define LAST_CLOSE 90
#define AS_UNIT 0.01
#define BOOK_SIZE (LAST_CLOSE/10/AS_UNIT*2)


void dut_suborder_book(
	orderMessage order_message,
	ap_uint<8> read_max,
	ap_uint<1> req_read_in,
	hls::stream<price_depth> &feed_stream_out
){
    static price_depth book[BOOK_SIZE];
    static addr_index topbid_bookIndex = 0;
    static price_t topbid_price = 0;
    static price_t UNIT = AS_UNIT;
    static price_t base_price = LAST_CLOSE;

    order order_info = order_message.order_info;

    // calculate location in the book
    addr_index bookIndex = hls::round((order_info.price - base_price) / UNIT);
    bookIndex = (bookIndex < 0)? BOOK_SIZE+bookIndex : bookIndex;

    // new base price
    if ((order_info.price > topbid_price)
        && (order_message.side == )){
        topbid_price = order_info.price;
    }

    // Update book
    price_depth cur_block = book[bookIndex];
    if (order_message.operation == REMOVE){
        if (cur_block.price == order_info.price){
            book[bookIndex].price = 0;
            if (cur_block.price == topbid_price){
                addr_index tmp_pointer = topbid_bookIndex-1;
                REMOVE_UPDATE_BASE:
                for (int i=0; i<BOOK_SIZE; i++){
                    price_depth iter_block = book[tmp_pointer];
                    if (iter_block.price != 0){
                        topbid_price = iter_block.price;
                        topbid_bookIndex = tmp_pointer;
                        break;
                    }
                    if (--tmp_pointer < 0){
                        tmp_pointer += BOOK_SIZE;
                    }
                }
            }
        }
    }
    else if (order_message.operation == CHANGE){
        if (cur_block.price == order_info.price){
            cur_block.size += order_info.size;
            if (cur_block.size == 0){
                // update bid top
                if (cur_block.price == topbid_price){
                    addr_index tmp_pointer = topbid_bookIndex-1;
                    CHANGE_UPDATE_BASE:
                    for (int i=0; i<BOOK_SIZE; i++){
                        price_depth iter_block = book[tmp_pointer];
                        if (iter_block.price != 0){
                            topbid_price = iter_block.price;
                            topbid_bookIndex = tmp_pointer;
                            break;
                        }
                        if (--tmp_pointer < 0){
                            tmp_pointer += BOOK_SIZE;
                        }
                    }
                }

                cur_block.price = 0;
            }
            book[bookIndex].size = cur_block;
        }
    }else if (order_message.operation == NEW){
        book[bookIndex] = {order_info.price, order_info.size};
    }

    // read book
    if (req_read_in){
        // read_book(book, feed_stream_out);
        read_book(book, topbid_bookIndex, read_max, feed_stream_out);
    }

}

// /*
void read_book(
    price_depth &book[BOOK_SIZE],
    addr_index topbid_bookIndex,
	ap_uint<8> read_max,
	hls::stream<price_depth> &feed_stream_out
){
    for (int side=0; side<2; side++){
        int read_cnt = read_max;
        for (int i=0; i<uint(BOOK_SIZE)>>1; i++){
            addr_index index;
            if (side == 1){
                index = topbid_bookIndex + i;
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
    }
}
// */
// /*
void read_book(
    price_depth &book[BOOK_SIZE],
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