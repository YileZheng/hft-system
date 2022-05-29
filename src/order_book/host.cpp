
#include <vector>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.hpp"
#include "host_message.hpp"
// #include "host_kernel_handle_cl.hpp"
#include "event_timer.hpp"
#include "xcl2.hpp"


#define OCL_CHECK(error,call)                                       \
    call;                                                           \
    if (error != CL_SUCCESS) {                                      \
      printf("%s:%d Error calling " #call ", error code is: %d\n",  \
              __FILE__,__LINE__, error);                            \
      exit(EXIT_FAILURE);                                           \
    }                                       
	
class KernelHandle{
	char*    STR_ERROR   = "ERROR:   ";
	char*    STR_FAILED  = "FAILED:  ";
	char*    STR_PASSED  = "PASSED:  ";
	char*    STR_INFO    = "INFO:    ";
	char*    STR_USAGE   = "USAGE:   ";

    EventTimer et;

	cl::Buffer buf_in;
	cl::Buffer buf_out;
	Message* host_write_ptr;
	price_depth* host_read_ptr;

    int MAX_WRITE, MAX_READ;
    int read_lvls;	
	
	public:
	KernelHandle(int max_write, int max_read, cl::Context& m_context){
        MAX_WRITE = max_write, MAX_READ = max_read;


		et.add("Allocating memory buffer");
		int ret;
		ret = posix_memalign((void **)&host_write_ptr, 4096, MAX_WRITE * sizeof(Message));
		ret |= posix_memalign((void **)&host_read_ptr, 4096, MAX_READ * sizeof(price_depth));
		if (ret != 0) {
			std::cout << "Error allocating aligned memory!" << std::endl;
			exit(1);
		}

		et.finish();

		// Map our user-allocated buffers as OpenCL buffers using a shared
		// host pointer
		et.add("Map host buffers to OpenCL buffers");
		buf_in = cl::Buffer(m_context,
							static_cast<cl_mem_flags>(CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR),
							MAX_WRITE * sizeof(Message),
							host_write_ptr,
							NULL);
		buf_out = cl::Buffer(m_context,
							static_cast<cl_mem_flags>(CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR),
							MAX_READ * sizeof(price_depth),
							host_read_ptr,
							NULL);
		et.finish();


		// // Map our user-allocated buffers as OpenCL buffers using a shared
		// // host pointer
		// et.add("Allocate contiguous OpenCL buffers");
		// buf_in = cl::Buffer(m_context,
		// 					static_cast<cl_mem_flags>(CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR),
		// 					MAX_WRITE * sizeof(Message),
		// 					NULL,
		// 					NULL);

		// buf_out = cl::Buffer(m_context,
		// 					static_cast<cl_mem_flags>(CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR),
		// 					MAX_READ * sizeof(price_depth),
		// 					NULL,
		// 					NULL);

		// et.finish();

		// et.add("Map buffers to userspace pointers");
		// host_write_ptr = (Message *)m_queue.enqueueMapBuffer(buf_in,
		// 											CL_TRUE,
		// 											CL_MAP_WRITE,
		// 											0,
		// 											MAX_WRITE * sizeof(Message));
		// host_read_ptr = (price_depth *)m_queue.enqueueMapBuffer(buf_out,
		// 											CL_TRUE,
		// 											CL_MAP_READ,
		// 											0,
		// 											MAX_READ * sizeof(price_depth));

		// et.finish();
	}

	~KernelHandle(){
		// m_queue.enqueueUnmapMemObject(buf_in, host_write_ptr);
		// m_queue.enqueueUnmapMemObject(buf_out, host_read_ptr);
		// m_queue.finish();
	}

    double config_orderbook(ap_uint<8> axi_read_max, cl::CommandQueue& m_queue, cl::Kernel& m_kernel);
    double boot_orderbook(cl::CommandQueue& m_queue, cl::Kernel& m_kernel);
    double read_orderbook(vector<price_depth>& data_out, symbol_t axi_read_symbol, cl::CommandQueue& m_queue, cl::Kernel& m_kernel);
    double new_orders(vector<Message>& data_in, cl::CommandQueue& m_queue, cl::Kernel& m_kernel);
    double run(
	    // data
	    vector<Message>& data_in,
	    vector<price_depth>& data_out,

	    // configuration inputs
	    symbol_t axi_read_symbol,
	    ap_uint<8> axi_read_max,

	    // control input
	    char axi_instruction, 
		cl::CommandQueue& m_queue, cl::Kernel& m_kernel
    );

};


char symbols[STOCK_TEST][8] =  {{' ',' ',' ',' ','L','P', 'A','A'},
								{' ',' ',' ',' ', 'N','Z','M','A'},
								{' ',' ',' ',' ', 'G','O','O','G'}};
// config
symbol_t *symbol_map=(symbol_t*)symbols;

const char*    STR_ERROR   = "ERROR:   ";
const char*    STR_FAILED  = "FAILED:  ";
const char*    STR_PASSED  = "PASSED:  ";
const char*    STR_INFO    = "INFO:    ";
const char*    STR_USAGE   = "USAGE:   ";

int main(int argc, char* argv[]) {
    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
    // ------------------------------------------------------------------------------------

    char* binaryFile;
    char* file_dir;
    if (argc == 3){
        file_dir = argv[2];
        std::cout <<"Using order message file specfied through the command line: " << file_dir << std::endl;
        binaryFile = argv[1];
        std::cout <<"Using FPGA binary file specfied through the command line: " << binaryFile << std::endl;
    }else{
        file_dir = ".";
        std::cout <<"No order message file specfied through the command line, using: " << file_dir << std::endl;
        if (argc==2) {
            binaryFile = argv[1];
            std::cout <<"Using FPGA binary file specfied through the command line: " << binaryFile << std::endl;
        }
        else {
            binaryFile = "./order_book.xclbin";
            std::cout << "No FPGA binary file specified through the command line, using:" << binaryFile <<std::endl;
        }

    }
    int MAX_WRITE = 1024, MAX_READ = 1024;
    char* kernel_name = "order_book";
    // KernelHandle k_handler(MAX_WRITE, MAX_READ, binaryFile, kernel_name);

    std::string xclbin_path(binaryFile);
	cl::Device 		m_device;
	cl::Context 	m_context;
	cl::CommandQueue m_queue;
	cl::Kernel 		m_kernel;
    EventTimer et;
	cl::Buffer buf_in;
	cl::Buffer buf_out;
	Message* host_write_ptr;
	price_depth* host_read_ptr;
    // Platform related operations
    std::cout << STR_INFO << "Constructing cl API Handler..." << std::endl;
    std::vector<cl::Device> devices = m_tools::get_xil_devices();
    std::cout << STR_INFO << "Got devices" << std::endl;
    m_device = devices[0];

    // Creating Context and Command Queue for selected Device
    m_context = cl::Context(m_device);
    std::cout << STR_INFO << "Initialize context" << std::endl;
    m_queue = cl::CommandQueue(m_context, m_device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::cout << STR_INFO << "Initialize command queue" << std::endl;
    std::string devName = m_device.getInfo<CL_DEVICE_NAME>();
    printf("INFO: Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = m_tools::import_binary_file(xclbin_path);
    std::cout << STR_INFO << "Read hardware binary" << std::endl;
    devices.resize(1);
    cl::Program program(m_context, devices, xclBins);
    std::cout << STR_INFO << "Programmed device" << std::endl;
    m_kernel = cl::Kernel(program, kernel_name);
    std::cout << STR_INFO << "Kernel has been created: " << kernel_name << std::endl;

    et.add("Allocating memory buffer");
    int ret;
    ret = posix_memalign((void **)&host_write_ptr, 4096, MAX_WRITE * sizeof(Message));
    ret |= posix_memalign((void **)&host_read_ptr, 4096, MAX_READ * sizeof(price_depth));
    if (ret != 0) {
        std::cout << "Error allocating aligned memory!" << std::endl;
        exit(1);
    }

    et.finish();

    // Map our user-allocated buffers as OpenCL buffers using a shared
    // host pointer
    et.add("Map host buffers to OpenCL buffers");
    buf_in = cl::Buffer(m_context,
                        static_cast<cl_mem_flags>(CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR),
                        MAX_WRITE * sizeof(Message),
                        host_write_ptr,
                        NULL);
    buf_out = cl::Buffer(m_context,
                        static_cast<cl_mem_flags>(CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR),
                        MAX_READ * sizeof(price_depth),
                        host_read_ptr,
                        NULL);
    et.finish();

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

	messageManager messages_handler(file_dir);
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
    KernelHandle k_handler(MAX_WRITE, MAX_READ, m_context);

	// axilite configuring ------------------------------

    k_handler.config_orderbook(read_max, m_queue, m_kernel);

	// std::cout << "Configuring orderbook system" << std::endl;
	// symbol_t axi_read_symbol = 0;
	// int axi_size = 0;
    // char axi_instruction = 'A'; 
	// ap_uint<8> axi_read_max = read_max;


	// // Set vadd kernel arguments
	// et.add("Set kernel arguments");
	// m_kernel.setArg(0, buf_in);
	// m_kernel.setArg(1, buf_out);
	// m_kernel.setArg(2, axi_read_symbol);
	// m_kernel.setArg(3, axi_read_max);
	// m_kernel.setArg(4, axi_size);
	// m_kernel.setArg(5, axi_instruction);
	// et.finish();

	// cl::Event event_sp;
	// et.add("Enqueue task & wait");
	// m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	// clWaitForEvents(1, (const cl_event *)&event_sp);
	// et.finish();
	// elapse_ns = (double)et.last_duration() * 1000000;

	// et.print();


    k_handler.boot_orderbook(m_queue, m_kernel);
	// std::cout << "Booting orderbook system" << std::endl;
    // axi_instruction = 'R'; 

	// // Set vadd kernel arguments
	// et.add("Set kernel arguments");
	// m_kernel.setArg(0, buf_in);
	// m_kernel.setArg(1, buf_out);
	// m_kernel.setArg(2, axi_read_symbol);
	// m_kernel.setArg(3, axi_read_max);
	// m_kernel.setArg(4, axi_size);
	// m_kernel.setArg(5, axi_instruction);
	// et.finish();

	// et.add("Enqueue task & wait");
	// m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	// clWaitForEvents(1, (const cl_event *)&event_sp);
	// et.finish();
	// elapse_ns = (double)et.last_duration() * 1000000;

	// et.print();


    // streaming ---------------------------------------

    std::cout << "Initiate orderbook contents" << std::endl;

    std::cout << "Prepare data..." << std::endl;
    stream_data = messages_handler.init_book_messsages();

    std::cout << "Stream initial orderbook levls to the kernel" <<std::endl;
    elapse_ns = k_handler.new_orders(stream_data, m_queue, m_kernel);

    read_symbol = symbol2hex["AAPL"];
    std::cout << "Read orderbook from system for symbol: " << string((char*)&read_symbol, 8) <<std::endl;
    elapse_ns = k_handler.read_orderbook(read_price, read_symbol, m_queue, m_kernel);
    auto match = messages_handler.check_resultbook(read_price, read_symbol);

    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
}





double KernelHandle::config_orderbook(
	ap_uint<8> axi_read_max,

	cl::CommandQueue& m_queue, cl::Kernel& m_kernel 
){
	std::cout << "Configuring orderbook system" << std::endl;
	symbol_t axi_read_symbol = 0;
	int axi_size = 0;
    char axi_instruction = 'A'; 

	read_lvls = axi_read_max;

	// Set vadd kernel arguments
	et.add("Set kernel arguments");
	m_kernel.setArg(0, buf_in);
	m_kernel.setArg(1, buf_out);
	m_kernel.setArg(2, axi_read_symbol);
	m_kernel.setArg(3, axi_read_max);
	m_kernel.setArg(4, axi_size);
	m_kernel.setArg(5, axi_instruction);
	et.finish();

	cl::Event event_sp;
	et.add("Enqueue task & wait");
	m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;

	et.print();
	return elapsed_ns;
}


double KernelHandle::boot_orderbook(
	cl::CommandQueue& m_queue, cl::Kernel& m_kernel
){
	std::cout << "Booting orderbook system" << std::endl;
	symbol_t axi_read_symbol = 0;
	ap_uint<8> axi_read_max = read_lvls;
	int axi_size = 0;
    char axi_instruction = 'R'; 

	// Set vadd kernel arguments
	et.add("Set kernel arguments");
	m_kernel.setArg(0, buf_in);
	m_kernel.setArg(1, buf_out);
	m_kernel.setArg(2, axi_read_symbol);
	m_kernel.setArg(3, axi_read_max);
	m_kernel.setArg(4, axi_size);
	m_kernel.setArg(5, axi_instruction);
	et.finish();

	cl::Event event_sp;
	et.add("Enqueue task & wait");
	m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;

	et.print();
	return elapsed_ns;
}


double KernelHandle::read_orderbook(
	vector<price_depth>& data_out, 
	symbol_t axi_read_symbol,

	cl::CommandQueue& m_queue, cl::Kernel& m_kernel 
){
	std::cout << "Read orderbook from kernel" << std::endl;
	ap_uint<8> axi_read_max = read_lvls;
	int axi_size = 0;
    char axi_instruction = 'r'; 

	// Set vadd kernel arguments
	et.add("Set kernel arguments");
	m_kernel.setArg(0, buf_in);
	m_kernel.setArg(1, buf_out);
	m_kernel.setArg(2, axi_read_symbol);
	m_kernel.setArg(3, axi_read_max);
	m_kernel.setArg(4, axi_size);
	m_kernel.setArg(5, axi_instruction);
	et.finish();

	cl::Event event_sp;
	et.add("Enqueue task & wait");
	m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;

	// Migrate memory back from device
	et.add("Read back computation results");
	m_queue.enqueueMigrateMemObjects({buf_out}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	
	data_out.clear();
	for (int i=0; i<2*read_lvls+2; i++){
		data_out.push_back(host_read_ptr[i]);
	}
	
	et.print();
	return elapsed_ns;
}


double KernelHandle::new_orders(
	// data
	vector<Message>& data_in,

	cl::CommandQueue& m_queue, cl::Kernel& m_kernel
){
	std::cout << "Feed new orders to orderbook system" << std::endl;
	symbol_t axi_read_symbol = 0;
	ap_uint<8> axi_read_max = read_lvls;
    char axi_instruction = 'v'; 
	int axi_size = data_in.size();

	//setting input data
	for(int i = 0 ; i < axi_size; i++){
		host_write_ptr[i] = data_in[i];
     	std::cout<< "Send message - Symbol: " << std::string((char*)&host_write_ptr[i].symbol, 8) <<" OrderID: "<<host_write_ptr[i].orderID << " Side: " <<host_write_ptr[i].side<<" Type: "<<host_write_ptr[i].operation<<" Price: "<< host_write_ptr[i].price <<" Volume: "<< host_write_ptr[i].size<<std::endl;
	}

	// Set vadd kernel arguments
	et.add("Set kernel arguments");
	m_kernel.setArg(0, buf_in);
	m_kernel.setArg(1, buf_out);
	m_kernel.setArg(2, axi_read_symbol);
	m_kernel.setArg(3, axi_read_max);
	m_kernel.setArg(4, axi_size);
	m_kernel.setArg(5, axi_instruction);
	et.finish();

	cl::Event event_sp;
	// Send the buffers down to the Alveo card
	et.add("Memory object migration enqueue");
	m_queue.enqueueMigrateMemObjects({buf_in}, 0, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);

	et.add("Enqueue task & wait");
	m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;

	et.print();
	return elapsed_ns;
}

double KernelHandle::run(
	// data
	vector<Message>& data_in,
	vector<price_depth>& data_out,

	// configuration inputs
	symbol_t axi_read_symbol,
	ap_uint<8> axi_read_max,

	// control input
	char axi_instruction, 

	cl::CommandQueue& m_queue, cl::Kernel& m_kernel
){

	int axi_size = data_in.size();
	//setting input data
	for(int i = 0 ; i < axi_size; i++){
		host_write_ptr[i] = data_in[i];
	}

	// Set vadd kernel arguments
	et.add("Set kernel arguments");
	m_kernel.setArg(0, buf_in);
	m_kernel.setArg(1, buf_out);
	m_kernel.setArg(2, axi_read_symbol);
	m_kernel.setArg(3, axi_read_max);
	m_kernel.setArg(4, axi_size);
	m_kernel.setArg(5, axi_instruction);
	et.finish();

	cl::Event event_sp;
	// Send the buffers down to the Alveo card
	et.add("Memory object migration enqueue");
	m_queue.enqueueMigrateMemObjects({buf_in}, 0, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);

	et.add("Enqueue task & wait");
	m_queue.enqueueTask(m_kernel, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	double elapsed_ns = (double)et.last_duration() * 1000000;

	// Migrate memory back from device
	if (axi_instruction == 'r'){
		et.add("Read back computation results");
		m_queue.enqueueMigrateMemObjects({buf_out}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &event_sp);
		clWaitForEvents(1, (const cl_event *)&event_sp);
		et.finish();
		
		data_out.clear();
		for (int i=0; i<2*read_lvls+2; i++){
			data_out.push_back(host_read_ptr[i]);
		}
	}

	et.print();
	return elapsed_ns;
}
