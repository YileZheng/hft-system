#include "order_book.hpp"



struct symbol_map
{
	ap_uint<48> symbol;         // string of length 6 
	ap_uint<12> bookIndex;		// stock's correponding index in the order book, up to 4096 stocks
};

struct decoded_message {
	ap_uint<48> symbol;		/*Stock symbol of maximum 6 characters */
	ap_uint<7> posLevel;	/*Position of price level up to 128 levels */
	ap_uint<12> posInLevel; /*Position in the price level, up to 4096 capacity per price level */
    ap_ufixed<28, 8> price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    ap_uint<8> size;        /*Order size in hundreds*/
    ap_uint<32> orderID;    /*Unique ID for each order*/
    ap_uint<3> direction;   /*Order type: 0 - CHANGE ASK 	1 - CHANGE BID   */
};                          /*			  2 - INCOMING ASK 	3 - INCOMING BID */
                            /*		   	  4 - REMOVE ASK	5 - REMOVE BID	 */


struct price_depth{
    ap_ufixed<28, 8> price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    ap_uint<8> size;        /*Order size in hundreds*/
	};


struct order_depth{
    ap_ufixed<28, 8> price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    ap_uint<8> size;        /*Order size in hundreds*/
    ap_uint<32> orderID;    /*Unique ID for each order*/
	};

typedef order_depth top_of_order;



void order_book_system(
		stream<decoded_message> &messages_stream,
		stream<Time> &incoming_time,
		stream<metadata> &incoming_meta,
		stream<Time> &outgoing_time,
		stream<metadata> &outgoing_meta,
		stream<order> &top_bid,
		stream<order> &top_ask,
		ap_uint<32> &top_bid_id,
		ap_uint<32> &top_ask_id)
{
	static order book[STOCKS][LEVELS][CAPACITY];
}