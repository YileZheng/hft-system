#include "suborder_book.hpp"


//#define __DEBUG__

void suborder_book(
	// data
	Message *stream_in,
	price_depth *stream_out,

	// configuration inputs
	// symbol_t axi_read_symbol,
	ap_uint<1> axi_read_req,
	ap_uint<8> axi_read_max,
	int axi_size,

	// control input
	// char axi_instruction  // void, run, halt, read book, clear, config symbol map | read_max 

){
#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem0 port=stream_out offset=slave
#pragma HLS INTERFACE m_axi depth=4096 bundle=gmem1 port=stream_in offset=slave

#pragma HLS INTERFACE s_axilite bundle=control port=return
#pragma HLS INTERFACE s_axilite port=axi_read_req bundle=control
#pragma HLS INTERFACE s_axilite port=axi_read_max bundle=control
#pragma HLS INTERFACE s_axilite port=axi_size 	bundle=control
#pragma HLS INTERFACE s_axilite port=stream_in 	bundle=control
#pragma HLS INTERFACE s_axilite port=stream_out bundle=control


    // static 
	hls::stream<price_depth, 50> feed_stream_out;
    ap_uint<1> read_req = axi_read_req;
	Message message_in;
	orderMessage ordermessage_in;

	// order symbol mapping
	STREAM_IN_READ:
	for (int i=0; i<axi_size; i++){
		message_in = stream_in[i];
		ordermessage_in.order_info.orderID = message_in.orderID;
		ordermessage_in.order_info.size = message_in.size;
		ordermessage_in.order_info.price = message_in.price;
		ordermessage_in.operation = message_in.operation;
		ordermessage_in.side = message_in.side;		

        dut_suborder_book(
            ordermessage_in,
            axi_read_max,
            read_req,
            feed_stream_out
        );
        if (read_req){
            read_req = 0;
        }
	}
    

	if (axi_read_req) {
		for (int i=0; i<2*axi_read_max+2; i++){
			stream_out[i] = feed_stream_out.read();
		}
	}

    // int read_cnt = 0;
    // while ( ~feed_stream_out.empty()  ){
    //     stream_out[read_cnt++] = feed_stream_out.read();
    // }

}
