#ifndef GRAPH
#define GRAPH

#include <string>
#include <unordered_map>
#include <ctime>
#include <vector>
#include "daemons.h"
#include "mysql.h"


/*      Point       */

class Point {
private:
	int id;
	std::string series_name;
	double value;
	std::time_t time;

public:

	Point(
		const int id,
		const std::string& series_name,
		const double value,
		const std::time_t& time
	);

	std::string GetSeriesNme() const;

	int GetTime() const;

	double GetValue() const;
};

/*    Edge     */

class BlockBase;


class Edge {
private:
	int id;
	std::string edge_name;
	BlockBase* to;

public:
	Edge(const int id, const std::string& name, BlockBase* to);


	BlockBase* To() const;
	std::string GetEdgeName() const;
};

/*      Block       */

class BlockBase {
private:
	int id;
	std::vector<std::string> incoming_edges_names;
	std::unordered_map<int, std::unordered_map<std::string, Point> > data;
	std::unordered_map<std::string, Edge*> incoming_edges;
	std::unordered_map<std::string, Edge*> outgoing_edges;

	bool Check(const std::time_t& time) const;

public:

	virtual Point Do(const std::time_t& time);

	void Insert(const Point& point, const std::string& edge_name);

	bool Verification() const;

	void Save();

	BlockBase(const int id, const std::vector<std::string>& incoming_edges_names);
};

/*      Graph       */

class Graph {
private:
	int id;
	std::string graph_name;
	std::unordered_map<std::string, BlockBase*> graph;
	std::vector<Edge*> edges;
//	Table blocks_table;
//	Table edges_table;
//	Table outgoing_edges_and_blocks_table;

public:
	Graph(const int id, const std::string& graph_name);

	int GetId() const;

	void CreateVertex(const std::string& block_type, const std::string& block_name);

	void Insert(const Point& point, const std::string& block_name);

	void Delete(const std::string& block_name);

	bool Verification() const;

};


/*    WorkSpace    */

class WorkSpace : public DaemonBase {
private:
	std::unordered_map<std::string, Graph*> graphs;
	Table graphs_table;
	Table graphs_and_blocks_table;

	std::string Respond(const std::string& query);
public:

	void Recover();

	WorkSpace();

	void CreateGraph(const int graph_id, const std::string& graph_name);

	void DeleteGraph(const int graph_id, const std::string& graph_name);

	class WorkSpaceExceptions : public std::exception {
	private:
		int id;
		std::string reason;

	public:
		WorkSpaceExceptions(const std::string& reason);

		const char * what() const throw();

		~WorkSpaceExceptions() throw();
	};

	~WorkSpace();

};
#endif
