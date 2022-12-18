/*
* Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>

#include<iostream>
#include<fstream>
#include<sstream>
// #include<string>
#include<vector>
#include<ctime>
#include<numeric>
#include<cmath>

#include "model.hpp"

using namespace std;

typedef uint32_t u32;


#define XAdder_apint_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset >> 2)) = (u32)(Data)
#define XAdder_apint_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset >> 2))

/* kernel configuration related definitions */
#define KRNL_CONF_ADDR_A_DATA       0x10
#define KRNL_CONF_ADDR_B_DATA       0x1c
#define KRNL_CONF_ADDR_AP_CTRL      0x00
#define AXI_SGNL_GPIO_ADDR			0xA0000000
#define KRNL_REG_BASE_ADDR          0xA0020000
/* transfer related definitions */
#define RD_ADDR                     0x10000000
#define WR_ADDR                     0x20000000
#define INTI_DATA_LEN               2*1024*1024
#define TRANS_LEN                   64
#define TEST_LOOP_NUM               3

#define lluint long long unsigned int


vector<pricebase_t> split_string(std::string s,std::string delimiter);
vector<pricebase_t> slicing(vector<pricebase_t>& arr, int X, int Y);
string concat_string(vector<pricebase_t> vin, std::string delimiter);


template <typename T, int D0, int D1>  // changed to 1d array
void vector1d2array1d(
    vector<T>   vin,
    T* arr
){
    for (int y = 0; y < D0; y++){ 
        for (int x = 0; x < D1; x++){
            arr[y * D1 + x] = vin[y*D1 + x];
        }
    }
}

template <typename T, int D0>
bool approx_equal(
    vector<T>   vin,
    T* arr
){
    float eps = 5e-4;
    assert(vin.size() == D0);
    for (int i = 0; i < D0; i++){
        if ((abs(vin[i] - arr[i]) > eps) && (arr[i] == arr[i])){  // equal and arr is not nan
            cout << "Comparison wrong: " << endl;
            cout << "Answer" << '\t' << "Outputs" << endl;
            for (int ii = 0; ii < D0; ii++)
                cout << vin[ii] << '\t' << arr[ii] << endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    printf("\n\r--- Enterring main() (ACE Interface)---\r\n");

    // int dh = open("/dev/mem", O_RDWR | O_SYNC);
    int dh = open("/dev/mem", O_RDWR);
    pricebase_t* krnl_reg_base = (pricebase_t*)mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, dh, KRNL_REG_BASE_ADDR);
	pricebase_t* axi_sgnl_gpio = (pricebase_t*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dh, AXI_SGNL_GPIO_ADDR);
    pricebase_t* rd_addr       = (pricebase_t*)mmap(NULL, 4*1024, PROT_READ | PROT_WRITE, MAP_SHARED, dh, RD_ADDR);
    pricebase_t* wr_addr       = (pricebase_t*)mmap(NULL, 4*1024, PROT_READ | PROT_WRITE, MAP_SHARED, dh, WR_ADDR);

    string data_dir("data/");
	string input_path(data_dir+"input_s.csv");
	string output_path(data_dir+"output_pmodel_s.csv");
	string result_path("result.csv");
	string answer_path("answer.csv");

	ifstream inputs, outputs;
	ofstream result, answer;

	// clock_t start, end;
	// double elapsed_ms;
	// double elapsed_sum = 0;
	struct timespec start = {0, 0};
	struct timespec end = {0, 0};
	lluint elapsed_sum = 0;
	lluint elapsed_ms;

	result.open(result_path, ios::out | ios::trunc);
	answer.open(answer_path, ios::out | ios::trunc);
	inputs.open(input_path, ios::in);
	outputs.open(output_path, ios::in);

	if((!inputs) || (!outputs)){
		std::cout<<"Sorry the file you are looking for is not available"<<endl;
		return -1;
	}
	
	string iline, oline;
	vector<pricebase_t> line_split, line_splito;

	int retval=0;
    int id = 0;
	char bufi[2096] = {0};
	char bufo[1024] = {0};
	pricebase_t* pricein, predout;
	while (inputs.getline(bufi, sizeof(bufi), '\n')){
		// Read Data from the files and convert from string into numeric values (1D array of shape (INPUT_LENGTH * INPUT_SIZE))
		iline = string(bufi);
        outputs.getline(bufo, sizeof(bufo), '\n');
		oline = string(bufo);

		line_splito = split_string(oline, string(","));
		line_split = split_string(iline, string(","));
        vector1d2array1d<pricebase_t, INPUT_LENGTH, INPUT_SIZE>(line_split, rd_addr);

        // Configure and start up the kernel
		/* set axi signals for cache coherency */
		XAdder_apint_WriteReg(axi_sgnl_gpio, 0, 0x4002FF0);

        /* set read address */
        XAdder_apint_WriteReg(krnl_reg_base, KRNL_CONF_ADDR_A_DATA, RD_ADDR);
        XAdder_apint_WriteReg(krnl_reg_base, KRNL_CONF_ADDR_A_DATA + 4, 0x0);

        /* set write address */
        XAdder_apint_WriteReg(krnl_reg_base, KRNL_CONF_ADDR_B_DATA, WR_ADDR);
        XAdder_apint_WriteReg(krnl_reg_base, KRNL_CONF_ADDR_B_DATA + 4, 0x0);

        /* start the kernel */
        // start = clock();
        uint control = (XAdder_apint_ReadReg(krnl_reg_base, KRNL_CONF_ADDR_AP_CTRL) & 0x80);
        XAdder_apint_WriteReg(krnl_reg_base, KRNL_CONF_ADDR_AP_CTRL, control | 0x01);

		clock_gettime(CLOCK_REALTIME, &start);
        while (((XAdder_apint_ReadReg(krnl_reg_base, KRNL_CONF_ADDR_AP_CTRL) >> 1) & 0x1) == 0) {
            // wait until kernel finish running
        }
		// end = clock();
		clock_gettime(CLOCK_REALTIME, &end);
		// elapsed_ms = (double)(end-start) / CLOCKS_PER_SEC * 1000000;
		elapsed_ms = end.tv_nsec-start.tv_nsec;
        // printf("duration %d is %lluns\n", i, duration);

        /* Output result saving */
		answer << concat_string(line_splito, string(",")) << std::endl;
		result << concat_string(vector<pricebase_t>(wr_addr, wr_addr + OUTPUT_LENGTH), string(",")) << std::endl;
		std::cout << "Line " << id; 

        /* Output result checking */
        if (!approx_equal<pricebase_t, OUTPUT_LENGTH>(slicing(line_splito,  line_splito.size()-OUTPUT_LENGTH, line_splito.size()-1), wr_addr)){
            retval = -1;
			std::cout << " Fail" << std::endl;
            break;
		} else{
			std::cout << " Pass" << std::endl;
		}
        elapsed_sum += elapsed_ms;
        id++;

    }

	inputs.close();
	outputs.close();
	answer.close();
	result.close();

	cout << "Averaged Running Time: " << elapsed_sum / id << endl;
    printf("\n\r--- Exiting main() ---\r\n");
	// Return 0 if the test passed
	return retval;
}

vector<pricebase_t> slicing(vector<pricebase_t>& arr,
                    int X, int Y)
{

    // Starting and Ending iterators
    auto start = arr.begin() + X;
    auto end = arr.begin() + Y + 1;

    // To store the sliced vector
    vector<pricebase_t> result(Y - X + 1);

    // Copy vector using copy function()
    copy(start, end, result.begin());

    // Return the final sliced vector
    return result;
}


vector<pricebase_t> split_string(
	std::string s,
	std::string delimiter
	){

	size_t pos = 0;
	std::string token;
	vector<pricebase_t> res;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		res.push_back(pricebase_t(stof(token)));
		s.erase(0, pos + delimiter.length());
	}
	res.push_back(pricebase_t(stof(s)));
//	std::cout << "Splitted line length: " << res.size() << std::endl;
	return res;
}

string concat_string(
	vector<pricebase_t> vin,
	std::string delimiter
	){

	std::ostringstream ss;

	for (std::vector<pricebase_t>::iterator it = vin.begin(); it != vin.end(); ++it){
		ss << (*it);
		if (it != vin.end()-1){
			ss << delimiter;
		}
	}
	return std::string(ss.str());
}
