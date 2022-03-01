#include<iostream>
#include<fstream>
#include<string>

#include "order_book.hpp"
#include "order_book_core.hpp"

using namespace std;

int main()
{
	string pr, qty;
	char ab, op;
	ifstream new_file;

	int id=0;
	order orderin;
	orderOp odop;
	ap_uint<1> bid, req_read;
	stream<price_depth> price_stream_out;
	price_depth price_read;
	
	new_file.open("C:\\Users\\Leon Zheng\\git\\hft-system\\data\\SHIBUSDT_partial.txt",ios::in);

	if(!new_file)
	{
		cout<<"Sorry the file you are looking for is not available"<<endl;
		return -1;
	}
	
	while (!new_file.eof())
	{
		new_file>>ab>>op>>pr>>qty;
		orderin.price = (price_t)(stof(pr)*10000); orderin.size = (qty_t)stof(qty); orderin.orderID = id;

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
		req_read = (id == 244)? 1: 0;

		// cout<< orderin.orderID << " " <<bid<<" "<<odop<<" "<< orderin.price <<" "<< orderin.size <<" "<<req_read<<endl;
		cout<< orderin.orderID << " " <<ab<<" "<<op<<" "<< orderin.price <<" "<< orderin.size <<" "<<req_read<<endl;

		suborder_book(
			orderin,		// price size ID
			odop,		// new change remove
			bid,
			req_read,
			price_stream_out
		);

		while (!price_stream_out.empty()){
			price_read = price_stream_out.read();
			cout << price_read.price << " " << price_read.size << endl;
		}
		id++;
	}
	new_file.close();

	return 0;
}