
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<map>
#include "common.hpp"

#define STOCK_TEST 1
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
	const char*    STR_ERROR   = "ERROR:   ";
	const char*    STR_FAILED  = "FAILED:  ";
	const char*    STR_PASSED  = "PASSED:  ";
	const char*    STR_INFO    = "INFO:    ";
	const char*    STR_USAGE   = "USAGE:   ";

	int time_speedup = 1; // how many times faster to backtest, controls the message send rate
	int level=LEVEL_TEST;

	// files
	string message_path[STOCK_TEST] =  {{"/AAPL_2012-06-21_34200000_57600000_message_10.csv"},
										// {"/AMZN_2012-06-21_34200000_57600000_message_10.csv"},
										// {"/GOOG_2012-06-21_34200000_57600000_message_10.csv"}
										};
	string orderbook_path[STOCK_TEST] ={{"/AAPL_2012-06-21_34200000_57600000_orderbook_10.csv"},
										// {"/AMZN_2012-06-21_34200000_57600000_orderbook_10.csv"},
										// {"/GOOG_2012-06-21_34200000_57600000_orderbook_10.csv"}
										};
	string result_path{"result.csv"};
	string answer_path{"answer.csv"};
	
	// symbol
	// #ifndef __isLittleEndian__
	// char symbols[STOCK_TEST][8] =  {{'A','A','P','L',' ',' ',' ',' '},
	// 								{'A','M','Z','N',' ',' ',' ',' '},
	// 								{'G','O','O','G',' ',' ',' ',' '}};
	// #else
	// char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
	// 								{' ',' ',' ',' ', 'N','Z','M','A'},
	// 								{' ',' ',' ',' ', 'G','O','O','G'}};
	// #endif

	// symbol_t *symbol_map=(symbol_t*)symbols;
	// 10 stock symbols: "AAPL", "AMZN", "GOOG",  "INTC", "MSFT",  "SPY", "TSLA", "NVDA", "AMD", "QCOM"
	symbol_t symbol_map[STOCKS] = {  0x4141504c20202020, 
									// 0x414d5a4e20202020, 0x474f4f4720202020, 0x494e544320202020, 
									// 0x4d53465420202020, 0x5350592020202020, 0x54534c4120202020, 0x4e56444120202020, 
									// 0x414d442020202020, 0x51434f4d20202020
									};

	// caches
	last_manager* last_ls[STOCK_TEST];
	long int cur_msg_time_ls[STOCK_TEST];
	string last_orderbook_line_ls[STOCK_TEST], llast_orderbook_line_ls[STOCK_TEST], cur_msg_line_ls[STOCK_TEST];
	ifstream message_ls[STOCK_TEST],  orderbook_ls[STOCK_TEST];
	ofstream result, answer;

	// order stream
	long id=-1;

	ap_uint<STOCK_TEST> neof = 0-1;

	int get_symbol_loc(symbol_t target_symbol);
	void close_files();

	public:

	vector<Message> init_book_messsages();
	vector<Message> generate_messages(int num, vector<orderOp> v_orderop);
	vector<Message> generate_messages_classified(int num, vector<orderOp> v_orderop);
	bool final_check();
	bool check_resultbook(vector<vector<price_depth>> resultbook, symbol_t target_symbol);
	string get_true_orderbook(symbol_t target_symbol);
	
	messageManager(char* file_dir){
		// open files
		for (int i=0; i<STOCK_TEST; i++){
			// ifstream message, orderbook;
			// message.open(message_path[i], ios::in);
			// orderbook.open(orderbook_path[i], ios::in);
			message_ls[i].open(file_dir+message_path[i], ios::in);
			orderbook_ls[i].open(file_dir+orderbook_path[i], ios::in);
			if((!message_ls[i])){
				std::cout<<"Sorry the file you are looking for is not available: "<<file_dir+message_path[i]<<endl;
			}
			// message_ls.push_back(&message);

			if((!orderbook_ls[i])){
				std::cout<<"Sorry the file you are looking for is not available: "<<file_dir+orderbook_path[i]<<endl;
			}
			// orderbook_ls.push_back(&orderbook);
		}

		result.open(result_path, ios::out | ios::trunc);
		answer.open(answer_path, ios::out | ios::trunc);
	}

};

