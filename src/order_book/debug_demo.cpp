#include "order_book.hpp"
#include "order_book_core.hpp"

using namespace std;

int main(){
	cout << (price_t)UNIT << endl;
	cout << (price_t)0.02 << endl;
	cout<<(price_t)0.02 / (price_t)UNIT<< endl; 
	cout<<(unsigned int)hls::abs((price_t)0.02 / (price_t)UNIT)<< endl; 

	return 0;
}
