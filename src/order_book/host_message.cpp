#include <string>
#include "host_message.hpp"

#define __isLittleEndian__
#define MULTI 10000
#define LV 10
#define UNIT 0.01
#define SLOTSIZE 10
#define READ_MAX 10
#define STOCK_TEST 3


void messageManager::close_files(){
	answer.close();
	result.close();
	for (int i=0; i<STOCK_TEST; i++){
		message_ls[i].close();
		orderbook_ls[i].close();
	}
}

int messageManager::get_symbol_loc(symbol_t target_symbol){
	for (int i=0; i<STOCK_TEST; i++){
		if (*(symbol_map+i) == target_symbol)
			return i;
	}
	return -1;
}

string messageManager::get_true_orderbook(symbol_t target_symbol){
	return last_orderbook_line_ls[get_symbol_loc(target_symbol)];
}

vector<Message> messageManager::init_book_messsages(){
	// stream inputs
	vector<Message> out_list;
	string pr, qty, oid, tstmp, ab, op;
	orderOp odop;
	ap_uint<1> bid, req_read;
	Message input_in;
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
				out_list.push_back(input_in);
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
	return out_list;
}

// check the length of <out_list> before use to assure not running the end of messages
// when not running to the EOF, output length should be <num>
// Otherwise, less than or equal to <num> 
vector<Message> messageManager::generate_messages(
	int num,  // number of order messages to retrive
	vector<orderOp> v_orderop
){
	vector<Message> out_list;
	string pr, qty, oid, tstmp, ab, op;
	orderOp odop;
	ap_uint<1> bid;
	Message input_in;
	string line, orderbook_line;
	vector<string> line_split;
	v_orderop.clear();
	while (neof){
		id++;
		int ii = id%STOCK_TEST;
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

			auto conpensate_list = last_ls[ii]->check_update_last_price(split_string(orderbook_line, string(",")),(Time)(stof(tstmp)*1000000000));
			if (!conpensate_list.empty())
				out_list.insert(out_list.end(), conpensate_list.begin(), conpensate_list.end());

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
			std::cout<<"Line: " << (id/STOCK_TEST) << " Symbol: " << string((char*)(symbol_map+ii), 8) <<" OrderID: "<<input_in.orderID << " Side: " <<bid<<" Type: "<<odop<<" Price: "<< input_in.price <<" Volume: "<< input_in.size <<endl;

			input_in.timestamp = (Time)(stof(tstmp)*1000000000);
			input_in.symbol = symbol_map[ii];
			input_in.operation = odop;
			input_in.side = bid;
			out_list.push_back(input_in);
			v_orderop.push_back(odop);
			
			last_orderbook_line_ls[ii] = orderbook_line;
			std::cout<<std::endl;
			
			if (--num == 0)		// to output a certain number of messages
				return out_list;
		}
		else{
			neof &= ~(ap_uint<STOCK_TEST>(0x1) << ii);
		}
	}
	std::cout << "All messages ends." << std::endl;
	return out_list;
}


bool messageManager::check_resultbook(
	vector<price_depth> price_depth_table,
	symbol_t target_symbol
){	
	price_depth price_read;
	vector<vector<price_depth>> resultbook;
	vector<price_depth> cur_v;
	bool match = false;
	std::cout << "Checking orderbook results... " << std::endl;
	if (price_depth_table.empty()){
		std::cout << STR_FAILED << "Got empty order book price levels of symbol " << std::string((char*)&target_symbol, 8) << std::endl;
		return match;
	}

	while (!price_depth_table.empty()){
		price_read = price_depth_table[0];
		price_depth_table.erase(price_depth_table.begin());

		std::cout <<"Price: " << price_read.price.to_float() << " Size: " << price_read.size << std::endl;
		if (price_read.price != 0){
			cur_v.push_back(price_read);
		}else {
			resultbook.push_back(cur_v);
			cur_v.clear();
		}
	}

	int loc = get_symbol_loc(target_symbol);
	string last_orderbook_line = last_orderbook_line_ls[loc];
	string s = concat_string(resultbook, string(","), level);
	if (s.compare(last_orderbook_line) != 0){
		std::cout <<"Symbol: " <<symbol_map[loc]<<": Result orderbook not match !!!!!!!!" <<std::endl;
		std::cout <<"Ground Truth: "<< last_orderbook_line << std::endl;
		std::cout <<"OrderBook:    "<< s << std::endl;
	}else{
		match = true;
	}
	result << s << endl;
	answer << last_orderbook_line << endl;

	return match;
}


bool messageManager::final_check(){

	close_files();
	// Compare the results file with the golden results
	
	int retval=0;
	retval = system((string("diff --brief -w ")+result_path+" "+answer_path).data());
	if (retval != 0) {
		printf("Test failed  !!!\n"); 
		return false;
	} else {
		printf("Test passed !\n");
		return true;
	}
}

// --------------- Lat manager ----------------------

vector<Message> last_manager::check_update_last_price(
	vector<string> orderBook_split,
	Time tmstmp
){
	vector<Message> out_list;
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
				out_list.push_back(input_in);
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
				out_list.push_back(input_in);
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

	return out_list;
}

//  -------------- utils ----------------------------

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

