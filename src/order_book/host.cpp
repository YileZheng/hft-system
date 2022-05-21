
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
#include "xrtApiHandle.hpp"


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
    char* kernel_name = "order_book";
    KernelHandle k_handler(MAX_WRITE, MAX_READ, binaryFile, kernel_name);

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
    ap_uint<8> read_max=LEVEL_TEST;
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
    double elapse_ns;

	// axilite configuring ------------------------------

    k_handler.config_orderbook(read_max);
    k_handler.boot_orderbook();


    // streaming ---------------------------------------

    std::cout << "Initiate orderbook contents" << std::endl;

    std::cout << "Prepare data..." << std::endl;
    stream_data = messages_handler.init_book_messsages();

    std::cout << "Stream initial orderbook levls to the kernel" <<std::endl;
    elapse_ns = k_handler.new_orders(stream_data);

    read_symbol = symbol2hex["AAPL"];
    std::cout << "Read orderbook from system for symbol: " << string((char*)&read_symbol, 8) <<std::endl;
    elapse_ns = k_handler.read_orderbook(read_price, read_symbol);
    auto match = messages_handler.check_resultbook(read_price, read_symbol);

    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
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

