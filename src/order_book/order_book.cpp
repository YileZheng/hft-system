#include "order_book.hpp"
#include "suborder_book.hpp"


//#define __DEBUG__

void order_book(
	// data
	Message *stream_in,
	price_depth *stream_out,

	// configuration inputs
	symbol_t axi_read_symbol,
	ap_uint<8> axi_read_max,
	int axi_size,

	// control input
	char axi_instruction  // void, run, halt, read book, clear, config symbol map | read_max 

){
#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem0 port=stream_out offset=slave
#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem1 port=stream_in offset=slave

#pragma HLS INTERFACE s_axilite bundle=control port=return
#pragma HLS INTERFACE s_axilite port=axi_instruction bundle=control
#pragma HLS INTERFACE s_axilite port=axi_read_symbol bundle=control
#pragma HLS INTERFACE s_axilite port=axi_read_max bundle=control
#pragma HLS INTERFACE s_axilite port=axi_size 	bundle=control
#pragma HLS INTERFACE s_axilite port=stream_in 	bundle=control
#pragma HLS INTERFACE s_axilite port=stream_out bundle=control

	
	static SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS]={{AS_SLOTSIZE, AS_UNIT}};
#pragma HLS ARRAY_PARTITION variable=books dim=1 complete
  // TODO
	// 10 stock symbols: "AAPL", "AMZN", "GOOG",  "INTC", "MSFT",  "SPY", "TSLA", "NVDA", "AMD", "QCOM"
	// static symbol_t symbol_map[STOCKS] = {  0x4141504c20202020, 0x414d5a4e20202020, 0x474f4f4720202020, 0x494e544320202020, 
	// 										0x4d53465420202020, 0x5350592020202020, 0x54534c4120202020, 0x4e56444120202020, 
	// 										0x414d442020202020, 0x51434f4d20202020};
	static symbol_t symbol_map[STOCKS] = {0};
#pragma HLS ARRAY_RESHAPE variable=symbol_map dim=1 complete
	static price_depth stream_out_buffer[STOCKS][STREAMOUT_BUFFER_SIZE];
#pragma HLS ARRAY_PARTITION variable=stream_out_buffer dim=1 complete
	static ap_uint<STOCKS> read_req_concat=0;
	static ap_uint<8> read_max=10;
	
	// control signal
	static ap_uint<1> halt=1, en_read=0, en_clear=0, read_map=0; 
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
		read_map = 1;
		break;
	case 'm':	// update read max 
		read_max = axi_read_max;
		break;
	case 'A': 	// config all
		read_max = axi_read_max;

	default:
		break;
	} 
	instruction = 'v';


	// read symbol mapping
	if (en_read){
		index_read = symbol_mapping(symbol_map, axi_read_symbol, true);
#ifdef __DEBUG__
			if (!(index_read>=0)&&(index_read<STOCKS)){
				printf("Read symbol not found in the local symbol map, discard!!!!!!!!");
			}
#endif
		read_req_concat = (0x1<<index_read);
	}

	ROUTINE_SUBBOOKS_CONTROLLER:
	for (int i=0; i<STOCKS; i++){
#pragma HLS UNROLL
		ap_uint<1> read_req = (read_req_concat>>i) & 0x1;
		books[i].subbook_controller(read_req);
	}

	// order symbol mapping
	STREAM_IN_READ:
	for (int i=0; i<axi_size; i++){
		message_in = stream_in[i];
		ordermessage_in.order_info.orderID = message_in.orderID;
		ordermessage_in.order_info.size = message_in.size;
		ordermessage_in.order_info.price = message_in.price;
		ordermessage_in.operation = message_in.operation;
		ordermessage_in.side = message_in.side;
		index_msg = symbol_mapping(symbol_map, message_in.symbol, false);
#ifdef __DEBUG__
			if (!(index_msg>=0)&&(index_msg<STOCKS)){
				printf("Input symbol not found in the local symbol map, discard!!!!!!!!");
			}
#endif
		ROUTINE_SUBBOOKS_NEWORDER:
		for (int i=0; i<STOCKS; i++){
#pragma HLS UNROLL
			transMessage transmessage_in = {ordermessage_in, {((index_msg==i)&&(!halt))?1:0}};
			ap_uint<1> read_req = (read_req_concat>>i) & 0x1;
			books[i].book_maintain(transmessage_in);
		}
		
	}
	// if (read_map){
	// 	READ_MAP:
	// 	for (int i=0; i<STOCKS; i++){
	// 		stream_out[i].price = (price_t)symbol_map[i];
	// 	}
	// 	read_map = 0;
	// }
	if (en_read){
		ROUTINE_SUBBOOKS_READ:
		for (int i=0; i<STOCKS; i++){
	#pragma HLS UNROLL
			ap_uint<1> read_req = (read_req_concat>>i) & 0x1;
			books[i].book_read(read_max, stream_out_buffer[i]);
		}
		READ_STREAM_BUFFER:
		for (int i=0; i<(2*read_max)+2; i++){
			stream_out[i] = stream_out_buffer[index_read][i];
		}
		en_read = 0;
		read_req_concat = 0;
	}

}


int symbol_mapping(
	symbol_t symbol_map[STOCKS],
	symbol_t symbol,
	bool read_req
){
#pragma HLS INLINE off
	static char num = 0;
	int index=-1;
	SYMBOL_MAPPING:
	for (int i=0; i<STOCKS; i++){
#pragma HLS UNROLL
		if (symbol_map[i]==symbol){
			index = i;
		}
	}
	if ((index == -1)&&(num != STOCKS)&&(!read_req)){
		index = num;
		symbol_map[num++] = symbol;
	}
	return index;
}

// void routine_subbooks(
// 	ap_uint<8> read_max,
// 	ap_uint<STOCKS> &read_req_concat,
	
// 	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS],

// 	hls::stream<price_depth> &stream_out
// ){
// 	ROUTINE_SUBBOOKS:
// 	for (int i=0; i<STOCKS; i++){
// #pragma HLS UNROLL
// 		ap_uint<1> read_req = (read_req_concat>>i) & 0x1;
// 		books[i].suborder_book(ordermessage_in, read_max, read_req, stream_out);
// 	}
// 	read_req_concat = 0;
// }

// void update_subbooks(
// 	int index_msg,
// 	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS],

// 	orderMessage ordermessage_in
// ){
// 	// update books on message event
// 	UPDATE_SUBBOOKS:
// 	for (int i=0; i<STOCKS; i++){
// #pragma HLS UNROLL
// 		if (i == index_msg){
// 			books[i].book_maintain(ordermessage_in);
// 		}
// 	}

// }

// void init_books(
// 	int slotsize,
// 	float unit,
// 	SubOrderBook<AS_RANGE, AS_CHAIN_LEVELS> books[STOCKS]
// ){
// 	for (int i=0; i<STOCKS; i++){
// 		books[i].config(slotsize, unit);
// 	}
// }

