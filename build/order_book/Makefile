ndef = $(if $(value $(1)),,$(error $(1) must be set prior to running))

XF_PROJ_ROOT ?= /home/yzhengbv/00-data/git/hft-system
#XF_PROJ_ROOT ?= /home/leon/01-git/hft-system
#XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/Getting_Started/Vitis/example/src/*}')

TARGET := hw_emu
PLATFORM := xilinx_zcu102_base_202020_1
SRCDIR := $(XF_PROJ_ROOT)/src/order_book
SRCDIR_TOOL := $(XF_PROJ_ROOT)/src/tools
BUILD_DIR := $(shell pwd)/$(TARGET)
EMBEDDED_EXEC_SCRIPT := run_$(TARGET).sh
# EMBEDDED_PACKAGE_OUT := $(BUILD_DIR)package/sd_card.img
SDKTARGETSYSROOT := $(SYSROOT)
ROOTFS ?= $(EDGE_COMMON_SW)
KERNEL_NAME := order_book

HOST_SRC_CPP := $(SRCDIR)/host.cpp
HOST_SRC_CPP += $(SRCDIR)/host_message.cpp
# HOST_SRC_CPP += $(SRCDIR)/host_kernel_handle_cl.cpp
HOST_SRC_CPP += $(SRCDIR_TOOL)/event_timer.cpp
HOST_SRC_CPP += $(SRCDIR_TOOL)/xcl2.cpp

HOST_INCLUDE_PATH_FLAG := -I$(SRCDIR)
HOST_INCLUDE_PATH_FLAG += -I$(SRCDIR_TOOL)
HOST_INCLUDE_PATH_FLAG += -I$(SDKTARGETSYSROOT)/usr/include/xrt
HOST_INCLUDE_PATH_FLAG += -I$(XF_PROJ_ROOT)/include
HOST_INCLUDE_PATH_FLAG += -I$(XF_PROJ_ROOT)/../HLS_arbitrary_Precision_Types/include
HOST_INCLUDE_PATH_FLAG += -I/usr/include/xrt

HOST_LIBRARY_PATH_FLAG := -L$(SDKTARGETSYSROOT)/lib
HOST_LIBRARY_PATH_FLAG += -L$(SDKTARGETSYSROOT)/usr/lib

KERNEL_DEBUG_FLAG := --debug.chipscope $(KERNEL_NAME) --debug.protocol all

KERNEL_INCLUDE_PATH_FLAG := -I$(SRCDIR)

KERNEL_SRC_CPP :=  $(SRCDIR)/order_book.cpp
KERNEL_SRC_CPP +=  $(SRCDIR)/utils.cpp

PACKAGE_DATA_FILES_FLAG := --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_message_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/AMZN_2012-06-21_34200000_57600000_message_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/GOOG_2012-06-21_34200000_57600000_message_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/INTC_2012-06-21_34200000_57600000_message_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/MSFT_2012-06-21_34200000_57600000_message_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/AAPL_2012-06-21_34200000_57600000_orderbook_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/AMZN_2012-06-21_34200000_57600000_orderbook_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/GOOG_2012-06-21_34200000_57600000_orderbook_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/INTC_2012-06-21_34200000_57600000_orderbook_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file $(XF_PROJ_ROOT)/data/lobster/subset/MSFT_2012-06-21_34200000_57600000_orderbook_10.csv
PACKAGE_DATA_FILES_FLAG += --package.sd_file ../transfer_back.sh


run: build
ifeq ($(TARGET),hw)
	#cd $(BUILD_DIR) && ./app.exe
	echo "Copy the contents of the sd_card folder to an SD card, and run './app.exe' on the target platform."
else
	# cd $(BUILD_DIR) && XCL_EMULATION_MODE=$(TARGET) ./app.exe
	cd $(BUILD_DIR)/package && ./launch_$(TARGET).sh -g -no-reboot -run-app $(EMBEDDED_EXEC_SCRIPT) -forward-port 1440 22
endif

#build: host emconfig xclbin
build: $(BUILD_DIR)/app.exe $(BUILD_DIR)/${KERNEL_NAME}.xclbin $(BUILD_DIR)/package/sd_card.img

host: $(BUILD_DIR)/app.exe
$(BUILD_DIR)/app.exe:
	$(call ndef,SDKTARGETSYSROOT)
	mkdir -p $(BUILD_DIR)
	cp xrt.ini $(BUILD_DIR)
	cp run_$(TARGET).sh $(BUILD_DIR) 
	$(CXX) -Wall -g -std=c++11 $(HOST_SRC_CPP) -o $(BUILD_DIR)/app.exe $(HOST_INCLUDE_PATH_FLAG) $(HOST_LIBRARY_PATH_FLAG) \
		-lOpenCL -lpthread -lrt -lstdc++ #-lxilinxopencl -lxrt_core

xo: $(BUILD_DIR)/${KERNEL_NAME}.xo
$(BUILD_DIR)/${KERNEL_NAME}.xo:
#	cp $(XF_PROJ_ROOT)/proj/order_book_hls/top_orderbook_system/order_book_system.xo $(BUILD_DIR)/${KERNEL_NAME}.xo
	cd $(BUILD_DIR) && v++ -c -g -t ${TARGET} --platform $(PLATFORM) --config ../zcu102.cfg -k ${KERNEL_NAME} --hls.pre_tcl ../hls_time_patch.tcl \
		 $(KERNEL_INCLUDE_PATH_FLAG) \
		 $(KERNEL_SRC_CPP) \
		 -o $(BUILD_DIR)/${KERNEL_NAME}.xo 

xclbin: $(BUILD_DIR)/${KERNEL_NAME}.xclbin
$(BUILD_DIR)/${KERNEL_NAME}.xclbin: $(BUILD_DIR)/${KERNEL_NAME}.xo
	cd $(BUILD_DIR) && v++ -l -g -t ${TARGET} --platform $(PLATFORM) --config ../zcu102.cfg $(BUILD_DIR)/${KERNEL_NAME}.xo -o $(BUILD_DIR)/${KERNEL_NAME}.xclbin

package: $(BUILD_DIR)/package/sd_card.img
$(BUILD_DIR)/package/sd_card.img: $(BUILD_DIR)/app.exe $(BUILD_DIR)/${KERNEL_NAME}.xclbin
	$(call ndef,ROOTFS)
	cd $(BUILD_DIR) && v++ -p -t ${TARGET} --platform $(PLATFORM) --config ../zcu102.cfg ./${KERNEL_NAME}.xclbin \
		--package.out_dir ./package \
		--package.rootfs ${ROOTFS}/rootfs.ext4 \
		--package.sd_file ./${KERNEL_NAME}.xclbin \
		--package.sd_file ${ROOTFS}/Image \
		--package.sd_file ./xrt.ini \
		--package.sd_file ./app.exe \
		--package.sd_file ./run_$(TARGET).sh $(PACKAGE_DATA_FILES_FLAG)
	# cd $(BUILD_DIR) && mkdir package/sd_card/data && cp $(XF_PROJ_ROOT)/data/lobster/subset/*10.csv ./package/sd_card/data

clean:
	rm -rf $(BUILD_DIR) a.xlcbin ${KERNEL_NAME}* app.exe *json opencl* *log *summary _x xilinx* .run .Xil .ipcache *.jou

# Unless specified, use the current directory name as the v++ build target
#TARGET ?= $(notdir $(CURDIR))
