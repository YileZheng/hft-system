#include<iostream>
#include<fstream>

using namespace std;

int main()
{
	char ch;
	//creating the object of the data type 'istream'
	ifstream new_file;
	//opening the above file
	new_file.open("data/BNBBTC.txt",ios::in);
	//checking whether the file is available or not
	if(!new_file)
	{
		cout<<"Sorry the file you are looking for is not available"<<endl;
		return -1;
	}
	// reading the whole file till the end
	// while (!new_file.eof())
	// {
		new_file>>noskipws>>ch;
		// printing the content on the console
		cout<<ch;
	// }
	//closing the file after reading
	new_file.close();
	return 0;
}