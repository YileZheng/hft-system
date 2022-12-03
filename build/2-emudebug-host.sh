export ENABLE_KERNEL_DEBUG=true
export XCL_EMULATION_MODE=hw_emu
# export XCL_EMULATION_MODE=sw_emu

# add app_debug=true under [Debug] section

xgdb --args app.exe order_book.xclbin