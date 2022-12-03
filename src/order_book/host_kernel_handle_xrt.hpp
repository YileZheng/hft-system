
#include "common.hpp"
#include "xrtApiHandle.hpp"


class KernelHandle: public xrtApiHandle{
    Message *my_host_write_ptr0;
    price_depth *my_host_read_ptr; 
    int MAX_WRITE, MAX_READ;
    int read_lvls;

    public:
    xrt::bo my_input0;
    xrt::bo my_output;


    KernelHandle(int max_write, int max_read, char* binaryName, char* kernel_name): xrtApiHandle(binaryName, kernel_name){
        MAX_WRITE = max_write, MAX_READ = max_read;

        my_input0 = xrt::bo(my_device, MAX_WRITE * sizeof(Message), XCL_BO_FLAGS_NONE, my_kernel.group_id(0));
        my_output = xrt::bo(my_device, MAX_READ * sizeof(price_depth), XCL_BO_FLAGS_NONE, my_kernel.group_id(1));
        std::cout << STR_PASSED << "auto my_kernel_i/oX = xrt::bo(my_device, SIZEX, XCL_BO_FLAGS_NONE, my_kernel.group_id(X) (=" << my_kernel.group_id(0) << "))" << std::endl;

        my_host_write_ptr0 = my_input0.map<Message*>();
        my_host_read_ptr = my_output.map<price_depth*>();
        std::cout << STR_PASSED << "auto my_kernel_i/oX_mapped = = my_kernel_i/oX.map<T*>()" << std::endl;

    }
    double boot_orderbook();
    double config_orderbook(ap_uint<8> axi_read_max);
    double read_orderbook(vector<price_depth> data_out, symbol_t axi_read_symbol);
    double new_orders(vector<Message> data_in);

    double run(
        // data
        vector<Message> data_in,
        vector<price_depth> data_out,

        // configuration inputs
        symbol_t axi_read_symbol,
        ap_uint<8> axi_read_max,

        // control input
        char axi_instruction  
        );
};

double KernelHandle::boot_orderbook(){
        symbol_t axi_read_symbol = 0;
        ap_uint<8> axi_read_max = read_lvls;

        // control input
        char axi_instruction = 'R'; 
        int axi_size = 0;

        clock_t start = clock();
        auto my_run = my_kernel(my_input0, my_output, axi_read_symbol, axi_read_max, axi_size, axi_instruction);
        std::cout << STR_PASSED << "Run my_kernel" << std::endl;

        // Waiting for all kernels to end
        std::cout << std::endl << STR_INFO << "Waiting kernel to end..." << std::endl << std::endl;
        my_run.wait();
        clock_t end = clock();

		double elapsed_ns = (double)(end-start)/CLOCKS_PER_SEC * 1000000000;  // in ns
        return elapsed_ns;
}


double KernelHandle::config_orderbook(
	// configuration inputs
	ap_uint<8> axi_read_max
    ){
        read_lvls = axi_read_max;

	    symbol_t axi_read_symbol = 0;

        // control input
        char axi_instruction = 'A'; 
        int axi_size = 0;

        clock_t start = clock();
        auto my_run = my_kernel(my_input0, my_output, axi_read_symbol, axi_read_max, axi_size, axi_instruction);
        std::cout << STR_PASSED << "Run my_kernel" << std::endl;

        // Waiting for all kernels to end
        std::cout << std::endl << STR_INFO << "Waiting kernel to end..." << std::endl << std::endl;

        my_run.wait();
        clock_t end = clock();

		double elapsed_ns = (double)(end-start)/CLOCKS_PER_SEC * 1000000000;  // in ns
        return elapsed_ns;
}

double KernelHandle::read_orderbook(
	vector<price_depth> data_out,
	symbol_t axi_read_symbol
    ){
        ap_uint<8> axi_read_max = read_lvls;

        // control input
        char axi_instruction = 'r'; 
        int axi_size = 0;

        clock_t start = clock();
        auto my_run = my_kernel(my_input0, my_output, axi_read_symbol, axi_read_max, axi_size, axi_instruction);
        std::cout << STR_PASSED << "Run my_kernel" << std::endl;

        // Waiting for all kernels to end
        std::cout << std::endl << STR_INFO << "Waiting kernel to end..." << std::endl << std::endl;

        my_run.wait();
        clock_t end = clock();

        my_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE, (2*read_lvls+2)*sizeof(price_depth), 0);
        std::cout << STR_PASSED << "Transfer back output data to host" << std::endl;

        data_out.clear();
        for (int i=0; i<2*read_lvls+2; i++){
            data_out[i] = my_host_read_ptr[i];
        }

		double elapsed_ns = (double)(end-start)/CLOCKS_PER_SEC * 1000000000;  // in ns
        return elapsed_ns;
}


double KernelHandle::new_orders(
	// data
	vector<Message> data_in
    ){
        
        symbol_t axi_read_symbol = 0;
        ap_uint<8> axi_read_max = read_lvls;

        int axi_size = data_in.size();
	    char axi_instruction   = 'v';
        //setting input data
        for(int i = 0 ; i < axi_size; i++){
            my_host_write_ptr0[i] = data_in[i];
        }

        my_input0.sync(XCL_BO_SYNC_BO_TO_DEVICE, axi_size*sizeof(Message), 0);
        std::cout << STR_PASSED << "Transfer input data to kernel" << std::endl;

        clock_t start = clock();
        auto my_run = my_kernel(my_input0, my_output, axi_read_symbol, axi_read_max, axi_size, axi_instruction);
        std::cout << STR_PASSED << "Run my_kernel" << std::endl;

        // Waiting for all kernels to end
        std::cout << std::endl << STR_INFO << "Waiting kernel to end..." << std::endl << std::endl;

        my_run.wait();
        clock_t end = clock();

		double elapsed_ns = (double)(end-start)/CLOCKS_PER_SEC * 1000000000;  // in ns
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
            my_host_write_ptr0[i] = data_in[i];
        }

        my_input0.sync(XCL_BO_SYNC_BO_TO_DEVICE, axi_size*sizeof(Message), 0);
        std::cout << STR_PASSED << "Transfer input data to kernel" << std::endl;

        clock_t start = clock();
        auto my_run = my_kernel(my_input0, my_output, axi_read_symbol, axi_read_max, axi_size, axi_instruction);
        std::cout << STR_PASSED << "Run my_kernel" << std::endl;

        // Waiting for all kernels to end
        std::cout << std::endl << STR_INFO << "Waiting kernel to end..." << std::endl << std::endl;

        my_run.wait();
        clock_t end = clock();

        if (axi_instruction == 'r'){
            my_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE, (2*read_lvls+2)*sizeof(price_depth), 0);
            std::cout << STR_PASSED << "Transfer back output data to host" << std::endl;

            data_out.clear();
            for (int i=0; i<2*read_lvls+2; i++){
                data_out[i] = my_host_read_ptr[i];
            }
        }
        
		double elapsed_ns = (double)(end-start)/CLOCKS_PER_SEC * 1000000000;  // in ns
        return elapsed_ns;

}
