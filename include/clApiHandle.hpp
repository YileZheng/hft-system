
#include <CL/cl2.hpp>
#include <CL/cl_ext_xilinx.h>
#include "xcl2.hpp"


class clApiHandle{
	protected:
	cl::Device 		m_device;
	cl::Context 	m_context;
	cl::CommandQueue m_queue;
	cl::Kernel 		m_kernel;


	public:
	cl::Device& 		getDevice()	{	return m_device; 	}
	cl::Context& 		getContext(){	return m_context; }
	cl::CommandQueue& 	getQueue()	{	return m_queue; 		}
	cl::Kernel& 		getKernel()	{	return m_kernel;}

	clApiHandle(std::string xclbin_path, char* kernel_name){
			
		// Platform related operations
		std::vector<cl::Device> devices = xcl::get_xil_devices();
		m_device = devices[0];

		// Creating Context and Command Queue for selected Device
		m_context = cl::Context(m_device);
		m_queue = cl::CommandQueue(m_context, m_device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
		std::string devName = m_device.getInfo<CL_DEVICE_NAME>();
		printf("INFO: Found Device=%s\n", devName.c_str());

		cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
		devices.resize(1);
		cl::Program program(m_context, devices, xclBins);
		m_kernel = cl::Kernel(program, kernel_name);
		std::cout << "INFO: Kernel has been created" << std::endl;
	}
}