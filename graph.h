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

class Block;


class Edge {
private:
	int id;
	std::string edge_name;
	Block* from;
	Block* to;

public:
	Edge(const int id, const std::string& name, Block* from, Block* to);

	int GetEdgeId() const;

	Block* To() const;

	Block* From() const;

	std::string GetEdgeName() const;

};

/*      BlockBase       */
class BlockBase {
private:
//	virtual Point Do(const std::time_t& time);

//	virtual bool Check(const std::time_t& time) const;

public:
	std::unordered_set<std::string> incoming_edges_names;

	BlockBase(const std::unordered_set<std::string>& incoming_edges_names);
};

class TestBlock : public BlockBase {
private:

//	Point Do(const std::time_t& time);

//	bool Check(const std::time_t& time) const;
public:
	TestBlock();

};

class EmptyTestBlock : public BlockBase {
private:

//	Point Do(const std::time_t& time);

//	bool Check(const std::time_t& time) const;
public:
	EmptyTestBlock();

};


/*	Block	*/

class Block {
private:
	BlockBase* block;
	int id;
	std::string block_name;
	std::unordered_map<int, std::unordered_map<std::string, Point> > data;
	std::string block_type;

public:

	std::unordered_map<std::string, Edge*> incoming_edges;
	std::unordered_map<std::string, Edge*> outgoing_edges;

	Block(
		const int id,
		const std::string& block_name,
		const std::string& block_type
	);

	std::string GetBlockType() const;

	int GetBlockId() const;

	std::string GetBlockName() const;

	void AddIncomingEdge(Edge* edge);

	void DeleteIncomingEdge(const std::string& edge_name);

	void AddOutgoingEdgeToTable(Edge* edge, Table* blocks_and_outgoing_edges_table);

	void AddOutgoingEdge(Edge* edge, Table* blocks_and_outgoing_edges_table);

	void DeleteOutgoingEdge(const std::string& edge_name, Table* blocks_and_outgoing_edges_table);

	void Insert(const Point& point, const std::string& edge_name);

	bool Verification() const;

	void Save();

	bool DoesEdgeExist(std::string& incoming_edge_name);

	bool CanEdgeExist(std::string& incoming_edge_name);

	Edge* GetOutgoingEdge(const std::string& edge_name);

	Edge* GetIncomingEdge(const std::string& edge_name);

	void DeleteEdge(
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	);

	void DeleteEdge(Edge* edge);

	~Block();

};


/*      Graph       */

class Graph {
private:
	int id;
	std::string graph_name;
	std::unordered_map<std::string, Block*> blocks;
	std::vector<Edge*> edges;
	Table* graphs_and_blocks_table;
	Table* blocks_table;
	Table* edges_table;
	Table* blocks_and_outgoing_edges_table;



	Block* GetBlock(
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

	std::string GetGraphName() const;

	void CreateVertex(const std::string& block_type, const std::string& block_name);

	void Insert(const Point& point, const std::string& block_name);

	void Delete(const std::string& block_name);

	void Verification();

	std::string BFSFindCycle(
		std::unordered_map<std::string, bool>* used,
		const std::string& start_block
	);

	void AddBlockToTables(Block* block);

	Block* CreateBlock(
		const std::string& block_type,
		const int block_id,
		const std::string& block_name
	);

	bool In(const std::string& block_name) const;

 	void DeleteBlock(const std::string& block_name);

	void DeleteGraph();

	bool DoesEdgeExist(const std::string& block_name, std::string& incoming_edge_name);

	bool CanEdgeExist(const std::string& block_name, std::string& incoming_edge_name);

	void AddEdgeToTables(Edge* edge);

	Edge* CreateEdge(
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

	~Graph();

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

	void AddGaphToTables(Graph* grpah);

	Graph* CreateGraph(const int graph_id, const std::string& graph_name);

	void DeleteGraph(const int graph_id, const std::string& graph_name);

	void ChangeGraphsValid(const std::string& graph_name, const int valid);

	void Verification(const std::string& graph_name);

	~WorkSpace();

};
#endif
