#include <xrt/experimental/xrt_device.h>
#include <xrt/experimental/xrt_kernel.h>
#include <xrt/experimental/xrt_bo.h>

class xrtApiHandle{
    protected:
	
	const char*    STR_ERROR   = "ERROR:   ";
	const char*    STR_FAILED  = "FAILED:  ";
	const char*    STR_PASSED  = "PASSED:  ";
	const char*    STR_INFO    = "INFO:    ";
	const char*    STR_USAGE   = "USAGE:   ";

	xrt::kernel	my_kernel;
	xrt::device my_device;

	public:
	xrt::device& get_device(){return my_device;}
	xrt::kernel& get_kernel(){return my_kernel;}

	xrtApiHandle(char* xclbinFilename, char* kernelName ){

		unsigned int dev_index = 0;
		auto my_device = xrt::device(dev_index);
		std::cout << STR_PASSED << "auto my_device = xrt::device(" << dev_index << ")" << std::endl;

		auto xclbin_uuid = my_device.load_xclbin(xclbinFilename);
		std::cout << STR_PASSED << "auto xclbin_uuid = my_device.load_xclbin(" << xclbinFilename << ")" << std::endl;

		my_kernel = xrt::kernel(my_device, xclbin_uuid, kernelName);
		std::cout << STR_PASSED << "auto my_kernel = xrt::kernel(my_device, xclbin_uuid, \"" << kernelName << "\")" << std::endl;
	}
};
