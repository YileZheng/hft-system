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

#include "suborder_book.hpp"

using namespace std;

#define MULTI 10000
#define LV 10
#define UNIT 0.01
#define SLOTSIZE 10
#define READ_MAX 10


vector<string> split_string(std::string s,std::string delimiter);
string concat_string(vector<vector<price_depth>> pd, std::string delimiter, int level);
void check_update_last_price(vector<string> orderBook_split, int price_lasta_init, int price_lastb_init, int vol_lasta_init, int vol_lastb_init);
int read_max = READ_MAX;

int main()
{
	int level=LV;
	string message_path("/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_message_10.csv");
	string orderbook_path("/home/yzhengbv/00-data/git/hft-system/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_orderbook_10.csv"); 
	string result_path("result.csv");
	string answer_path("answer.csv");

	string pr, qty, oid, tstmp, ab, op;
	ifstream new_file, message, orderbook;
	ofstream result, answer;
	string last_orderbook_line;

	vector<double> stat[4]; // stat_add, stat_chg, stat_rmv;
	clock_t start, end;
	double elapsed_ms;

	int id=0;
	order orderin;
	orderOp odop;
	ap_uint<1> bid, req_read;
	hls::stream<price_depth> price_stream_out;
	hls::stream<orderMessage> stream_in;
	price_depth price_read;
	orderMessage input_in;

	int pricea_init, priceb_init, vola_init, volb_init;


	result.open(result_path, ios::out | ios::trunc);
	answer.open(answer_path, ios::out | ios::trunc);
	message.open(message_path, ios::in);
	orderbook.open(orderbook_path, ios::in);

	if((!orderbook) || (!message))
	{
		std::cout<<"Sorry the file you are looking for is not available"<<endl;
		return -1;
	}
	
	string line, orderbook_line;
	vector<string> line_split;

	if (!message.eof()){
		id++;
		orderbook >> orderbook_line;
		message >> line;
		line_split = split_string(orderbook_line, string(","));
		std:: cout << "Initiate order book" << std::endl;
		bid = 1;
		odop = NEW;
		req_read = 0;
		for(vector<string>::iterator it = line_split.begin(); it != line_split.end(); it++){
			pr = *it;
			qty = *(++it);
			orderin.price = (price_t)(stof(pr)/MULTI); orderin.size = (qty_t)(stof(qty)); orderin.orderID = (size_t)1;
			bid++;
			input_in.order_info = orderin;
			input_in.operation = odop;
			input_in.side = bid;
			// stream_in.write(input_in);
			dut_suborder_book(
				input_in,
				read_max,
				req_read,
				price_stream_out
			);
		}

		pricea_init = stoi(*(line_split.end()-4));
		priceb_init = stoi(*(line_split.end()-2));
		vola_init = stoi(*(line_split.end()-3));
		volb_init = stoi(*(line_split.end()-1));
	}

	while (!message.eof())
	{
		last_orderbook_line = orderbook_line;
		orderbook >> orderbook_line;
		message >> line;
		line_split = split_string(line, string(","));
		check_update_last_price(split_string(orderbook_line, string(",")), pricea_init, priceb_init, vola_init, volb_init );

		tstmp = line_split[0];
		op = line_split[1];
		oid = line_split[2];
		qty = line_split[3];
		pr = line_split[4];
		ab = line_split[5];

		if ((op[0] == '5') || (op[0] == '7'))
			continue;

		orderin.price = (price_t)(stof(pr)/MULTI); orderin.size = (qty_t)(stof(qty)); orderin.orderID = (size_t)(stoi(oid));


		switch (op[0])
		{
			case '1':
				odop = NEW;
				break;
			case '2':
			case '3':
			case '4':
				odop = CHANGE;
				orderin.size = -orderin.size;
				break;
			default:
				break;
		}

		bid = (ab == "1")? 1: 0;
		req_read = ((id)%5 == 0)? 1: 0;
		std::cout<<"Line: " << id << " OrderID: "<<orderin.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< orderin.price <<" Volume: "<< orderin.size <<" Read: "<<req_read<<endl;

		start = clock();
		input_in.order_info = orderin;
		input_in.operation = odop;
		input_in.side = bid;
		// stream_in.write(input_in);
		dut_suborder_book(
			input_in,
			read_max,
			req_read,
			price_stream_out
		);
		end = clock();
		elapsed_ms = (double)(end-start)/CLOCKS_PER_SEC * 1000000;

		id++; 

		vector<vector<price_depth>> resultbook;
		vector<price_depth> cur_v;
		while (!price_stream_out.empty()){
			price_read = price_stream_out.read();
			std::cout << price_read.price << " " << price_read.size << endl;
			if (price_read.price != 0){
				cur_v.push_back(price_read);
			}else {
				resultbook.push_back(cur_v);
				cur_v.clear();
			}
		}

		if (req_read==1){
			stat[3].push_back(elapsed_ms);
			string s = concat_string(resultbook, string(","), level);
			if (s.compare(orderbook_line) != 0){
				std::cout << "Line: " <<id<<": Result orderbook not match !!!!!!!!" <<std::endl;
				std::cout <<"Ground Truth  "<< orderbook_line << std::endl;
				std::cout <<":OrderBook:   "<< s << std::endl;
			}
			result << s << endl;
			answer << orderbook_line << endl;
		}
		else{
			stat[(int)odop].push_back(elapsed_ms);
		}
		
		std::cout<<std::endl;
		
	}
	answer.close();
	result.close();
	message.close();
	orderbook.close();

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

//	for (int i=0; i<delimiter.size(); i++){
//		ss<< "\b";
//	}

	return std::string(ss.str());
}

void check_update_last_price(
	vector<string> orderBook_split,
	int price_lasta_init,
	int price_lastb_init,
	int vol_lasta_init,
	int vol_lastb_init

){
	static map<int, int> cache_last;
	static vector<pair<int, int>> cache_lasta, cache_lastb;
	static int price_last_b=price_lastb_init, price_last_a=price_lasta_init;
	static int vol_last_b=vol_lastb_init, vol_last_a=vol_lasta_init;
	static hls::stream<price_depth> price_stream_out;
	static hls::stream<orderMessage> stream_in;
	const int cache_max_len = 10;

	order orderin;
	orderOp odop;
	ap_uint<1> bid, req_read=0;
	orderMessage input_in;
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
				orderin.price = (price_t)((float)(target_price)/MULTI); orderin.size = (qty_t)(vol_diff); orderin.orderID = 1000;
				bid = 1;
				odop = (vol_diff<0)? CHANGE: NEW;
				std::cout <<"Change on price level at the end: ";
				std::cout<<" OrderID: "<<orderin.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< orderin.price <<" Volume: "<< orderin.size <<" Read: "<<req_read<<endl;
				input_in.order_info = orderin;
				input_in.operation = odop;
				input_in.side = bid;
				dut_suborder_book(
					input_in,
					read_max,
					req_read,
					price_stream_out
				);
				// stream_in.write(input_in);
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
				orderin.price = (price_t)((float)(target_price)/MULTI); orderin.size = (qty_t)(vol_diff); orderin.orderID = 1000;
				bid = 0;
				odop = (vol_diff<0)? CHANGE: NEW;
				std::cout <<"Change on price level at the end: ";
				std::cout<<" OrderID: "<<orderin.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< orderin.price <<" Volume: "<< orderin.size <<" Read: "<<req_read<<endl;
				input_in.order_info = orderin;
				input_in.operation = odop;
				input_in.side = bid;
				dut_suborder_book(
					input_in,
					read_max,
					req_read,
					price_stream_out
				);
				// stream_in.write(input_in);
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
