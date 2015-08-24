#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <functional>
#include <algorithm>

#include "execute.h"

ExecuteHandler::ExecuteHandler(const std::string& cmd)
: result()
, index(0)
{
	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		std::cerr << "Error while opening pipe\n";
		return;
	}
	char buffer[128];
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL) {
			result.push_back(static_cast<std::string>(buffer));
		}
	}
	pclose(pipe);
}

bool ExecuteHandler::operator >> (std::string& string) {
	if (index == result.size()) {
		return 0;
	} else {
		string = result[index++];
		string.erase(
			string.begin(),
			std::find_if(
				string.begin(),
				string.end(),
				std::not1(std::ptr_fun<int, int>(std::isspace))
			)
		);
		string.erase(
			std::find_if(
				string.rbegin(),
				string.rend(),
				std::not1(std::ptr_fun<int, int>(std::isspace))
			).base(),
			string.end()
		);
		return 1;
	}
}

