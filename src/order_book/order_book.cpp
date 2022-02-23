#include "order_book.hpp"

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
			last_empty = true
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

void suborder_book(){

}

void order_book_system(
		stream<decoded_message> &messages_stream,
		stream<Time> &incoming_time,
		stream<metadata> &incoming_meta,
		stream<Time> &outgoing_time,
		stream<metadata> &outgoing_meta,
		stream<order> &top_bid,
		stream<order> &top_ask,
		orderID_t &top_bid_id,
		orderID_t &top_ask_id)
{
	static sub_order book[LEVELS*STOCKS][2*CAPACITY];
	static price_lookup price_map[LEVELS*STOCKS][2];
	static symbol_t symbol_list[STOCKS];         // string of length 6

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