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
	std::string series_name;
	double value;
	std::time_t time;
	bool is_empty;

public:
	Point();

	Point(
		const std::string& series_name,
		const double value,
		const std::time_t& time
	);

	std::string GetSeriesName() const;

	std::time_t GetTime() const;

	double GetValue() const;

	bool IsEmpty() const;

	static Point Empty();
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

public:
	std::unordered_set<std::string> incoming_edges_names;

	BlockBase(const std::unordered_set<std::string>& incoming_edges_names);

	virtual Point Do(
		std::unordered_map<std::string, Point>& values,
		const std::time_t& time,
		const std::unordered_map<std::string, StringType>& param_values
	) = 0;

};


class EmptyBlock : public BlockBase {
private:

	Point Do(
		std::unordered_map<std::string, Point>& values,
		const std::time_t& time,
		const std::unordered_map<std::string, StringType>& param_values
	);

public:
	EmptyBlock();

};


class Sum : public BlockBase {
private:

	std::unordered_set<std::string> CreateIncomingEdges(const int edges_name) const;

public:
	Sum(const int edges_cout);

	Point Do(
		std::unordered_map<std::string, Point>& values,
		const std::time_t& time,
		const std::unordered_map<std::string, StringType>& param_values
	);

};


class PrintToLogs : public BlockBase {
private:

public:
	PrintToLogs();

	Point Do(
		std::unordered_map<std::string, Point>& values,
		const std::time_t& time,
		const std::unordered_map<std::string, StringType>& param_values
	);

};

class TimeShift : public BlockBase {
private:
public:
	TimeShift();

	Point Do(
		std::unordered_map<std::string, Point>& values,
		const std::time_t& time,
		const std::unordered_map<std::string, StringType>& param_values
	);

};

/*	Block	*/

class Block {
private:
	BlockBase* block;
	int id;
	std::string block_name;
	std::unordered_map<std::time_t, std::unordered_map<std::string, Point> > data;
	std::string block_type;

	std::unordered_set<std::string> param_names;
	std::unordered_map<std::string, StringType> param_values;

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

	void Verification() const;

	bool DoesEdgeExist(std::string& incoming_edge_name);

	bool CanEdgeExist(std::string& incoming_edge_name);

	Edge* GetOutgoingEdge(const std::string& edge_name);

	Edge* GetIncomingEdge(const std::string& edge_name);

	void DeleteEdge(
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	);

//	void Save();

	void Insert(const Point& point, const std::string& edge_name);

	void SendByAllEdges(const Point& point) const;

	bool Check(const std::time_t& time) const;

	void DeleteEdge(Edge* edge);

	void AddParam(const std::string& param_name, const StringType& param_value);

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
	Table* blocks_params_table;
	bool valid;



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
		Table* blocks_and_outgoing_edges_table,
		Table* blocks_params_table,
		const bool valid
	);

	void Load();

	int GetGraphId() const;

	std::string GetGraphName() const;

	bool GetGraphValid() const;

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

	void InsertPoint(const Point& point, const std::string& block_name);

	size_t IncomingEdgesCount(const std::string& block_name) const;

	void InsertPointToAllPossibleBlocks(const Point& point);

	void ChangeGraphsValid(const bool new_valid);

	void AddParamToTable(
		const std::string& param_name,
		const StringType& param_value,
		const std::string& block_name
	);

	void AddParam(const std::string& param_name, const StringType& param_value, const std::string& block_name);

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
	Table blocks_params_table;



	std::string Respond(const std::string& query);
public:

	void Load();

	WorkSpace();

	void AddGaphToTables(Graph* grpah);

	Graph* CreateGraph(const int graph_id, const std::string& graph_name, const bool valid);

	void DeleteGraph(const int graph_id, const std::string& graph_name);

	void ChangeGraphsValid(const std::string& graph_name, const int valid);

	void Verification(const std::string& graph_name);



	~WorkSpace();

};
#endif
