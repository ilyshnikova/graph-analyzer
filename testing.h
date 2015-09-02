#ifndef TESTING
#define TESTING

#include <map>
#include "graph.h"
#include <vector>
#include "yaml-cpp/yaml.h"
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "execute.h"

struct TestPoint {
	Point point;

	TestPoint();

	TestPoint(const Point& point);

	friend void operator >> (const YAML::Node& node, TestPoint& point);

};


struct IncomingPoint {
	std::string edge_name;
	TestPoint point;

	friend void operator >> (const YAML::Node& node, IncomingPoint& point);


};

struct Test {
	std::unordered_map<std::string, StringType> params;
	std::vector<std::unordered_map<std::string, Point> > incoming_points;
	std::vector<Point> outgoing_points;

	bool Testing(const std::string& block_type) const;

	friend void operator >> (const YAML::Node& node, Test& test);
};


struct BlockTesting {
	std::string block_type;
	std::vector<Test> tests;


	BlockTesting(const std::string& block_type);

	bool Testing();

	friend void operator >> (const YAML::Node& node, BlockTesting& bt);

};


#endif

