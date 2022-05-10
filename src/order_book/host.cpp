
#include <vector>
#include <CL/cl2.hpp>
#include <iostream>
#include <fstream>
#include <CL/cl_ext_xilinx.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#include "common.hpp"
#include "host_message.hpp"

static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";

//Some Library functions to be used.
template <typename T>
struct aligned_allocator
{
  using value_type = T;
  T* allocate(std::size_t num)
  {
    void* ptr = nullptr;
    if (posix_memalign(&ptr,4096,num*sizeof(T)))
      throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
  }
  void deallocate(T* p, std::size_t num)
  {
    free(p);
  }
};


#define OCL_CHECK(error,call)                                       \
    call;                                                           \
    if (error != CL_SUCCESS) {                                      \
      printf("%s:%d Error calling " #call ", error code is: %d\n",  \
              __FILE__,__LINE__, error);                            \
      exit(EXIT_FAILURE);                                           \
    }                                       
	
namespace xcl {
std::vector<cl::Device> get_devices(const std::string& vendor_name) {

    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    OCL_CHECK(err, err = cl::Platform::get(&platforms));
    cl::Platform platform;
    for (i  = 0 ; i < platforms.size(); i++){
        platform = platforms[i];
        OCL_CHECK(err, std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err));
        if (platformName == vendor_name){
            std::cout << "Found Platform" << std::endl;
            std::cout << "Platform Name: " << platformName.c_str() << std::endl;
            break;
        }
    }
    if (i == platforms.size()) {
        std::cout << "Error: Failed to find Xilinx platform" << std::endl;
        exit(EXIT_FAILURE);
    }
   
    //Getting ACCELERATOR Devices and selecting 1st such device 
    std::vector<cl::Device> devices;
    OCL_CHECK(err, err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices));
    return devices;
}
   
std::vector<cl::Device> get_xil_devices() {
    return get_devices("Xilinx");
}

char* read_binary_file(const std::string &xclbin_file_name, unsigned &nb) 
{
    std::cout << "INFO: Reading " << xclbin_file_name << std::endl;

	if(access(xclbin_file_name.c_str(), R_OK) != 0) {
		printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
		exit(EXIT_FAILURE);
	}
    //Loading XCL Bin into char buffer 
    std::cout << "Loading: '" << xclbin_file_name.c_str() << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg (0, bin_file.end);
    nb = bin_file.tellg();
    bin_file.seekg (0, bin_file.beg);
    char *buf = new char [nb];
    bin_file.read(buf, nb);
    return buf;
}
};


char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
								{' ',' ',' ',' ', 'N','Z','M','A'},
								{' ',' ',' ',' ', 'G','O','O','G'}};
// config
symbol_t *symbol_map=(symbol_t*)symbols;


int main(int argc, char* argv[]) {
	{ // device, context, command queue, binary file, program
	const char* xclbinFilename;

	if (argc==2) {
		xclbinFilename = argv[1];
		std::cout <<"Using FPGA binary file specfied through the command line: " << xclbinFilename << std::endl;
	}
	else {
		xclbinFilename = "../order_book.xclbin";
		std::cout << "No FPGA binary file specified through the command line, using:" << xclbinFilename <<std::endl;
	}

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    devices.resize(1);

	
    // Creating Context and Command Queue for selected device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);

    // Load xclbin 
    std::cout << "Loading: '" << xclbinFilename << "'\n";
    std::ifstream bin_file(xclbinFilename, std::ifstream::binary);
    bin_file.seekg (0, bin_file.end);
    unsigned nb = bin_file.tellg();
    bin_file.seekg (0, bin_file.beg);
    char *buf = new char [nb];
    bin_file.read(buf, nb);
    
    // Creating Program from Binary File
    cl::Program::Binaries bins;
    bins.push_back({buf,nb});
    cl::Program program(context, devices, bins);
    
	}
    
    // This call will get the kernel object from program. A kernel is an 
    // OpenCL function that is executed on the FPGA. 
    cl::Kernel krnl_vector_add(program,"order_book");
	cl::Kernel krnl_const_add(program,"rtl_kernel_wizard_0");

    cl::Kernel krnl_order_book(program,"order_book");
	 
	// Data preparation -----------------------------

    // Compute the size of array in bytes
    size_t size_in_bytes = DATA_SIZE * sizeof(int);
    
    // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // using customized allocator for getting buffer alignment to 4k boundary
    std::vector<int,aligned_allocator<int>> source_a(DATA_SIZE, 10);
    std::vector<int,aligned_allocator<int>> source_b(DATA_SIZE, 32);
    std::vector<int,aligned_allocator<int>> source_results(DATA_SIZE);

	messageManager messages_handler;

	map<string, long> symbol2hex = {{"AAPL", 0x4141504c20202020},
									{"AMZN", 0x414d5a4e20202020},
									{"GOOG", 0x474f4f4720202020},
									{"INTC", 0x494e544320202020},
									{"MSFT", 0x4d53465420202020},
									{"SPY" , 0x5350592020202020},
									{"TSLA", 0x54534c4120202020},
									{"NVDA", 0x4e56444120202020},
									{"AMD" , 0x414d442020202020},
									{"QCOM", 0x51434f4d20202020}};

	symbol_t read_symbol = symbol2hex["AAPL"];
	ap_uint<8> read_max = 10;
	char instruction = 'A';


	// streamming ------------------------------

	// Device connection specification of the stream through extension pointer
	cl_mem_ext_ptr_t  ext;  // Extension pointer
	ext.param = krnl_order_book;     // The .param should be set to kernel (cl_kernel type)
	ext.obj = nullptr;

	// The .flag should be used to denote the kernel argument
	// Create write stream for argument 0 of kernel
	ext.flags = 0;
	cl_stream h2k_stream = clCreateStream(device, XCL_STREAM_READ_ONLY, CL_STREAM, &ext, &ret);

	// Create read stream for argument 1 of kernel
	ext.flags = 1;
	cl_stream k2h_stream = clCreateStream(device, XCL_STREAM_WRITE_ONLY, CL_STREAM, &ext,&ret);


	// Set kernel non-stream argument (if any)
	clSetKernelArg(krnl_order_book, 2, sizeof(symbol_t), 	&read_symbol);
	clSetKernelArg(krnl_order_book, 3, sizeof(char), 		&read_max);		// 64 bits here instead of 8
	clSetKernelArg(krnl_order_book, 4, sizeof(char), 		&instruction);
	// 3rd and 4th arguments are not set as those are already specified when creating the streams

	clSetKernelArg(krnl_order_book, 4, sizeof(char), 		&instruction);
	// // Schedule kernel enqueue
	// clEnqueueTask(commands, kernel, . .. . );

	// Initiate the READ transfer
	cl_stream_xfer_req rd_req {0};
	rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
	rd_req.priv_data = (void*)"bookread"; // You can think this as tagging the transfer with a name
	clReadStream(k2h_stream, host_read_ptr, max_read_size, &rd_req, &ret);

	// Initiating the WRITE transfer
	cl_stream_xfer_req wr_req {0};
	wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
	wr_req.priv_data = (void*)"orderwrite";
	clWriteStream(h2k_stream, host_write_ptr, write_size, &wr_req , &ret);


	// Checking the request completion, wait until finished
	cl_streams_poll_req_completions poll_req[2] {0, 0}; // 2 Requests

	auto num_compl = 2;
	clPollStreams(device, poll_req, 2, 2, &num_compl, 100000, &ret);
	// Blocking API, waits for 2 poll request completion or 100'000ms, whichever occurs first

	for (auto i=0; i<2; ++i) {
		if(rd_req.priv_data == poll_req[i].priv_data) { // Identifying the read transfer

		// Getting read size, data size from kernel is unknown
		ssize_t result_size=poll_req[i].nbytes;
		}
	}
	// ----------------------------------


    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device. 
    cl::Buffer buffer_a(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,  
            size_in_bytes, source_a.data());
    cl::Buffer buffer_b(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,  
            size_in_bytes, source_b.data());
    cl::Buffer buffer_result(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
            size_in_bytes, source_results.data());
    
    // Data will be transferred from system memory over PCIe to the FPGA on-board
    // DDR memory.
    q.enqueueMigrateMemObjects({buffer_a,buffer_b},0/* 0 means from host*/);

    //set the kernel Arguments

    krnl_vector_add.setArg(0,buffer_a);
    krnl_vector_add.setArg(1,buffer_b);
    krnl_vector_add.setArg(2,buffer_result);
    krnl_vector_add.setArg(3,DATA_SIZE);

    krnl_const_add.setArg(0,buffer_result);
    //Launch the Kernel
    q.enqueueTask(krnl_vector_add);
    q.enqueueTask(krnl_const_add);
    // The result of the previous kernel execution will need to be retrieved in
    // order to view the results. This call will transfer the data from FPGA to
    // source_results vector

    q.enqueueMigrateMemObjects({buffer_result},CL_MIGRATE_MEM_OBJECT_HOST);

    q.finish();

    //Verify the result
    int match = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
			int host_result = source_a[i] + source_b[i] +1;
        if (source_results[i] != host_result) {
            printf(error_message.c_str(), i, host_result, source_results[i]);
            match = 1;
            break;
        }
    }

    std::cout << "TEST WITH TWO KERNELS " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

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
				stream_in.write(input_in);
				order_book_system(
					// data
					stream_in,
					price_stream_out,
					// configuration inputs
					cur_symbol,
					read_max,
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
				stream_in.write(input_in);
				order_book_system(
					// data
					stream_in,
					price_stream_out,
					// configuration inputs
					cur_symbol,
					read_max,
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
