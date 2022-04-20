#include "order_book.hpp"

void order_book_system(
	stream<Message> stream_in,
	stream<price_depth> stream_out,
	symbol_t axi_read_symbol
){
	
	static stream<price_depth> feed_streams[STOCKS];
	static stream<orderMessage> order_streams[STOCKS];
	static symbol_t symbol_map[STOCKS];
	static ap_uint<STOCKS> read_req;
	
	Message message_in;
	orderMessage ordermessage_in;
	int index_msg, index_read;
	// Order book access from master 

	// symbol mapping
	if (!stream_in.empty()){
		message_in = stream_in.read();
		ordermessage_in.order_info.orderID = message_in.orderID;
		ordermessage_in.order_info.size = message_in.size;
		ordermessage_in.order_info.price = message_in.price;
		ordermessage_in.operation = message_in.operation;
		ordermessage_in.side = message_in.side;
		index_msg = symbol_mapping(symbol_map, message_in.symbol);
		order_streams[index_msg].write(ordermessage_in);
	}

	// read target register

	// message relocator

	// 
}

int symbol_mapping(
	symbol_t symbol_map[STOCKS],
	symbol_t symbol
){
	ap_uint<STOCKS> onehot=0;
	int index;
	for (int i=0; i<STOCKS; i++){
		if (symbol_map[i]==symbol){
			onehot |= 1<<i;
			index = i;
		}else{
			onehot |= 0<<i;
			index = i;
		}
	}
	return index;
}