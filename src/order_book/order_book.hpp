#include <cfloat>
#include <iostream>
#include <bitset>
#include <hls_stream.h>
#include "ap_int.h"

#define CAPACITY 1024   // up to 4096, number of orders in each side
#define LEVELS 100       // up to 128, number of price levels in the order book
#define STOCKS 10       // up to 4096 (related to bookIndex, symbol_list)

using namespace hls;

typedef ap_uint<64> Time;	/*Time stamp for round-trip latency measurements*/

typedef signed int addr_index;
typedef ap_uint<48> symbol_t; 
typedef ap_ufixed<52, 32> price_t; 
typedef ap_int<32> qty_t;        /*Order size in hundreds*/
typedef ap_uint<32> orderID_t;    /*Unique ID for each order*/
typedef ap_uint<12> link_t;
    
enum orderOp{
    NEW = 0,
    CHANGE,
    REMOVE
};

struct price_lookup
{
	ap_uint<12> orderCnt = 0;
	price_t	price;
};

struct symbol_map
{
	symbol_t symbol;         // string of length 6 
	ap_uint<12> bookIndex;		// stock's correponding index in the order book, up to 4096 stocks
};

struct decoded_message {
	symbol_t symbol;		/*Stock symbol of maximum 6 characters */
	ap_uint<7> posLevel;	/*Position of price level up to 128 levels */
	ap_uint<12> posInLevel; /*Position in the price level, up to 4096 capacity per price level */
    price_t price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    qty_t size;        /*Order size in hundreds*/
    orderID_t orderID;    /*Unique ID for each order*/
    ap_uint<3> operation;   /*Order type: 0 - CHANGE ASK 	1 - CHANGE BID   */
};                          /*			  2 - INCOMING ASK 	3 - INCOMING BID */
                            /*		   	  4 - REMOVE ASK	5 - REMOVE BID	 */

struct order
{
    price_t price; /*Order price as an 20Q16 fixed-point number, upto 1'048'576 */
    qty_t size;        /*Order size in hundreds*/
    orderID_t orderID;    /*Unique ID for each order*/
};    

struct orderMessage
{
    order order_info;
    orderOp operation;
    ap_uint<1> side;
};

struct sub_order
{
    qty_t size;        /*Order size in hundreds*/
    orderID_t orderID;    /*Unique ID for each order*/
};   

struct price_depth{
    price_t price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    qty_t size;        /*Order size in hundreds*/
	};
struct price_depth_chain{
    price_t price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    qty_t size;        /*Order size in hundreds*/
    link_t next;
	};


struct order_depth{
    price_t price; /*Order price as an 20Q8 fixed-point number, upto 1'048'576 */
    qty_t size;        /*Order size in hundreds*/
    orderID_t orderID;    /*Unique ID for each order*/
	};

typedef order_depth top_of_order;

struct sockaddr_in {
    ap_uint<16>     port;   /* port in network byte order */
    ap_uint<32>     addr;   /* internet address */
};

struct metadata {
    sockaddr_in sourceSocket;
    sockaddr_in destinationSocket;
};

