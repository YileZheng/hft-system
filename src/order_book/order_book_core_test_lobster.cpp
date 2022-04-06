#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include <vector>
#include <ctime>
#include <numeric>

#include "order_book.hpp"
// #include "order_book_core.hpp"
// #include "order_book_core_IndexPop.hpp"
#include "order_book_core_IndexPop_opt.hpp"

using namespace std;

#define MULTI 10000
#define LV 10

vector<string> split_string(std::string s,std::string delimiter);
string concat_string(vector<vector<price_depth>> pd, std::string delimiter, int level);

int main()
{
	int level=LV;
	string message_path("data/lobster/AAPL_2012-06-21_34200000_57600000_message_10.csv.txt");
	string orderbook_path("data/lobster/AAPL_2012-06-21_34200000_57600000_orderbook_10.csv.txt"); 
	string result_path("result.csv");
	string answer_path("answer.csv");

	string pr, qty, oid, tstmp, ab, op;
	ifstream new_file, message, orderbook;
	ofstream result, answer;

	vector<double> stat[4]; // stat_add, stat_chg, stat_rmv;
	clock_t start, end;
	double elapsed_ms;

	int id=0;
	order orderin;
	orderOp odop;
	ap_uint<1> bid, req_read;
	stream<price_depth> price_stream_out;
	price_depth price_read;

	
	result.open(result_path, ios::out | ios::trunc);
	answer.open(answer_path, ios::out | ios::trunc);
	message.open(message_path, ios::in);
	orderbook.open(orderbook_path, ios::in);

	if((!orderbook) || (!message))
	{
		cout<<"Sorry the file you are looking for is not available"<<endl;
		return -1;
	}
	
	string line, orderbook_line;
	vector<string> line_split;

	while (!message.eof())
	{
		orderbook >> orderbook_line;
		message >> line;
		line_split = split_string(line, string(","));

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
		req_read = ((id++)%5 == 4)? 1: 0;

		cout<<"OrderID: "<<orderin.orderID << " Side: " <<ab<<" Type: "<<op<<" Price: "<< orderin.price <<" Volume: "<< orderin.size <<" Read: "<<req_read<<endl;

		start = clock();
		suborder_book(
			orderin,		// price size ID
			odop,		// new change remove
			bid,
			req_read,
			price_stream_out
		);
		end = clock();
		elapsed_ms = (double)(end-start)/CLOCKS_PER_SEC * 1000000;

		vector<vector<price_depth>> resultbook;
		vector<price_depth> cur_v;
		while (!price_stream_out.empty()){
			price_read = price_stream_out.read();
			cout << price_read.price << " " << price_read.size << endl;
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
			result << s << endl;
			answer << orderbook_line << endl;
		}
		else{
			stat[(int)odop].push_back(elapsed_ms);
		}
		
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
	double price;
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
			ss<< price <<delimiter<< size <<delimiter;
		}
	}

	for (int i=0; i<delimiter.size(); i++){
		ss<< "\b";
	}

	return std::string(ss.str());
}

