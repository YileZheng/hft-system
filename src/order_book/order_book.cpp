#include "order_book.hpp"
#include "suborder_book.hpp"


#define __DEBUG__

void order_book_system(
	// data
	hls::stream<Message> &stream_in,
	hls::stream<price_depth> &stream_out,

	// configuration inputs
	symbol_t axi_symbol_map[STOCKS],
	symbol_t axi_read_symbol,
	ap_uint<8> axi_read_max,

	// control input
	char axi_instruction  // void, run, halt, read book, clear, config symbol map | read_max 

){
#pragma HLS INTERFACE axis register_mode=both register port=stream_in
#pragma HLS INTERFACE axis register_mode=both register port=stream_out
#pragma HLS INTERFACE s_axilite port=axi_instruction bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=axi_read_symbol bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=axi_read_max bundle=BUS_A
#pragma HLS INTERFACE s_axilite port=return bundle=BUS_A

	
	static SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS]={{AS_SLOTSIZE, AS_UNIT}};  // TODO
	static symbol_t symbol_map[STOCKS];
	static ap_uint<STOCKS> read_req_concat=0;
	static ap_uint<8> read_max=10;
	
	// control signal
	static ap_uint<1> halt=1, en_read=0, en_clear=0; 
	static char instruction = 'v';

	// dataflow
	Message message_in;
	orderMessage ordermessage_in;
	int index_msg, index_read;

	// control
	instruction = axi_instruction;
	switch (instruction)
	{
	case 'v':	// void
		break;
	case 'R':	// run
		halt = 0;
		break;
	case 'H':	// halt
		halt = 1;
		break;
	case 'r':	// read book
		en_read = 1;
		break;
	case 'c':	// clear
		en_clear = 1;
		break;
	case 's':	// update symbol map
		update_symbol_map(axi_symbol_map, symbol_map);
		break;
	case 'm':	// update read max 
		read_max = axi_read_max;
		break;
	case 'A': 	// config all
		update_symbol_map(axi_symbol_map, symbol_map);
		read_max = axi_read_max;

	default:
		break;
	} 
	instruction = 'v';


	// order symbol mapping
	while (!stream_in.empty()){
		message_in = stream_in.read();
		if (!halt){
			ordermessage_in.order_info.orderID = message_in.orderID;
			ordermessage_in.order_info.size = message_in.size;
			ordermessage_in.order_info.price = message_in.price;
			ordermessage_in.operation = message_in.operation;
			ordermessage_in.side = message_in.side;
			index_msg = symbol_mapping(symbol_map, message_in.symbol);
#ifdef __DEBUG__
			if (!(index_msg>=0)&&(index_msg<STOCKS)){
				printf("Input symbol not found in the local symbol map, discard!!!!!!!!");
			}
#endif
			update_subbooks(index_msg, books, ordermessage_in);
		}
	}

	// read symbol mapping
	if (en_read){
		index_read = symbol_mapping(symbol_map, axi_read_symbol);
		read_req_concat = (0x1<<index_read);
		en_read = 0;
	}

	routine_subbooks(read_max, read_req_concat, books, stream_out);

}

void update_symbol_map(
	symbol_t axi_symbol_map[STOCKS],
	symbol_t symbol_map[STOCKS]
){
	for (int i=0; i<STOCKS; i++){
		*(symbol_map+i) = *(axi_symbol_map+i);
	}
}

void controller(

){

}

void routine_subbooks(
	ap_uint<8> read_max,
	ap_uint<STOCKS> &read_req_concat,
	
	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS],

	hls::stream<price_depth> &stream_out
){

	for (int i=0; i<STOCKS; i++){
#pragma HLS UNROLL
		ap_uint<1> read_req = (read_req_concat>>i) & 0x1;
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
#pragma HLS UNROLL
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
#pragma HLS INLINE off
	int index=-1;
	for (int i=0; i<STOCKS; i++){
#pragma HLS UNROLL
		if (symbol_map[i]==symbol){
			index = i;
		}
	}
	return index;
}
