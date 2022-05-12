
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<map>
#include "common.hpp"

#define STOCK_TEST 3
#define LEVEL_TEST 10

using namespace std;


vector<string> split_string(std::string s,std::string delimiter);
string concat_string(vector<vector<price_depth>> pd, std::string delimiter, int level);
void check_update_last_price(vector<string> orderBook_split, int price_lasta_init, int price_lastb_init, int vol_lasta_init, int vol_lastb_init);

class last_manager{
	int cache_max_len = 10;
	map<int, int> cache_last;
	vector<pair<int, int>> cache_lasta, cache_lastb;
	int price_last_b, price_last_a;
	int vol_last_b, vol_last_a;
	symbol_t cur_symbol;
	

	public:
	last_manager(
		int price_lasta_init,
		int price_lastb_init,
		int vol_lasta_init,
		int vol_lastb_init,
		symbol_t cur_symbol_init
	){
		price_last_b=price_lastb_init, price_last_a=price_lasta_init;
		vol_last_b=vol_lastb_init, vol_last_a=vol_lasta_init;
		cur_symbol = cur_symbol_init;
	}

	last_manager(last_manager &l){
		price_last_b=l.price_last_b;
		price_last_a=l.price_last_a;
		vol_last_b=l.vol_last_b;
		vol_last_a=l.vol_last_a;
		cur_symbol = l.cur_symbol;

	}

	vector<Message> check_update_last_price(
		vector<string> orderBook_split,
		Time tmstmp
	);

};


class messageManager{

	int time_speedup = 1; // how many times faster to backtest, controls the message send rate
	int level=LEVEL_TEST;

	// files
	string message_path[STOCK_TEST] =  {{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_message_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AMZN_2012-06-21_34200000_57600000_message_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/GOOG_2012-06-21_34200000_57600000_message_10.csv"}};
	string orderbook_path[STOCK_TEST] ={{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_orderbook_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AMZN_2012-06-21_34200000_57600000_orderbook_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/GOOG_2012-06-21_34200000_57600000_orderbook_10.csv"}};
	string result_path{"result.csv"};
	string answer_path{"answer.csv"};
	
	// symbol
	#ifndef __isLittleEndian__
	char symbols[STOCK_TEST][8] =  {{'A','A','P','L',' ',' ',' ',' '},
									{'A','M','Z','N',' ',' ',' ',' '},
									{'G','O','O','G',' ',' ',' ',' '}};
	#else
	char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
									{' ',' ',' ',' ', 'N','Z','M','A'},
									{' ',' ',' ',' ', 'G','O','O','G'}};
	#endif

	symbol_t *symbol_map=(symbol_t*)symbols;

	// caches
	last_manager* last_ls[STOCK_TEST];
	long int cur_msg_time_ls[STOCK_TEST];
	string last_orderbook_line_ls[STOCK_TEST], cur_msg_line_ls[STOCK_TEST];
	ifstream message_ls[STOCK_TEST],  orderbook_ls[STOCK_TEST];
	ofstream result, answer;

	// order stream
	long id=-1;

	ap_uint<STOCK_TEST> neof = 0-1;

	int get_symbol_loc(symbol_t target_symbol);
	void close_files();

	public:

	vector<Message> init_book_messsages();
	vector<Message> generate_messages(int num);
	bool check_resultbook(vector<price_depth> price_stream_out, symbol_t target_symbol);
	bool final_check();
	string get_true_orderbook(symbol_t target_symbol);
	
	messageManager(){
		// open files
		for (int i=0; i<STOCK_TEST; i++){
			// ifstream message, orderbook;
			// message.open(message_path[i], ios::in);
			// orderbook.open(orderbook_path[i], ios::in);
			message_ls[i].open(message_path[i], ios::in);
			orderbook_ls[i].open(orderbook_path[i], ios::in);
			if((!message_ls[i])){
				std::cout<<"Sorry the file you are looking for is not available: "<<message_path[i]<<endl;
			}
			// message_ls.push_back(&message);

			if((!orderbook_ls[i])){
				std::cout<<"Sorry the file you are looking for is not available: "<<orderbook_path[i]<<endl;
			}
			// orderbook_ls.push_back(&orderbook);
		}

		result.open(result_path, ios::out | ios::trunc);
		answer.open(answer_path, ios::out | ios::trunc);
	}

};


class StreamHandle: public ApiHandle{
    Message *host_write_ptr;
    price_depth *host_read_ptr; 
    int MAX_WRITE, MAX_READ;

    public:
    cl_stream h2k_stream;
    cl_stream k2h_stream;

    StreamHandle(int max_write, int max_read, char* binaryName, char* kernel_name, bool oooQueue): ApiHandle(binaryName, kernel_name, oooQueue){
        MAX_WRITE = max_write, MAX_READ = max_read;
        posix_memalign((void**)&host_write_ptr, 4096, MAX_WRITE * sizeof(Message)); 
        posix_memalign((void**)&host_read_ptr, 4096, MAX_READ * sizeof(price_depth)); 
        
        // streamming ------------------------------

        cl_int err;
        // Device connection specification of the stream through extension pointer
        cl_mem_ext_ptr_t  ext;  // Extension pointer
        ext.param = m_kernel;     // The .param should be set to kernel (cl_kernel type)
        ext.obj = nullptr;

        // The .flag should be used to denote the kernel argument
        // Create write stream for argument 0 of kernel
        std::cout << "Create host-kernel stream" << std::endl;
        ext.flags = 0;
        h2k_stream = clCreateStream(m_device_id, XCL_STREAM_READ_ONLY, CL_STREAM, &ext, &err);

        // Create read stream for argument 1 of kernel
        std::cout << "Create kernel-host stream" << std::endl;
        ext.flags = 1;
        k2h_stream = clCreateStream(m_device_id, XCL_STREAM_WRITE_ONLY, CL_STREAM, &ext,&err);

    }

    cl_streams_poll_req_completions* wait(int reqN, int timeout_m){
        cl_int err;
        // Checking the request completion, wait until finished
        std::cout << "Polling for completions of streaming.." << std::endl;
        cl_streams_poll_req_completions poll_req[reqN] {0}; // 1 Requests
        auto num_compl = reqN;
        OCL_CHECK(err, clPollStreams(m_device_id, poll_req, reqN, reqN, &num_compl, timeout_m, &err));
        std::cout << "Polling complete" << std::endl;
        // Blocking API, waits for 2 poll request completion or 100'000ms, whichever occurs first
        return poll_req;
    }

    void write_nb(vector<Message> data){
        cl_int err;
        int size = data.size();
        if(size <= MAX_WRITE){
            for (int i=0; i<size; i++){
                host_write_ptr[i] = data[i];
            }
        }else{
			std::cout << "FAILED TEST - Stream from host to kernel overflow max mem buffer" << std::endl;
			exit(1);
        }

        // Initiating the WRITE transfer
        std::cout << "Initiate write stream transfer" << std::endl;
        cl_stream_xfer_req wr_req {0};
        wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
        wr_req.priv_data = (void*)"orderwrite";
        OCL_CHECK(err, clWriteStream(h2k_stream, host_write_ptr, size, &wr_req , &err));

    }

    void write(vector<Message> data){
        write_nb(data);
        wait(1, 100000);    // wait for 100000ms
    }

    cl_stream_xfer_req read_nb(){
        cl_int err;
	    // Initiate the READ transfer
        std::cout << "Initiate read stream transfer" << std::endl;
        cl_stream_xfer_req rd_req {0};
        rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
        rd_req.priv_data = (void*)"bookread"; // You can think this as tagging the transfer with a name
        OCL_CHECK(err, clReadStream(k2h_stream, host_read_ptr, MAX_READ, &rd_req, &err));
        return rd_req;
    }

    vector<price_depth> read(){
        auto rd_req = read_nb();
        auto poll_req = wait(1, 100000);
        
		vector<price_depth> read_data;
        for (auto i=0; i<1; ++i) {
            if(rd_req.priv_data == poll_req[i].priv_data) { // Identifying the read transfer

                // Getting read size, data size from kernel is unknown
                std::cout << "Read stream data from the kernel" <<std::endl;
                ssize_t result_size=poll_req[i].nbytes;
                int lvls = result_size/sizeof(price_depth);
                for (int i=0; i< lvls; i++){
                    read_data.push_back(host_read_ptr[i]);
                }
            }
        }
        return read_data;
    }
};