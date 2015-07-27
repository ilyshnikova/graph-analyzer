#ifndef GRAPH
#define GRAPH

#include <string>
#include <unordered_map>
#include <ctime>
#include <vector>
#include <unordered_set>

#include "daemons.h"
#include "mysql.h"
#include "gan-exception.h"


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
	BlockBase* from;
	BlockBase* to;

public:
	Edge(const int id, const std::string& name, BlockBase* from, BlockBase* to);

	int GetEdgeId() const;

	BlockBase* To() const;

	BlockBase* From() const;

	std::string GetEdgeName() const;
};

/*      Block       */

class BlockBase {
private:
	int id;
	std::string block_name;
	std::unordered_set<std::string> incoming_edges_names;
	std::unordered_map<int, std::unordered_map<std::string, Point> > data;


//	virtual Point Do(const std::time_t& time) = 0;

//	virtual bool Check(const std::time_t& time) const;

public:

	std::unordered_map<std::string, Edge*> incoming_edges;
	std::unordered_map<std::string, Edge*> outgoing_edges;

	BlockBase(
		const int id,
		const std::string& block_name,
		const std::unordered_set<std::string>& incoming_edges_names
	);

	int GetBlockId() const;

	std::string GetBlockName() const;

	void AddIncomingEdge(Edge* edge);

	void DeleteIncomingEdge(const std::string& edge_name);

	void AddOutgoingEdge(Edge* edge, Table* blocks_and_outgoing_edges_table);

	void DeleteOutgoingEdge(const std::string& edge_name, Table* blocks_and_outgoing_edges_table);

	void Insert(const Point& point, const std::string& edge_name);

	bool Verification() const;

	void Save();

	Edge* GetOutgoingEdge(const std::string& edge_name);

	Edge* GetIncomingEdge(const std::string& edge_name);

	void DeleteEdge(
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	);

	void DeleteEdge(Edge* edge);

};

class TestBlock : public BlockBase {
private:

//	Point Do(const std::time_t& time);

//	bool Check(const std::time_t& time) const;
public:
	TestBlock(
		const int id,
		const std::string& block_name
	);

};

/*      Graph       */

class Graph {
private:
	int id;
	std::string graph_name;
	std::unordered_map<std::string, BlockBase*> blocks;
	std::vector<Edge*> edges;
	Table* graphs_and_blocks_table;
	Table* blocks_table;
	Table* edges_table;
	Table* blocks_and_outgoing_edges_table;



	BlockBase* GetBlock(
		const std::string& block_type,
		const int block_id,
		const std::string& block_name
	) const;

public:
	Graph(
		const int id,
		const std::string& graph_name,
		Table* graphs_and_blocks_table,
		Table* blocks_table,
		Table* edges_table,
		Table* blocks_and_outgoing_edges_table
	);

	void Load();

	int GetGraphId() const;

	void CreateVertex(const std::string& block_type, const std::string& block_name);

	void Insert(const Point& point, const std::string& block_name);

	void Delete(const std::string& block_name);

	void Verification();

	std::string BFSFindCycle(
		std::unordered_map<std::string, bool>* used,
		const std::string& start_block
	);

	void CreateBlock(
		const std::string& block_type,
		const int block_id,
		const std::string& block_name
	);

	bool In(const std::string& block_name) const;

 	void DeleteBlock(const std::string& block_name);

	void DeleteGraph();

	void CreateEdge(
		const int edge_id,
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	);


	void DeleteEdge(
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	);

	void DeleteEdge(Edge* edge_name);

};


/*    WorkSpace    */

class WorkSpace : public DaemonBase {
private:
	std::unordered_map<std::string, Graph*> graphs;
	Table graphs_table;
	Table graphs_and_blocks_table;
	Table blocks_table;
	Table edges_table;
	Table blocks_and_outgoing_edges_table;



	std::string Respond(const std::string& query);
public:

	void Load();

	WorkSpace();

	void CreateGraph(const int graph_id, const std::string& graph_name);

	void DeleteGraph(const int graph_id, const std::string& graph_name);

	void ChangeGraphsValid(const std::string& graph_name, const int valid);

	void Verification(const std::string& graph_name);

	~WorkSpace();

};
#endif
