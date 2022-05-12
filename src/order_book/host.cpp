
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


#define OCL_CHECK(error,call)                                       \
    call;                                                           \
    if (error != CL_SUCCESS) {                                      \
      printf("%s:%d Error calling " #call ", error code is: %d\n",  \
              __FILE__,__LINE__, error);                            \
      exit(EXIT_FAILURE);                                           \
    }                                       
	

// Forward declaration of utility functions included at the end of this file
std::vector<cl::Device> get_xilinx_devices();
char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb);


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


class StreamHandle: public ApiHandle{
    Message *host_write_ptr;
    price_depth *host_read_ptr; 
    int MAX_WRITE, MAX_READ;

    public:
    cl_stream h2k_stream;
    cl_stream k2h_stream;

    StreamHandle(int max_write, int max_read, char* binaryName, char* kernel_name, bool oooQueue): ApiHandle(binaryName, kernel_name, oooQueue){
        MAX_WRITE = max_write, MAX_READ = max_read;
        posix_memalign((void**)&host_write_ptr, 4096, MAX_WRITE * sizeof(Message)); 
        posix_memalign((void**)&host_read_ptr, 4096, MAX_READ * sizeof(price_depth)); 
        
        // streamming ------------------------------

        cl_int err;
        // Device connection specification of the stream through extension pointer
        cl_mem_ext_ptr_t  ext;  // Extension pointer
        ext.param = m_kernel;     // The .param should be set to kernel (cl_kernel type)
        ext.obj = nullptr;

        // The .flag should be used to denote the kernel argument
        // Create write stream for argument 0 of kernel
        std::cout << "Create host-kernel stream" << std::endl;
        ext.flags = 0;
        h2k_stream = clCreateStream(m_device_id, XCL_STREAM_READ_ONLY, CL_STREAM, &ext, &err);

        // Create read stream for argument 1 of kernel
        std::cout << "Create kernel-host stream" << std::endl;
        ext.flags = 1;
        k2h_stream = clCreateStream(m_device_id, XCL_STREAM_WRITE_ONLY, CL_STREAM, &ext,&err);

    }

    cl_streams_poll_req_completions* wait(int reqN, int timeout_m);

    void write_nb(vector<Message> data);

    void write(vector<Message> data);

    cl_stream_xfer_req read_nb();

    vector<price_depth> read();
    
};


char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
								{' ',' ',' ',' ', 'N','Z','M','A'},
								{' ',' ',' ',' ', 'G','O','O','G'}};
// config
symbol_t *symbol_map=(symbol_t*)symbols;


int main(int argc, char* argv[]) {
    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
    // ------------------------------------------------------------------------------------

    char* binaryFile;
    	if (argc==2) {
		binaryFile = argv[1];
		std::cout <<"Using FPGA binary file specfied through the command line: " << binaryFile << std::endl;
	}
	else {
		binaryFile = "../order_book.xclbin";
		std::cout << "No FPGA binary file specified through the command line, using:" << binaryFile <<std::endl;
	}
    int MAX_WRITE = 1024, MAX_READ = 1024;
    bool oooq=false;
    char* kernel_name = "order_book";
    StreamHandle s_handler(MAX_WRITE, MAX_READ, binaryFile, kernel_name, oooq);

    // ------------------------------------------------------------------------------------
    // Step 2: Create buffers and initialize test values
    // ------------------------------------------------------------------------------------
  
    // This call will get the kernel object from program. A kernel is an 
    // OpenCL function that is executed on the FPGA. 

	// Data preparation -----------------------------

    // Compute the size of array in bytes
    // size_t size_in_bytes = DATA_SIZE * sizeof(int);
    
    // // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // // using customized allocator for getting buffer alignment to 4k boundary
    // std::vector<int,aligned_allocator<int>> source_a(DATA_SIZE, 10);
    // std::vector<int,aligned_allocator<int>> source_b(DATA_SIZE, 32);
    // std::vector<int,aligned_allocator<int>> source_results(DATA_SIZE);

	messageManager messages_handler;
    symbol_t read_symbol;
    ap_uint<8> read_max;
    char instruction;
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


    int WRITE_LENGTH = 1, READ_LENGTH = LEVEL_TEST+2;
    vector<Message> stream_data;
    vector<price_depth> read_price;


	// axilite configuring ------------------------------

    cl_kernel krnl_order_book = s_handler.getKernel();
	// Set kernel non-stream argument (if any)
    std::cout << "Configure orderbook registers..." << std::endl;
	read_symbol = symbol2hex["AAPL"];
	read_max = 10;
	instruction = 'A';
    std::cout << "[Register] write - read_symbol = " << read_symbol << std::endl;
    std::cout << "[Register] write - read_max    = " << read_max << std::endl;
    std::cout << "[Register] write - instruction = " << instruction << std::endl;
	clSetKernelArg(krnl_order_book, 2, sizeof(symbol_t), 	&read_symbol);
	clSetKernelArg(krnl_order_book, 3, sizeof(char), 		&read_max);		// 64 bits here instead of 8
	clSetKernelArg(krnl_order_book, 4, sizeof(char), 		&instruction);
	// 3rd and 4th arguments are not set as those are already specified when creating the streams

    usleep(100);
    std::cout << "Start up orderbook system..." << std::endl;
	instruction = 'R';
    std::cout << "[Register] write - instruction = " << instruction << std::endl;
	clSetKernelArg(krnl_order_book, 4, sizeof(char), 		&instruction);
	// // Schedule kernel enqueue
	// clEnqueueTask(commands, kernel, . .. . );

    // streaming ---------------------------------------

    std::cout << "Initiate orderbook contents" << std::endl;

    std::cout << "Prepare data..." << std::endl;
    stream_data = messages_handler.init_book_messsages();

    std::cout << "Stream initial orderbook levls to the kernel" <<std::endl;
    s_handler.write(stream_data);

    usleep(1000);
    std::cout << "Read orderbook from system for symbol: " << string((char*)&read_symbol, 8) <<std::endl;
    read_price = s_handler.read();
    auto match = messages_handler.check_resultbook(read_price, read_symbol);

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
    //         host_write_ptr[i] = stream_data[i]; 
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


// ------------------------------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------------------------------
std::vector<cl::Device> get_xilinx_devices()
{
    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    err = cl::Platform::get(&platforms);
    cl::Platform platform;
    for (i = 0; i < platforms.size(); i++)
    {
        platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
        if (platformName == "Xilinx")
        {
            std::cout << "INFO: Found Xilinx Platform" << std::endl;
            break;
        }
    }
    if (i == platforms.size())
    {
        std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
        exit(EXIT_FAILURE);
    }

    //Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
    return devices;
}

char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb)
{
    if (access(xclbin_file_name.c_str(), R_OK) != 0)
    {
        printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
        exit(EXIT_FAILURE);
    }
    //Loading XCL Bin into char buffer
    std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    char *buf = new char[nb];
    bin_file.read(buf, nb);
    return buf;
}


cl_streams_poll_req_completions* StreamHandle::wait(int reqN, int timeout_m){
    cl_int err;
    // Checking the request completion, wait until finished
    std::cout << "Polling for completions of streaming.." << std::endl;
    cl_streams_poll_req_completions poll_req[reqN] {0}; // 1 Requests
    auto num_compl = reqN;
    OCL_CHECK(err, clPollStreams(m_device_id, poll_req, reqN, reqN, &num_compl, timeout_m, &err));
    std::cout << "Polling complete" << std::endl;
    // Blocking API, waits for 2 poll request completion or 100'000ms, whichever occurs first
    return poll_req;
}

void StreamHandle::write_nb(vector<Message> data){
    cl_int err;
    int size = data.size();
    if(size <= MAX_WRITE){
        for (int i=0; i<size; i++){
            host_write_ptr[i] = data[i];
        }
    }else{
        std::cout << "FAILED TEST - Stream from host to kernel overflow max mem buffer" << std::endl;
        exit(1);
    }

    // Initiating the WRITE transfer
    std::cout << "Initiate write stream transfer" << std::endl;
    cl_stream_xfer_req wr_req {0};
    wr_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    wr_req.priv_data = (void*)"orderwrite";
    OCL_CHECK(err, clWriteStream(h2k_stream, host_write_ptr, size, &wr_req , &err));

}

void StreamHandle::write(vector<Message> data){
    write_nb(data);
    wait(1, 100000);    // wait for 100000ms
}

cl_stream_xfer_req StreamHandle::read_nb(){
    cl_int err;
    // Initiate the READ transfer
    std::cout << "Initiate read stream transfer" << std::endl;
    cl_stream_xfer_req rd_req {0};
    rd_req.flags = CL_STREAM_EOT | CL_STREAM_NONBLOCKING;
    rd_req.priv_data = (void*)"bookread"; // You can think this as tagging the transfer with a name
    OCL_CHECK(err, clReadStream(k2h_stream, host_read_ptr, MAX_READ, &rd_req, &err));
    return rd_req;
}

vector<price_depth> StreamHandle::read(){
    auto rd_req = read_nb();
    auto poll_req = wait(1, 100000);
    
    vector<price_depth> read_data;
    for (auto i=0; i<1; ++i) {
        if(rd_req.priv_data == poll_req[i].priv_data) { // Identifying the read transfer

            // Getting read size, data size from kernel is unknown
            std::cout << "Read stream data from the kernel" <<std::endl;
            ssize_t result_size=poll_req[i].nbytes;
            int lvls = result_size/sizeof(price_depth);
            for (int i=0; i< lvls; i++){
                read_data.push_back(host_read_ptr[i]);
            }
        }
    }
    return read_data;
}
