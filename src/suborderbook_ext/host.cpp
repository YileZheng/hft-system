
#include <limits.h>
#include <sys/stat.h>

#include "common.hpp"
#include "host_message.hpp"
// #include "host_kernel_handle_cl.hpp"
#include "event_timer.hpp"

#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>

#include<iostream>
#include<fstream>
#include<sstream>
// #include<string>
#include<vector>
#include<ctime>
#include<numeric>
#include<cmath>

#include "profile.hpp"

using namespace std;

typedef uint32_t u32;


#define XAdder_apint_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset >> 2)) = (u32)(Data)
#define XAdder_apint_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset >> 2))

/* kernel configuration related definitions */
#define AXILITE_OFFSET_CONTROL       	0x00	// 4 * 4Byte
#define AXILITE_OFFSET_ORDER       	0x10	// 6 * 4Byte
#define AXILITE_OFFSET_BOOK       	0x2c	// 2 * 4B
#define AXILITE_OFFSET_BID_HEAD      0x38	// 2 * 4B; Low: data, High: ap_ Valid
#define AXILITE_OFFSET_ASK_HEAD      0x48	// 2 * 4B; Low: data, High: ap_ Valid
#define KRNL_REG_BASE_ADDR          0xA0020000
/* transfer related definitions */
#define BOOK_ADDR                     0x10000000

#define INTI_DATA_LEN               2*1024*1024
#define TRANS_LEN                   64
#define TEST_LOOP_NUM               3

#define lluint long long unsigned int


#define OCL_CHECK(error,call)                                       \
    call;                                                           \
    if (error != CL_SUCCESS) {                                      \
      printf("%s:%d Error calling " #call ", error code is: %d\n",  \
              __FILE__,__LINE__, error);                            \
      exit(EXIT_FAILURE);                                           \
    }                                       
	
void wait_for_enter(const std::string &msg);

template<int RANGE, int SLOTSIZE, int INVALID_LINK>
void book_read(
	vector<vector<price_depth>> &resultbook, 
	int read_max,
	price_depth_chain* book,
	addr_index base_bookIndex[2]

){
	price_depth lvl_out;
	price_depth_chain cur_block;
	int ind, read_cnt;
	vector<price_depth> cur_v;

	READ_BOOK_SIDE:
	for(int side=0; side<=1; side++){
		read_cnt = read_max;
		addr_index book_side_offset = (side==1)? RANGE: 0;
		READ_BOOK_LEVELS:
		for (int i=0; i<RANGE; i++){
			if (read_cnt == 0) break;
			ind = (base_bookIndex[side]+i>=RANGE)? base_bookIndex[side]+i-RANGE: base_bookIndex[side]+i;
			ind += book_side_offset;
			cur_block = book[ind];
			if(cur_block.price != 0){
				READ_BOOK_LEVEL_LINK:
				for (int j=0; j<int(SLOTSIZE); j++){
					lvl_out.price = cur_block.price;
					lvl_out.size = cur_block.size;
					std::cout << lvl_out.price << " " << lvl_out.size << endl;
					cur_v.push_back(lvl_out);
					read_cnt--;
					if ((cur_block.next != INVALID_LINK) && (read_cnt != 0))
						cur_block = book[cur_block.next];
					else break;
				}
			}
		}
		resultbook.push_back(cur_v);
		cur_v.clear();
		std::cout << std::endl;
	}
}

class KernelHandle{
	char*    STR_ERROR   = "ERROR:   ";
	char*    STR_FAILED  = "FAILED:  ";
	char*    STR_PASSED  = "PASSED:  ";
	char*    STR_INFO    = "INFO:    ";
	char*    STR_USAGE   = "USAGE:   ";

    EventTimer et;
	addr_index book_head[2];
	price_depth_chain* book_addr;
	uint32_t* krnl_reg_base;

    int read_lvls = LEVEL_TEST;	
	
	public:
	KernelHandle(int dh){
		krnl_reg_base = (uint32_t*)mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, dh, KRNL_REG_BASE_ADDR);
		book_addr  = (price_depth_chain*)mmap(NULL, 128 * (2*AS_RANGE + AS_CHAIN_LEVELS), PROT_READ | PROT_WRITE, MAP_SHARED, dh, BOOK_ADDR);

	}

	~KernelHandle(){
		// m_queue.enqueueUnmapMemObject(buf_in, host_write_ptr);
		// m_queue.enqueueUnmapMemObject(buf_out, host_read_ptr);
		// m_queue.finish();
	}

    double read_orderbook(vector<vector<price_depth>>& data_out);
    double new_orders(vector<Message>& data_in);
    double run(orderMessage order);

};


char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
								// {' ',' ',' ',' ', 'N','Z','M','A'},
								// {' ',' ',' ',' ', 'G','O','O','G'}
								};
// config
symbol_t *symbol_map=(symbol_t*)symbols;

const char*    STR_ERROR   = "ERROR:   ";
const char*    STR_FAILED  = "FAILED:  ";
const char*    STR_PASSED  = "PASSED:  ";
const char*    STR_INFO    = "INFO:    ";
const char*    STR_USAGE   = "USAGE:   ";

int main(int argc, char* argv[]) {
    // int dh = open("/dev/mem", O_RDWR | O_SYNC);
    int dh = open("/dev/mem", O_RDWR);

    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
    // ------------------------------------------------------------------------------------

    char* binaryFile;
    char* file_dir;
    if (argc == 3){
        file_dir = argv[2];
        std::cout <<"Using order message file specfied through the command line: " << file_dir << std::endl;
        binaryFile = argv[1];
        std::cout <<"Using FPGA binary file specfied through the command line: " << binaryFile << std::endl;
    }else{
        file_dir = ".";
        std::cout <<"No order message file specfied through the command line, using: " << file_dir << std::endl;
        if (argc==2) {
            binaryFile = argv[1];
            std::cout <<"Using FPGA binary file specfied through the command line: " << binaryFile << std::endl;
        }
        else {
            binaryFile = "./order_book.xclbin";
            std::cout << "No FPGA binary file specified through the command line, using:" << binaryFile <<std::endl;
        }

    }
    EventTimer et;
    // wait_for_enter("\nPress ENTER to continue after setting up ILA trigger...");

	messageManager messages_handler(file_dir);
    symbol_t read_symbol;
    ap_uint<8> read_max=LEVEL_TEST;
    char instruction;
	map<string, symbol_t> symbol2hex = {{"AAPL", 0x4141504c20202020},
                                        {"AMZN", 0x414d5a4e20202020},
                                        {"GOOG", 0x474f4f4720202020},
                                        {"INTC", 0x494e544320202020},
                                        {"MSFT", 0x4d53465420202020},
                                        {"SPY" , 0x5350592020202020},
                                        {"TSLA", 0x54534c4120202020},
                                        {"NVDA", 0x4e56444120202020},
                                        {"AMD" , 0x414d442020202020},
                                        {"QCOM", 0x51434f4d20202020}};

	vector<string> v_symbol = {"AAPL", "AMZN", "GOOG", "INTC", "MSFT", "SPY" , "TSLA", "NVDA", "AMD" , "QCOM"};

    vector<Message> stream_data;
    vector<vector<price_depth>> read_price;
	vector<orderOp> ops;
    double elapse_ns;
    KernelHandle k_handler(dh);

    // initialize order books  ---------------------------------

    std::cout << "Initiate orderbook contents" << std::endl;

    std::cout << "Prepare data..." << std::endl;
    stream_data = messages_handler.init_book_messsages();

    std::cout << "Stream initial orderbook levels to the kernel" <<std::endl;
    elapse_ns = k_handler.new_orders(stream_data);


	// stream orders to kernel ---------------------------------

	// burst order feeding

	double elapse_ns_total = 0, num_orders = 0, elapse_read = 0, num_read = 0;
	int order_len = 20, shot = 10;      // should not exceed MAX_WRITE
	bool match;

	while (shot--){
		stream_data = messages_handler.generate_messages(order_len, ops);
    	elapse_ns = k_handler.new_orders(stream_data);
		std::cout << "Burst order length: " << stream_data.size() << " - elasped: " << elapse_ns << " ns"<< std::endl;
		elapse_ns_total += elapse_ns;
		num_orders += stream_data.size();
		
		read_symbol = symbol2hex[v_symbol[0]];
		std::cout << "Read orderbook from system " <<std::endl;
		elapse_ns = k_handler.read_orderbook(read_price);
		std::cout << "Read orderbook length order length: " << read_max << " - elasped: " << elapse_ns << " ns"<< std::endl;
		match = messages_handler.check_resultbook(read_price, read_symbol);
		num_read ++;
		elapse_read += elapse_ns;
	}
	double time_burst = (elapse_ns_total/num_orders);
	std::cout << ">> Average order processing time: " << time_burst << " ns - for " << num_orders << " order messages" << std::endl;

	double time_read = elapse_read/num_read;
	std::cout << ">> Average orderbook reading time: " << time_read << " ns - for " << num_read << " order messages" << std::endl;

    return 0;
}



double KernelHandle::read_orderbook(
	vector<vector<price_depth>>& data_out
){

	// Set vadd kernel arguments
	data_out.clear();
	et.add("Read orderbook levels");
	book_read<AS_RANGE, AS_SLOTSIZE, AS_INVALID_LINK>(data_out, read_lvls, book_addr, book_head);
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;
	
	et.print();
	return elapsed_ns;
}


double KernelHandle::new_orders(
	// data
	vector<Message>& data_in
){
	std::cout << "Feed new orders to orderbook system" << std::endl;
	orderMessage order_tmp;
	int num_orders = data_in.size();
	double elapsed_ns_total = 0;

	//setting input data
	for(int i = 0 ; i < num_orders; i++){
		Message m = data_in[i];
		order_tmp.operation = m.operation;
		order_tmp.side = m.side;
		order_tmp.order_info.orderID = m.orderID;
		order_tmp.order_info.price = m.price;
		order_tmp.order_info.size = m.size;
     	std::cout<< "Send order - Symbol: " << std::string((char*)&m.symbol, 8) <<" OrderID: "<<m.orderID << " Side: " <<m.side<<" Type: "<<m.operation<<" Price: "<< m.price <<" Volume: "<< m.size<<std::endl;
		elapsed_ns_total += run(order_tmp);
	}

	std::cout << "Averaged Single Order Running Time measured from " << num_orders << "orders: " << (elapsed_ns_total/num_orders) << std::endl;

	return elapsed_ns_total;
}

double KernelHandle::run(
	orderMessage order
){

	// Configure and start up the kernel
	/* set axi signals for cache coherency */

	/* set book address */
	XAdder_apint_WriteReg(krnl_reg_base, AXILITE_OFFSET_BOOK, BOOK_ADDR);
	XAdder_apint_WriteReg(krnl_reg_base, AXILITE_OFFSET_BOOK + 4, 0x0);

	/* set order data */
	for (int i = 0; i < 6; i++){
		XAdder_apint_WriteReg(krnl_reg_base, AXILITE_OFFSET_ORDER + 4*i, (order >> (32 * i)) & 0xffffffff);
	}

	/* start the kernel */
	uint control = (XAdder_apint_ReadReg(krnl_reg_base, AXILITE_OFFSET_CONTROL) & 0x80);
	XAdder_apint_WriteReg(krnl_reg_base, AXILITE_OFFSET_CONTROL, control | 0x01);

	et.add("Run kernel arguments");
	while (((XAdder_apint_ReadReg(krnl_reg_base, AXILITE_OFFSET_CONTROL) >> 1) & 0x1) == 0) {
		// wait until kernel finish running
	}
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;

	if ((XAdder_apint_ReadReg(krnl_reg_base, AXILITE_OFFSET_BID_HEAD + 4) & 0x1) == 1){
		book_head[0] = (XAdder_apint_ReadReg(krnl_reg_base, AXILITE_OFFSET_BID_HEAD);
	}

	if ((XAdder_apint_ReadReg(krnl_reg_base, AXILITE_OFFSET_ASK_HEAD + 4) & 0x1) == 1){
		book_head[1] = (XAdder_apint_ReadReg(krnl_reg_base, AXILITE_OFFSET_ASK_HEAD);
	}

	et.print();
	return elapsed_ns;
}

void wait_for_enter(const std::string &msg) {
 std::cout << msg << std::endl;
 std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
