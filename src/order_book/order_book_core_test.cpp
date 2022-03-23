#include<iostream>
#include<fstream>
#include<string>
#include <vector>
#include <ctime>
#include <numeric>

#include "order_book.hpp"
// #include "order_book_core.hpp"
#include "order_book_core_IndexPop.hpp"

using namespace std;

int main()
{
	string pr, qty;
	char ab, op;
	ifstream new_file;
	vector<double> stat[4]; // stat_add, stat_chg, stat_rmv;
	clock_t start, end;
	double elapsed_ms;

	int id=0;
	order orderin;
	orderOp odop;
	ap_uint<1> bid, req_read;
	stream<price_depth> price_stream_out;
	price_depth price_read;
	
	new_file.open("SHIBUSDT_partial.txt",ios::in);
//	new_file.open("/home/yzhengbv/00-data/git/hft-system/data/SHIBUSDT_partial.txt",ios::in);
//	new_file.open("C:\\Users\\Leon Zheng\\git\\hft-system\\data\\SHIBUSDT_partial.txt",ios::in);

	if(!new_file)
	{
		cout<<"Sorry the file you are looking for is not available"<<endl;
		return -1;
	}
	
	while (!new_file.eof())
	{
		new_file>>ab>>op>>pr>>qty;
		orderin.price = (price_t)(stof(pr)*10000); orderin.size = (qty_t)(stof(qty)*1000); orderin.orderID = id;

		switch (op)
		{
			case 'A':
				odop = NEW;
				break;
			case 'E':
				odop = CHANGE;
				break;
			case 'R':
				odop = REMOVE;
				break;
			default:
				break;
		}

		bid = (ab == 'b')? 1: 0;
		// req_read = (id == 244)? 1: 0;
		req_read = (id%5 == 4)? 1: 0;

		// cout<< orderin.orderID << " " <<bid<<" "<<odop<<" "<< orderin.price <<" "<< orderin.size <<" "<<req_read<<endl;
		cout<< orderin.orderID << " " <<ab<<" "<<op<<" "<< orderin.price <<" "<< orderin.size <<" "<<req_read<<endl;

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
		if (req_read==1)
			stat[3].push_back(elapsed_ms);
		else
			stat[(int)odop].push_back(elapsed_ms);
		


		while (!price_stream_out.empty()){
			price_read = price_stream_out.read();
			cout << price_read.price << " " << price_read.size << endl;
		}
		id++;
	}
	new_file.close();


	for(int i=0; i<4; i++){
		double sum = std::accumulate(stat[i].begin(), stat[i].end(), 0.0);
		double mean = sum / stat[i].size();

		double sq_sum = std::inner_product(stat[i].begin(), stat[i].end(), stat[i].begin(), 0.0);
		double stdev = std::sqrt(sq_sum / stat[i].size() - mean * mean);

		printf("Time elapsed: Mean %0.3fus, Stddev: %0.3fus\n", mean, stdev);
	}

	return 0;
}
