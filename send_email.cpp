#include <ostream>
#include <boost/regex.hpp>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <fstream>
#include <json/json.h>
#include <fstream>
#include <cstdio>
#include <cmath>



int main  (int argc, char* argv[]) {
//	std::string s = "echo \"Point(series_name: Threshold(Sum3(Scale(Division(b,a);0.080000),Scale(b;0.090000),a);63.012551) value: 1.000000 time : Sunday Sun Jan 31 03:33:52 2016) \" | mail -s \" GAN massage \" \"ilyshnikova@yandex.ru\" &";
//	std::cout <<  s << std::endl;
//	system(s.c_str());
	std::cout << std::string(argv[1]) << std::endl;
	std::string message = std::string("echo \"")
		+ std::string(argv[1])
		+ "\" | mail -s \" GAN massage \" \"" + std::string(argv[2])  +  "\" &";
	std::cout  << "send email : " + message << std::endl;
	system(message.c_str());

	return 0;
}
