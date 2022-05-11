
#include <vector>
#include <CL/cl2.hpp>
#include <iostream>
#include <fstream>
#include <CL/cl_ext_xilinx.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

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
    // size_t size_in_bytes = DATA_SIZE * sizeof(int);
    
    // // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // // using customized allocator for getting buffer alignment to 4k boundary
    // std::vector<int,aligned_allocator<int>> source_a(DATA_SIZE, 10);
    // std::vector<int,aligned_allocator<int>> source_b(DATA_SIZE, 32);
    // std::vector<int,aligned_allocator<int>> source_results(DATA_SIZE);

	messageManager messages_handler;

	map<string, symbol_t> symbol2hex = {{"AAPL", 0x4141504c20202020},
                                        {"AMZN", 0x414d5a4e20202020},
                                        {"GOOG", 0x474f4f4720202020},
                                        {"INTC", 0x494e544320202020},
                                        {"MSFT", 0x4d53465420202020},
                                        {"SPY" , 0x5350592020202020},
                                        {"TSLA", 0x54534c4120202020},
                                        {"NVDA", 0x4e56444120202020},
                                        {"AMD" , 0x414d442020202020},
                                        {"QCOM", 0x51434f4d20202020}};


    int MAX_WRITE = 1024, MAX_READ = 1024;
    int WRITE_LENGTH = 1, READ_LENGTH = LEVEL_TEST+2;
    Message *host_write_ptr;
    price_depth *host_read_ptr; // = (int*) malloc(MAX_LENGTH*sizeof(int));
    posix_memalign(&host_write_ptr, 4096, MAX_WRITE * sizeof(Message)); 
    posix_memalign(&host_read_ptr, 4096, MAX_READ * sizeof(price_depth)); 
    vector<Message> stream_data;
    vector<price_depth> read_price;


	// streamming ------------------------------

    std::cout << "Create streams" << std::endl;
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
    std::cout << "Configure orderbook registers..." << std::endl;
	symbol_t read_symbol = symbol2hex["AAPL"];
	ap_uint<8> read_max = 10;
	char instruction = 'A';
    std::cout << "[Register] write - read_symbol = " << read_symbol << std::endl;
    std::cout << "[Register] write - read_max    = " << read_max << std::endl;
    std::cout << "[Register] write - instruction = " << instruction << std::endl;
	clSetKernelArg(krnl_order_book, 2, sizeof(symbol_t), 	&read_symbol);
	clSetKernelArg(krnl_order_book, 3, sizeof(char), 		&read_max);		// 64 bits here instead of 8
	clSetKernelArg(krnl_order_book, 4, sizeof(char), 		&instruction);
	// 3rd and 4th arguments are not set as those are already specified when creating the streams

    usleep(100);
    std::cout << "Start up orderbook system..." << std::endl;
	char instruction = 'R';
    std::cout << "[Register] write - instruction = " << instruction << std::endl;
	clSetKernelArg(krnl_order_book, 4, sizeof(char), 		&instruction);
	// // Schedule kernel enqueue
	// clEnqueueTask(commands, kernel, . .. . );



    std::cout << "Initiate orderbook contents" << std::endl;

    std::cout << "Prepare data..." << std::endl;
    stream_data = messages_handler.init_book_messsages();
    WRITE_LENGTH = stream_data.size();
    // Aligning memory in 4K boundary
    // Fill the memory input 
    for(int i=0; i<WRITE_LENGTH; i++) {
        host_mem_ptr[i] = stream_data[i]; 
    }

	// Initiating the WRITE transfer
    std::cout << "Initiate write stream transfer" << std::endl;
	cl_stream_xfer_req wr_req {0};
	wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
	wr_req.priv_data = (void*)"orderwrite";
    int write_size = stream_data.size();
	clWriteStream(h2k_stream, host_write_ptr, write_size, &wr_req , &ret);

	// Checking the request completion, wait until finished
    std::cout << "Polling for completions of streaming.." << std::endl;
	cl_streams_poll_req_completions poll_req[1] {0}; // 1 Requests
	auto num_compl = 1;
	clPollStreams(device, poll_req, 1, 1, &num_compl, 100000, &ret);
    std::cout << "Polling complete" << std::endl;
	// Blocking API, waits for 2 poll request completion or 100'000ms, whichever occurs first


    usleep(1000);
	// Initiate the READ transfer
    std::cout << "Initiate read stream transfer" << std::endl;
	cl_stream_xfer_req rd_req {0};
	rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
	rd_req.priv_data = (void*)"bookread"; // You can think this as tagging the transfer with a name
	int max_read_size = MAX_READ;
    clReadStream(k2h_stream, host_read_ptr, max_read_size, &rd_req, &ret);

	// Checking the request completion, wait until finished
    std::cout << "Polling for completions of streaming.." << std::endl;
	cl_streams_poll_req_completions poll_req[1] {0}; // 1 Requests
	auto num_compl = 1;
	clPollStreams(device, poll_req, 2, 2, &num_compl, 100000, &ret);
    std::cout << "Polling complete" << std::endl;

    bool match;
	for (auto i=0; i<1; ++i) {
		if(rd_req.priv_data == poll_req[i].priv_data) { // Identifying the read transfer

		    // Getting read size, data size from kernel is unknown
            std::cout << "Orderbook from system for symbol: " << string((char*)&read_symbol, 8) <<std::endl;
		    ssize_t result_size=poll_req[i].nbytes;
            int lvls = result_size/sizeof(price_depth);
            read_price.clear();
            for (int i=0; i< lvls; i++){
                std::cout << "Price: " << host_read_ptr[i].price << " Size: " << host_read_ptr[i].size << std::endl;
                read_price.push_back(host_read_ptr[i]);
            }
            match = messages_handler.check_resultbook(read_price, read_symbol);
		}
	}

    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

    // std::cout << "Streaming orderbook messages.." << std::endl;
    // std::cout << "Preparing data..." << std::endl;
    // int WRITE_LENGTH = 1, READ_LENGTH = LEVEL_TEST+2;
    // Message *host_write_ptr, *host_read_ptr; // = (int*) malloc(MAX_LENGTH*sizeof(int));
    // // Aligning memory in 4K boundary
    // posix_memalign(&host_write_ptr,4096,WRITE_LENGTH*sizeof(Message)); 
    // posix_memalign(&host_read_ptr,4096,READ_LENGTH*sizeof(Message)); 
    // // Fill the memory input 
    // auto stream_data = messages_handler.generate_messages(WRITE_LENGTH);
    // if (stream_data.size() == WRITE_LENGTH){
    //     for(int i=0; i<WRITE_LENGTH; i++) {
    //         host_mem_ptr[i] = stream_data[i]; 
    //     }
    // }
	// ----------------------------------


    // // These commands will allocate memory on the Device. The cl::Buffer objects can
    // // be used to reference the memory locations on the device. 
    // cl::Buffer buffer_a(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,  
    //         size_in_bytes, source_a.data());
    // cl::Buffer buffer_b(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,  
    //         size_in_bytes, source_b.data());
    // cl::Buffer buffer_result(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
    //         size_in_bytes, source_results.data());
    
    // // Data will be transferred from system memory over PCIe to the FPGA on-board
    // // DDR memory.
    // q.enqueueMigrateMemObjects({buffer_a,buffer_b},0/* 0 means from host*/);

    // //set the kernel Arguments

    // krnl_vector_add.setArg(0,buffer_a);
    // krnl_vector_add.setArg(1,buffer_b);
    // krnl_vector_add.setArg(2,buffer_result);
    // krnl_vector_add.setArg(3,DATA_SIZE);

    // krnl_const_add.setArg(0,buffer_result);
    // //Launch the Kernel
    // q.enqueueTask(krnl_vector_add);
    // q.enqueueTask(krnl_const_add);
    // // The result of the previous kernel execution will need to be retrieved in
    // // order to view the results. This call will transfer the data from FPGA to
    // // source_results vector

    // q.enqueueMigrateMemObjects({buffer_result},CL_MIGRATE_MEM_OBJECT_HOST);

    // q.finish();

    // //Verify the result
    // int match = 0;
    // for (int i = 0; i < DATA_SIZE; i++) {
	// 		int host_result = source_a[i] + source_b[i] +1;
    //     if (source_results[i] != host_result) {
    //         printf(error_message.c_str(), i, host_result, source_results[i]);
    //         match = 1;
    //         break;
    //     }
    // }

    // std::cout << "TEST WITH TWO KERNELS " << (match ? "FAILED" : "PASSED") << std::endl; 
    // return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

}

