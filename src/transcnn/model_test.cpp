
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<ctime>
#include<numeric>
#include<map>

#include "model.hpp"

using namespace std;


vector<pricebase_t> split_string(std::string s,std::string delimiter);

int main()
{
    string data_dir("/home/yzhengbv/00-data/git/hft-system/data/modelio/");
	string input_path(data_dir+"input.csv");
	string output_path(data_dir+"output.csv"); 
	string result_path("result.csv");
	string answer_path("answer.csv");

	ifstream inputs, outputs;
	ofstream result, answer;

	clock_t start, end;
	double elapsed_ms;

	// result.open(result_path, ios::out | ios::trunc);
	// answer.open(answer_path, ios::out | ios::trunc);
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

	while (!inputs.eof()){
		inputs >> iline;
        outputs >> oline;
		line_splito = split_string(oline, string(","));
		line_split = split_string(iline, string(","));
        vector1d2array2d<pricebase_t, INPUT_LENGTH, INPUT_SIZE>(line_split, pricein);

        model(pricein, predout);
        if (!approx_equal<pricebase_t, OUTPUT_LENGTH>(line_splito, predout)) 
            break;

    }

	inputs.close();
	outputs.close();

	// Compare the results file with the golden results
	
	int retval=0;
	retval = system((string("diff --brief -w ")+result_path+" "+answer_path).data());
	if (retval != 0) {
		printf("Test failed  !!!\n"); 
		retval=1;
	} else {
		printf("Test passed !\n");
	}

	// Return 0 if the test passed
	return retval;
}


template <typename T, int D0, int D1>
void vector1d2array2d(
    vector<T>   vin, 
    T arr[D0][D1]
){
    for (int y = 0; y < D0; y++){
        for (int x = 0; x < D1; x++){
            arr[y][x] = vin[y*D0 + x];
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
        if (vin[i] - arr[i] > eps){
            cout << "Comparison wrong: " << endl;
            cout << "Answer" << '\t' << "Outputs" << endl;
            for (int ii = 0; ii < D0; ii++)
                cout << vin[ii] << '\t' << arr[ii] << endl;
            return false;
        }
    }
    return true;
}

vector<pricebase_t> split_string(
	std::string s,
	std::string delimiter
	){

	size_t pos = 0;
	std::string token;
	vector<string> res;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		res.push_back(stof(token));
		s.erase(0, pos + delimiter.length());
	}
	res.push_back(s);
//	std::cout << "Splitted line length: " << res.size() << std::endl;
	return res;
}