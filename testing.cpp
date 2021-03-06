#include "graph.h"
#include <vector>
#include <unordered_map>
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "testing.h"
#include "execute.h"


TestPoint::TestPoint(const Point& point)
: point(point)
{}

TestPoint::TestPoint()
: point()
{}

void operator >> (const YAML::Node& node, TestPoint& point) {
	std::string series_name;
	double value;
	std::time_t time;

	node["series_name"] >> series_name;
	node["value"] >> value;
	node["time"] >> time;
	point.point = Point(series_name, value, time);
}

void operator >> (const YAML::Node& node, IncomingPoint& point) {
	node["edge_name"] >> point.edge_name;
	std::string series_name;
	double value;
	std::time_t time;

	node["series_name"] >> series_name;
	node["value"] >> value;
	node["time"] >> time;
	point.point = TestPoint(Point(series_name, value, time));
}


bool Test::Testing(const std::string& block_type) const {
 	BlockBase* block = Block(1, "", block_type, NULL, NULL).GetBlock();
	block->param_values = params;
	for (size_t i = 0; i < incoming_points.size(); ++i) {
		std::unordered_map<std::string, Point> values = incoming_points[i];
		std::time_t time = values[*(block->incoming_edges_names.begin())].GetTime();
		Point result = block->Do(values, time);
		if (outgoing_points[i] != result) {
				std::cout << "\033[1;31m not ok \033[0m\n";
				std::cout << "In class "
				        << block_type
				        <<" in test number "
				       	<< std::to_string(i)
				        << " with incoming points: \n";

				for (
					auto it = values.begin();
					it != values.end();
					++it
				) {
					Point point = it->second;
					std::cout << "edge_name : "
						<<  it->first
						<< ", "
						<< point
						<< "\n";
				}

				std::cout <<
					"should have result:"
					<< outgoing_points[i]
					<< "\nhave result:"
					<< result
					<< "\n";

				return false;
		} else {
			std::cout << "\033[1;32m ok \033[0m in test " << std::to_string(i) << "\n";
		}

	}
	return true;
}

void operator >> (const YAML::Node& node, Test& test) {
	std::map<std::string, std::string> params;
	node["params"] >> params;

	for (auto it = params.begin(); it != params.end(); ++it) {
		test.params[it->first] = StringType(it->second);
	}

	const YAML::Node& values = node["incoming_points"];
	test.incoming_points = std::vector<std::unordered_map<std::string, Point> >(values.size());

	for (size_t i = 0; i < values.size(); ++i) {
		const YAML::Node& points = values[i];

		for (size_t j = 0; j < points.size(); ++j) {
			IncomingPoint point;
			points[j] >> point;
			test.incoming_points[i][point.edge_name] = point.point.point;
		}
	}

	const YAML::Node& results = node["outgoing_points"];
	test.outgoing_points = std::vector<Point>(results.size());
	for (size_t i = 0; i < results.size(); ++i) {
		TestPoint tpoint;
		results[i] >> tpoint;
		test.outgoing_points[i] = tpoint.point;
	}

}



BlockTesting::BlockTesting(const std::string& block_type)
: block_type(block_type)
, tests()
{}

void operator >> (const YAML::Node& node, BlockTesting& bt) {
	bt.tests = std::vector<Test>(node.size());
	for (size_t i = 0; i < node.size(); ++i) {
		 node[i] >> bt.tests[i];
	}

}


bool  BlockTesting::Testing() {
	for (size_t i = 0; i < tests.size(); ++i) {
		std::cout << "\ntest series number: " << std::to_string(i) << "\n";
		if (!tests[i].Testing(block_type)) {
			return false;
		}
	}
	return true;

}


void Testing(const std::string& dir) {
	ExecuteHandler eh((std::string("ls ") + dir).c_str());
	std::string file;
	while (eh >> file) {
		std::string block_type;
		std::cout << "\n" << file << ":\n";
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
