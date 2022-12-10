
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<ctime>
#include<numeric>
#include<cmath>

#include "model.hpp"

using namespace std;


vector<pricebase_t> split_string(std::string s,std::string delimiter);
vector<pricebase_t> slicing(vector<pricebase_t>& arr, int X, int Y);
string concat_string(vector<pricebase_t> vin, std::string delimiter);


template <typename T, int D0, int D1>
void vector1d2array2d(
    vector<T>   vin,
    T arr[D0][D1]
){
    for (int y = 0; y < D0; y++){
        for (int x = 0; x < D1; x++){
            arr[y][x] = vin[y*D1 + x];
        }
    }
}

template <typename T, int D0>
bool approx_equal(
    vector<T>   vin,
    T arr[D0]
){
    float eps = 5e-4;
    assert(vin.size() == D0);
    for (int i = 0; i < D0; i++){
        if (abs(vin[i] - arr[i]) > eps){
            cout << "Comparison wrong: " << endl;
            cout << "Answer" << '\t' << "Outputs" << endl;
            for (int ii = 0; ii < D0; ii++)
                cout << vin[ii] << '\t' << arr[ii] << endl;
            return false;
        }
    }
    return true;
}


int main()
{
    string data_dir("/home/yzhengbv/00-data/git/hft-system/data/modelio/");
	string input_path(data_dir+"input_s.csv");
	string output_path(data_dir+"output_pmodel_s.csv");
	string result_path("result.csv");
	string answer_path("answer.csv");

	ifstream inputs, outputs;
	ofstream result, answer;

	clock_t start, end;
	double elapsed_ms;
	double elapsed_sum = 0;

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
    pricebase_t pricein[INPUT_LENGTH][INPUT_SIZE];
    pricebase_t predout[OUTPUT_LENGTH];

	int retval=0;
    int id = 0;
	char bufi[2096] = {0};
	char bufo[1024] = {0};
	// while (!inputs.eof()){
	while (inputs.getline(bufi, sizeof(bufi), '\n')){
		// inputs.getline(bufi, sizeof(bufi), '\n');
		iline = string(bufi);
        outputs.getline(bufo, sizeof(bufo), '\n');
		oline = string(bufo);

		line_splito = split_string(oline, string(","));
		line_split = split_string(iline, string(","));
        vector1d2array2d<pricebase_t, INPUT_LENGTH, INPUT_SIZE>(line_split, pricein);

        start = clock();
        model(pricein, predout);
		end = clock();
		elapsed_ms = (double)(end-start)/CLOCKS_PER_SEC * 1000000;

		answer << concat_string(line_splito, string(",")) << std::endl;
		result << concat_string(vector<pricebase_t>(predout, predout+ sizeof(predout)/sizeof(pricebase_t)), string(",")) << std::endl;
		std::cout << "Line " << id; 

        if (!approx_equal<pricebase_t, OUTPUT_LENGTH>(slicing(line_splito,  line_splito.size()-OUTPUT_LENGTH, line_splito.size()-1), predout)){
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

	// Compare the results file with the golden results
	
//	retval = system((string("diff --brief -w ")+result_path+" "+answer_path).data());
//	if (retval != 0) {
//		printf("Test failed  !!!\n");
//		retval=1;
//	} else {
//		printf("Test passed !\n");
//	}

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
