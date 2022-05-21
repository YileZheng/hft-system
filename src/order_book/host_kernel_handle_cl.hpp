#include "common.hpp"
#include "clApiHandle.hpp"
#include "event_timer.hpp"

class KernelHandle: public clApiHandle{
    EventTimer et;
	cl::Buffer buf_in;
	cl::Buffer buf_out;
	Message* host_write_ptr;
	price_depth* host_read_ptr;
    int MAX_WRITE, MAX_READ;
    int read_lvls;

	public:
	KernelHandle(int max_write, int max_read, char* binaryName, char* kernel_name): clApiHandle(std::string(binaryName), kernel_name){
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
		cl::Buffer buf_in(context,
							static_cast<cl_mem_flags>(CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR),
							MAX_WRITE * sizeof(Message),
							buf_in,
							NULL);
		cl::Buffer buf_out(context,
							static_cast<cl_mem_flags>(CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR),
							MAX_READ * sizeof(price_depth),
							buf_out,
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
		m_queue.enqueueUnmapMemObject(buf_in, host_write_ptr);
		m_queue.enqueueUnmapMemObject(buf_out, host_read_ptr);
		m_queue.finish();
	}

    double config_orderbook(ap_uint<8> axi_read_max);
    double boot_orderbook();
    double read_orderbook(vector<price_depth> data_out, symbol_t axi_read_symbol);
    double new_orders(vector<Message> data_in);
    double run(Message *data_in, price_depth *data_out, symbol_t axi_read_symbol, ap_uint<8> axi_read_max, int axi_size, char axi_instruction );

}



double KernelHandle::config_orderbook(
	ap_uint<8> axi_read_max 
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


double KernelHandle::boot_orderbook(){
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
	vector<price_depth> data_out, 
	symbol_t axi_read_symbol 
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
	q.enqueueMigrateMemObjects({buf_out}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &event_sp);
	clWaitForEvents(1, (const cl_event *)&event_sp);
	et.finish();
	
	data_out.clear();
	for (int i=0; i<2*read_lvls+2; i++){
		data_out[i] = host_read_ptr[i];
	}
	
	et.print();
	return elapsed_ns;
}


double KernelHandle::new_orders(
	// data
	vector<Message> data_in
){
	std::cout << "Feed new orders to orderbook system" << std::endl;
	symbol_t axi_read_symbol = 0;
	ap_uint<8> axi_read_max = read_lvls;
    char axi_instruction = 'v'; 
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

	et.print();
	return elapsed_ns;
}

double KernelHandle::run(
	// data
	vector<Message> data_in,
	vector<price_depth> data_out,

	// configuration inputs
	symbol_t axi_read_symbol,
	ap_uint<8> axi_read_max,

	// control input
	char axi_instruction  
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
		q.enqueueMigrateMemObjects({buf_out}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &event_sp);
		clWaitForEvents(1, (const cl_event *)&event_sp);
		et.finish();
		
		data_out.clear();
		for (int i=0; i<2*read_lvls+2; i++){
			data_out[i] = host_read_ptr[i];
		}
	}

	et.print();
	return elapsed_ns;
}