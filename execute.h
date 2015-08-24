#ifndef EXECUTE
#define EXECUTE

#include <vector>
#include <string>

class ExecuteHandler {
private:
	std::vector<std::string> result;
	size_t index;
public:
	ExecuteHandler(const std::string& cmd);


	bool operator >> (std::string& string);
};

#endif
