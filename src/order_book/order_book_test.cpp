
#define __gmp_const const
#include <gmp.h>
#include <mpfr.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<ctime>
#include<numeric>
#include<map>

#include "order_book.hpp"

using namespace std;

#define MULTI 10000
#define LV 10
#define UNIT 0.01
#define SLOTSIZE 10
#define READ_MAX 10
#define STOCK_TEST 3
int read_max = READ_MAX;
//char symbols[STOCK_TEST][8] =  {{'A','A','P','L',' ',' ',' ',' '},
//								{'A','M','Z','N',' ',' ',' ',' '},
//								{'G','O','O','G',' ',' ',' ',' '}};

char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
								{' ',' ',' ',' ', 'N','Z','M','A'},
								{' ',' ',' ',' ', 'G','O','O','G'}};
// config
symbol_t *symbol_map=(symbol_t*)symbols;


vector<string> split_string(std::string s,std::string delimiter);
string concat_string(vector<vector<price_depth>> pd, std::string delimiter, int level);
void check_update_last_price(vector<string> orderBook_split, int price_lasta_init, int price_lastb_init, int vol_lasta_init, int vol_lastb_init);

class last_manager{
	map<int, int> cache_last;
	vector<pair<int, int>> cache_lasta, cache_lastb;
	price_depth price_stream_out[100];
	Message stream_in[10];
	int cache_max_len = 10;

	public:
	int price_last_b, price_last_a;
	int vol_last_b, vol_last_a;
	symbol_t cur_symbol;
	
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

	void check_update_last_price(
		vector<string> orderBook_split,
		Time tmstmp
	);

};



int main()
{
	int level=LV;
	string message_path[STOCK_TEST] =  {{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_message_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AMZN_2012-06-21_34200000_57600000_message_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/GOOG_2012-06-21_34200000_57600000_message_10.csv"}};
	string orderbook_path[STOCK_TEST] ={{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_orderbook_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AMZN_2012-06-21_34200000_57600000_orderbook_10.csv"},
										{"/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/GOOG_2012-06-21_34200000_57600000_orderbook_10.csv"}};
					
	string result_path("result.csv");
	string answer_path("answer.csv");

	string pr, qty, oid, tstmp, ab, op;
	ifstream message_ls[STOCK_TEST],  orderbook_ls[STOCK_TEST];
	// vector<ifstream *> message_ls(STOCK_TEST),  orderbook_ls(STOCK_TEST);
	// vector<last_manager *> last_ls(STOCK_TEST);
	last_manager* last_ls[STOCK_TEST];
	string last_orderbook_line_ls[STOCK_TEST];
	ifstream new_file;
	ofstream result, answer;

	vector<double> stat[4]; // stat_add, stat_chg, stat_rmv;
	clock_t start, end;
	double elapsed_ms;


	// order stream
	int id=-1;
	orderOp odop;
	ap_uint<1> bid, req_read;
	price_depth price_stream_out[100] = {0};
	Message stream_in[10] = {0};
	price_depth price_read;
	Message input_in;


	// initiate orderbook system
	order_book(
		// data
		stream_in,
		price_stream_out,
		// configuration inputs
		(symbol_t)0,
		read_max,
		0,
		// control input
		'A'  // void, run, halt, read book, clear, config symbol map | read_max 
	);

	// boot the kernel
	order_book(
		// data
		stream_in,
		price_stream_out,
		// configuration inputs
		(symbol_t)0,
		read_max,
		0,
		// control input
		'R'  // void, run, halt, read book, clear, config symbol map | read_max 
	);

	// open files
	for (int i=0; i<STOCK_TEST; i++){
		// ifstream message, orderbook;
		// message.open(message_path[i], ios::in);
		// orderbook.open(orderbook_path[i], ios::in);
		message_ls[i].open(message_path[i], ios::in);
		orderbook_ls[i].open(orderbook_path[i], ios::in);
		if((!message_ls[i])){
			std::cout<<"Sorry the file you are looking for is not available: "<<message_path[i]<<endl;
			return -1;
		}
		// message_ls.push_back(&message);

		if((!orderbook_ls[i])){
			std::cout<<"Sorry the file you are looking for is not available: "<<orderbook_path[i]<<endl;
			return -1;
		}
		// orderbook_ls.push_back(&orderbook);
	}

	result.open(result_path, ios::out | ios::trunc);
	answer.open(answer_path, ios::out | ios::trunc);


	// stream inputs
	string line, orderbook_line;
	vector<string> line_split;
	for (int ii=0; ii<STOCK_TEST; ii++){
		if (!message_ls[ii].eof()){
			id++;
			orderbook_ls[ii] >> orderbook_line;
			message_ls[ii] >> line;
			line_split = split_string(orderbook_line, string(","));
			std:: cout << "Initiate order book "<< ii << std::endl;
			bid = 1;
			odop = NEW;
			req_read = 0;
			for(vector<string>::iterator it = line_split.begin(); it != line_split.end(); it++){
				pr = *it;
				qty = *(++it);
				input_in.price = (price_t)(stof(pr)/MULTI); input_in.size = (qty_t)(stof(qty)); input_in.orderID = (size_t)1;
				bid++;
				input_in.timestamp = (Time)34200*1000000000;
				input_in.symbol = symbol_map[ii];
				input_in.operation = odop;
				input_in.side = bid;
				// stream_in.write(input_in);
				stream_in[0] = input_in;
				order_book(
					// data
					stream_in,
					price_stream_out,
					// configuration inputs
					symbol_map[ii],
					read_max,
					1,
					// control input
					'v'  // void, run, halt, read book, clear, config symbol map | read_max 
				);
			}
			int pricea_init, priceb_init, vola_init, volb_init;
			pricea_init = stoi(*(line_split.end()-4));
			priceb_init = stoi(*(line_split.end()-2));
			vola_init = stoi(*(line_split.end()-3));
			volb_init = stoi(*(line_split.end()-1));
			last_manager* last = new last_manager(pricea_init, priceb_init, vola_init, volb_init,symbol_map[ii]);
			// last_ls.push_back(&last);
			last_ls[ii] = last;
			last_orderbook_line_ls[ii] = orderbook_line;
		}
	}

	// boot the kernel
	order_book(
		// data
		stream_in,
		price_stream_out,
		// configuration inputs
		(symbol_t)0,
		read_max,
		0,
		// control input
		's'  // void, run, halt, read book, clear, config symbol map | read_max 
	);
	// std::cout << "Read symbol map from the kernel "<< std::endl;
	// for (int i=0; i<STOCK_TEST; i++){
	// 	printf("0x%x", ((symbol_t)price_stream_out[i]).to_uint64());
	// }


	ap_uint<STOCK_TEST> neof = 0-1;
	while (neof){
		for (int ii=0; ii<STOCK_TEST; ii++){
			id++;
			if (!message_ls[ii].eof()){
				orderbook_ls[ii] >> orderbook_line;
				message_ls[ii] >> line;
				line_split = split_string(line, string(","));

				tstmp = line_split[0];
				op = line_split[1];
				oid = line_split[2];
				qty = line_split[3];
				pr = line_split[4];
				ab = line_split[5];

				last_ls[ii]->check_update_last_price(split_string(orderbook_line, string(",")),(Time)(stof(tstmp)*1000000000));
				
				if ((op[0] == '5') || (op[0] == '7'))
					continue;

				input_in.price = (price_t)(stof(pr)/MULTI); input_in.size = (qty_t)(stof(qty)); input_in.orderID = (size_t)(stoi(oid));


				switch (op[0])
				{
					case '1':
						odop = NEW;
						break;
					case '2':
					case '3':
					case '4':
						odop = CHANGE;
						input_in.size = -input_in.size;
						break;
					default:
						break;
				}

				bid = (ab == "1")? 1: 0;
				req_read = (((id)%50 == 0)&&(!req_read))? 1: 0;
				char instr = req_read? 'r': 'v';
				std::cout<<"Line: " << (id/STOCK_TEST) << " Symbol: " << symbol_map[ii] <<" OrderID: "<<input_in.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< input_in.price <<" Volume: "<< input_in.size <<" Read: "<<req_read<<endl;

				start = clock();
				input_in.timestamp = (Time)(stof(tstmp)*1000000000);
				input_in.symbol = symbol_map[ii];
				input_in.operation = odop;
				input_in.side = bid;
				// stream_in.write(input_in);
				stream_in[0] = input_in;
				order_book(
					// data
					stream_in,
					price_stream_out,
					// configuration inputs
					symbol_map[ii],
					read_max,
					1,
					// control input
					'v'  // void, run, halt, read book, clear, config symbol map | read_max 
				);
				end = clock();
				elapsed_ms = (double)(end-start)/CLOCKS_PER_SEC * 1000000;
				stat[(int)odop].push_back(elapsed_ms);



				if (req_read==1){
					order_book(
						// data
						stream_in,
						price_stream_out,
						// configuration inputs
						symbol_map[ii],
						read_max,
						0,
						// control input
						'r'  // void, run, halt, read book, clear, config symbol map | read_max 
					);
					end = clock();
					elapsed_ms = (double)(end-start)/CLOCKS_PER_SEC * 1000000;
					stat[3].push_back(elapsed_ms);
					vector<vector<price_depth>> resultbook;
					vector<price_depth> cur_v;
					bool br=true;
					int i=0;
					while (true){
	//					std::cout << "Symbol: " << symbol_map[ii] << std::endl;
						price_read = price_stream_out[i++];
						std::cout << price_read.price << " " << price_read.size << std::endl;
						if (price_read.price != 0){
							cur_v.push_back(price_read);
						}else {
							br = !br;
							resultbook.push_back(cur_v);
							cur_v.clear();
							if (br) break;
						}
					}
					string last_orderbook_line = last_orderbook_line_ls[ii];
					string s = concat_string(resultbook, string(","), level);
					if (s.compare(orderbook_line) != 0){
						std::cout <<"Symbol: " <<symbol_map[ii]<<": Result orderbook not match !!!!!!!!" <<std::endl;
						std::cout <<"Ground Truth: "<< orderbook_line << std::endl;
						std::cout <<"OrderBook:    "<< s << std::endl;
					}
					result << s << endl;
					answer << orderbook_line << endl;
				}
				last_orderbook_line_ls[ii] = orderbook_line;
				std::cout<<std::endl;
			}
			else{
				neof &= ~(ap_uint<STOCK_TEST>(0x1) << ii);
			}
		}
	}
	answer.close();
	result.close();
	for (int i=0; i<STOCK_TEST; i++){
		message_ls[i].close();
		orderbook_ls[i].close();
	}

	// statistic
	for(int i=0; i<4; i++){
		double sum = std::accumulate(stat[i].begin(), stat[i].end(), 0.0);
		double mean = sum / stat[i].size();

		double sq_sum = std::inner_product(stat[i].begin(), stat[i].end(), stat[i].begin(), 0.0);
		double stdev = std::sqrt(sq_sum / stat[i].size() - mean * mean);

		printf("Time elapsed: Mean %0.3fus, Stddev: %0.3fus\n", mean, stdev);
	}

	// Compare the results file with the golden results
	
	int retval=0;
	retval = system((string("diff --brief -w ")+result_path+" "+answer_path).data());
	if (retval != 0) {
		printf("Test failed  !!!\n"); 
		retval=1;
	} else {
		printf("Test passed !\n");
	}

	// Return 0 if the test passed
	return retval;

}

vector<string> split_string(
	std::string s,
	std::string delimiter
	){

	size_t pos = 0;
	std::string token;
	vector<string> res;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		res.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	res.push_back(s);
//	std::cout << "Splitted line length: " << res.size() << std::endl;
	return res;
}

string concat_string(
	vector<vector<price_depth>> pd, 
	std::string delimiter,
	int level
	){
	float multiple = MULTI;

	std::ostringstream ss;
	price_depth cur_pd; 
	int price;
	int size;

	vector<vector<price_depth>::iterator> iter_v, end_v;
	for (std::vector<vector<price_depth>>::iterator it = pd.begin(); it != pd.end(); ++it){
		iter_v.push_back(it->begin());
		end_v.push_back(it->end());
	}

	for (int i=0; i<level; i++){
		for (int ii = 0; ii < iter_v.size(); ++ii){
			if (iter_v[ii] < end_v[ii]){
				cur_pd = *(iter_v[ii]++);
				price = (std::round(((float)cur_pd.price)/UNIT)*(multiple*UNIT));
				size = cur_pd.size;
			}else{
				price = (ii == 0)? -9999999999: 9999999999;
				size = 0;
			}
			ss<< price <<delimiter<< size;
			if (!((i == level-1) && (ii == iter_v.size()-1))){
				ss << delimiter;
			}
		}
	}

	return std::string(ss.str());
}


void last_manager::check_update_last_price(
	vector<string> orderBook_split,
	Time tmstmp
){
	orderOp odop;
	ap_uint<1> bid, req_read=0;
	Message input_in;
	vector<pair<int, int>> *cache;

	int offset, vol_cur, price_cur, target_price;
	// bid at the end
	offset = 0;
	price_cur = stoi(*(orderBook_split.end()-2-4*offset)); 
	while (price_cur == -9999999999){
		price_cur = stoi(*(orderBook_split.end()-2-4*(++offset)));
		cache_lastb.clear();
	}
	vol_cur = stoi(*(orderBook_split.end()-1-4*offset));

	cache = &cache_lastb;
	if (price_cur < price_last_b){
		int vol_diff;
		bool br = false;
		while (true){
			if (cache->size() > cache_max_len){
					vol_diff = - cache->front().second;
					target_price = cache->front().first;
					cache->erase(cache->begin());
			}
			else
			{
				if ((price_cur > cache->back().first) || (cache->size()==0)){
					// expected price maybe still behind, add this unexpected new price
					br = true;
					vol_diff = vol_cur;
					target_price = price_cur;
				}else if (price_cur == cache->back().first){
					// hit the expected price
					br = true;
					vol_diff = vol_cur - cache->back().second;
					target_price = price_cur;
					cache->pop_back();
				}else{
					// expected price is removed from the book already, remove it untill get a new reasonable expected price
					vol_diff = - cache->back().second;
					target_price = cache->back().first;
					cache->pop_back();
				}
			}
			
			if (vol_diff != 0){
				input_in.price = (price_t)((float)(target_price)/MULTI); input_in.size = (qty_t)(vol_diff); input_in.orderID = 1000;
				bid = 1;
				odop = (vol_diff<0)? CHANGE: NEW;
				std::cout <<"Change on price level at the end: ";
				std::cout<<" Symbol: " << cur_symbol <<" OrderID: "<<input_in.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< input_in.price <<" Volume: "<< input_in.size <<" Read: "<<req_read<<endl;
				input_in.timestamp = tmstmp;
				input_in.symbol = cur_symbol;
				input_in.operation = odop;
				input_in.side = bid;
				// stream_in.write(input_in);
				stream_in[0] = input_in;
				order_book(
					// data
					stream_in,
					price_stream_out,
					// configuration inputs
					cur_symbol,
					read_max,
					1,
					// control input
					'v'  // void, run, halt, read book, clear, config symbol map | read_max 
				);
			}
			if (br) break;
		}
	}
	else if (price_cur > price_last_b) // price at the end overflow
	{
		cache->push_back( make_pair(price_last_b, vol_last_b));
		std::cout << "Price orderflow: "<< price_last_b << std::endl;
	}
	price_last_b = price_cur;
	vol_last_b = vol_cur;
	

	// ask at the end
	offset = 0;
	price_cur = stoi(*(orderBook_split.end()-4-4*offset)); 
	while (price_cur == 9999999999){
		price_cur = stoi(*(orderBook_split.end()-4-4*(++offset)));
		cache_lasta.clear();
	}
	vol_cur = stoi(*(orderBook_split.end()-3-4*offset));

	cache = &cache_lasta;
	if (price_cur > price_last_a){
		int vol_diff;
		bool br = false;
		while (true){
			if (cache->size() > cache_max_len){
					vol_diff = - cache->front().second;
					target_price = cache->front().first;
					cache->erase(cache->begin());
			}
			else
			{
				if ((price_cur < cache->back().first) || (cache->size()==0)){
					// expected price maybe still behind, add this unexpected new price
					br = true;
					vol_diff = vol_cur;
					target_price = price_cur;
				}else if (price_cur == cache->back().first){
					// hit the expected price
					br = true;
					vol_diff = vol_cur - cache->back().second;
					target_price = price_cur;
					cache->pop_back();
				}else{
					// expected price is removed from the book already, remove it untill get a new reasonable expected price
					vol_diff = - cache->back().second;
					target_price = cache->back().first;
					cache->pop_back();
				}
			}
			
			if (vol_diff != 0){
				input_in.price = (price_t)((float)(target_price)/MULTI); input_in.size = (qty_t)(vol_diff); input_in.orderID = 1000;
				bid = 0;
				odop = (vol_diff<0)? CHANGE: NEW;
				std::cout <<"Change on price level at the end: ";
				std::cout<<" Symbol: " << cur_symbol <<" OrderID: "<<input_in.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< input_in.price <<" Volume: "<< input_in.size <<" Read: "<<req_read<<endl;
				input_in.timestamp = tmstmp;
				input_in.symbol = cur_symbol;
				input_in.operation = odop;
				input_in.side = bid;
				// stream_in.write(input_in);
				stream_in[0] = input_in;
				order_book(
					// data
					stream_in,
					price_stream_out,
					// configuration inputs
					cur_symbol,
					read_max,
					1,
					// control input
					'v'  // void, run, halt, read book, clear, config symbol map | read_max 
				);
			}
			if (br) break;
		}
	}
	else if (price_cur < price_last_a) // price at the end overflow
	{
		cache->push_back( make_pair(price_last_a, vol_last_a));
		std::cout << "Price orderflow: "<< price_last_a << std::endl;
	}
	price_last_a = price_cur;
	vol_last_a = vol_cur;
	
}

