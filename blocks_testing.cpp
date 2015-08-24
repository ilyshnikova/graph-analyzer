#include "testing.h"
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <fstream>
#include <string>
#include "execute.h"
#include <vector>

void Testing(const std::string& dir) {
	ExecuteHandler eh((std::string("ls ") + dir).c_str());
	std::string file;
	while (eh >> file) {
		std::string block_type;
		for (size_t i = 0; i < file.size() - 5; ++i) {
			block_type += file[i];
		}
		BlockTesting bt(block_type);
		std::ifstream fin(dir + "/" + file);
		YAML::Parser parser(fin);
		YAML::Node doc;
		parser.GetNextDocument(doc);
		doc >> bt;
		if (!bt.Testing()) {
			return;
		}
	}
}


int main() {
	Testing("./test_configs");
	return 0;
}
