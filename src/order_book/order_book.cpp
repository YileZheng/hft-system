#include "order_book.hpp"

typedef ap_uint<48> symbol_t; 
typedef ap_ufixed<28, 8> price_t; 
typedef ap_uint<8> size_t;        /*Order size in hundreds*/
typedef ap_uint<32> orderID_t;    /*Unique ID for each order*/
    
struct price_lookup
{
	ap_uint<12> orderCnt = 0;
	price_t	price;
};

struct symbol_map
{
	symbol_t symbol;         // string of length 6 
	ap_uint<12> bookIndex;		// stock's correponding index in the order book, up to 4096 stocks
};

struct decoded_message {
	symbol_t symbol;		/*Stock symbol of maximum 6 characters */
	ap_uint<7> posLevel;	/*Position of price level up to 128 levels */
	ap_uint<12> posInLevel; /*Position in the price level, up to 4096 capacity per price level */
    price_t price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    size_t size;        /*Order size in hundreds*/
    orderID_t orderID;    /*Unique ID for each order*/
    ap_uint<3> operation;   /*Order type: 0 - CHANGE ASK 	1 - CHANGE BID   */
};                          /*			  2 - INCOMING ASK 	3 - INCOMING BID */
                            /*		   	  4 - REMOVE ASK	5 - REMOVE BID	 */

struct order
{
    ap_ufixed<28, 8> price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    ap_uint<8> size;        /*Order size in hundreds*/
    ap_uint<32> orderID;    /*Unique ID for each order*/
};    

struct sub_order
{
    ap_uint<8> size;        /*Order size in hundreds*/
    ap_uint<32> orderID;    /*Unique ID for each order*/
};   

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

// make sure there should up to only one empty slot each time execute this function
void table_refresh(
	int ystart,
	int xstart,
	price_lookup price_table[LEVELS*STOCKS][2]
){
	bool last_empty = False;
	price_lookup slot;
	for (int i=0; i < LEVELS; i++){
		slot = price_table[ystart+i][xstart];
		if (last_empty){
			price_table[ystart+i][xstart] = slot;
			last_empty = True
		}else{
			last_empty = slot.orderCnt == 0;
		}
	}
}

// insert a new data in the table and move the following data 1 step backward
void table_insert(
	int &ystart,
	int &xstart,
	price_lookup price_table[LEVELS*STOCKS][2],
	price_lookup price_new,
	int &insert_pos // relative position to the start point
){
	price_lookup bubble = price_new, bubble_tmp;
	for (int i=0; i < LEVELS; i++){
		if (i >= insert_pos && bubble.orderCnt != 0){      // start from the insert_pos and ended at the last valid slot 
			bubble_tmp = price_table[ystart+i][xstart];
			price_table[ystart+i][xstart] = bubble;
			bubble = bubble_tmp;
		}
	}
}

void order_new(
	sub_order book[LEVELS*STOCKS][2*CAPACITY],
	price_lookup price_table[LEVELS*STOCKS][2],
	order order_info,
	ap_uint<12> bookIndex,
	bool bid=true
){
	int book_frame_ystart = bookIndex*LEVELS;
	int book_frame_xstart = bid? 0: CAPACITY;
	int price_table_frame_xstart = bid? 0: 1;

	static int heap_head=0, heap_tail=0;

	// search existing level
	if (bid) {
		for (int i=0; i < LEVELS; i++){
			price_lookup level_socket = price_table[book_frame_ystart+i][price_table_frame_xstart]
			level_socket.orderCnt == 0 
			|| level_socket.price == order_info.price
		}
	}
}
void order_change(bool bid=true){

}
void order_remove(bool bid=true){

}

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
	static sub_order book[LEVELS*STOCKS][2*CAPACITY];
	static price_lookup price_map[LEVELS*STOCKS][2];
	static ap_uint<48> symbol_list[STOCKS];         // string of length 6

	decoded_message msg_inbound = messages_stream.read();

	order order_inbound;
	order_inbound.price = msg_inbound.price;
	order_inbound.size = msg_inbound.size;
	order_inbound.orderID = msg_inbound.orderID;
	symbol_t symbol_inbound = msg_inbound.symbol;
	ap_uint<3> operation = msg_inbound.operation;


	// symbol location searching

	ap_uint<12> bookIndex;		// stock's correponding index in the order book, up to 4096 stocks
	for (int i=0; i<STOCKS; i++){
		if (symbol_inbound == symbol_list[i])
			bookIndex = i;
	}

	// Order book modification according to the operation type
	switch (operation)
	{
	case 0:	// Change ASK
		break;
	case 2:	// New ASK
		break;
	case 4:	// Remove ASK
		break;

	case 1:	// Change BID
		break;
	case 3:	// New BID
		break;
	case 5:	// Remove BID
		break;

	default:
		break;
	}

	// Order book access from master 

}