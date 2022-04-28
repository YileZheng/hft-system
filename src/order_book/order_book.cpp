#include "order_book.hpp"

#define AS_RANGE 1000
#define AS_CHAIN_LEVELS 200
#define AS_UNIT 0.01
#define AS_SLOTSIZE 10

#include "suborder_book.hpp"

void order_book_system(
	hls::stream<Message> &stream_in,
	hls::stream<price_depth> &stream_out,
	symbol_t axi_read_symbol,
	ap_uint<8> axi_read_max
){
#pragma HLS INTERFACE axis register_mode=both register port=stream_in
#pragma HLS INTERFACE axis register_mode=both register port=stream_out
#pragma HLS INTERFACE s_axilite port=axi_read_symbol bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=axi_read_max bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=return bundle=BUS_A

	
	static symbol_t symbol_map[STOCKS];
	static ap_uint<STOCKS> read_req_concat;
	static SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS];
	// static ap_uint<8> read_max=10;
	
	Message message_in;
	orderMessage ordermessage_in;
	int index_msg, index_read;

	// Order book access from master 


	// order symbol mapping
	if (!stream_in.empty()){
		message_in = stream_in.read();
		ordermessage_in.order_info.orderID = message_in.orderID;
		ordermessage_in.order_info.size = message_in.size;
		ordermessage_in.order_info.price = message_in.price;
		ordermessage_in.operation = message_in.operation;
		ordermessage_in.side = message_in.side;
		index_msg = symbol_mapping(symbol_map, message_in.symbol);
		update_subbooks(index_msg, books, ordermessage_in);
	}

	// read symbol mapping
	if (axi_read_symbol != 0){
		index_read = symbol_mapping(symbol_map, message_in.symbol);
		read_req_concat = (0x1<<index_read);
		axi_read_symbol = 0;
	}

	read_subbooks(axi_read_max, read_req_concat, books, stream_out);


	// read target register


	// 
}


void read_subbooks(
	// int index_read,
	ap_uint<8> read_max,
	ap_uint<STOCKS> &read_req_concat,
	
	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS],

	hls::stream<price_depth> &stream_out
){

	for (int i=0; i<STOCKS; i++){
#pragma HLS unroll
		ap_uint<1> read_req = (read_req_concat>>i & 0x1);
		books[i].subbook_controller(read_req);
		books[i].book_read(read_max, stream_out);		// TODO: parallel streaming channel reusing confliction?
	}
	read_req_concat = 0;
}

void update_subbooks(
	int index_msg,
	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS],

	orderMessage ordermessage_in
){
	// update books on message event
	for (int i=0; i<STOCKS; i++){
#pragma HLS unroll
		if (i == index_msg){
			books[i].book_maintain(ordermessage_in);
		}
	}

}

void init_books(
	int slotsize,
	float unit,
	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS]
){
	for (int i=0; i<STOCKS; i++){
		books[i].config(slotsize, unit);
	}
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