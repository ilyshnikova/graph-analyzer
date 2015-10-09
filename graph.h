#ifndef GRAPH
#define GRAPH

#include <string>
#include <unordered_map>
#include <ctime>
#include <vector>
#include <unordered_set>
#include <json/json.h>
#include "yaml-cpp/yaml.h"
#include "daemons.h"
#include "mysql.h"
#include "gan-exception.h"


/*	Json	*/

Json::Value CreateJson(const std::string& value);


template
<typename K,typename V>
Json::Value CreateJson(const std::map<K,V>& value) {
	Json::Value jvalue;
	for (auto it = value.cbegin(); it != value.cend(); ++it) {
		jvalue[it->first] = CreateJson(it->second);
	}
	return jvalue;
}


template
<typename T>
Json::Value CreateJson(const std::vector<T>& value) {
	Json::Value jvalue;
	for (size_t i = 0; i < value.size(); ++i) {
		jvalue.append(CreateJson(value[i]));
	}
	return jvalue;
}


/*	AnswerTable	*/

struct AnswerTable {
	std::string status;
	std::vector<std::string> head;
	std::vector<std::vector<std::string> > rows;

	std::string ToString() const;
};

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

	bool operator==(const Point& other) const;
	bool operator!=(const Point& other) const;

	static Point Empty();

	operator std::string() const;

	friend std::ostream& operator<< (std::ostream& out, const Point& point);

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

	friend YAML::Emitter& operator<< (YAML::Emitter& out, const Edge& edge);

};

/*      BlockBase       */
class BlockBase {
private:

public:

	std::string block_type;
	std::string block_name_for_definition;

	std::unordered_set<std::string> incoming_edges_names;

	std::unordered_set<std::string> params_names;
	std::unordered_map<std::string, StringType> param_values;

	BlockBase(
		const std::unordered_set<std::string>& incoming_edges_names,
		const std::string& block_type,
		const std::unordered_set<std::string>& params_names,
		const std::unordered_map<std::string, StringType>& param_values,
		const std::string& block_name_for_definition=std::string("")

	);


	virtual BlockBase* GetBlock(const std::string& type) const = 0;

	std::string GetBlockType() const;

	virtual Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	) = 0;


	virtual std::string ToString() const;

	virtual void FromString(const std::string& elements_string);

	static std::string Join(const std::vector<std::string>& strings, const std::string separator);

	std::string GetResultSeriesName(
		const std::unordered_map<std::string, Point>& values
	) const;


	virtual std::string Description() const = 0;

	virtual ~BlockBase();

};

/*	Reducer 	*/

class Reducer : public BlockBase {
private:
	std::string base_block_type;

	std::unordered_set<std::string> CreateIncomingEdges(const size_t edges_count, const std::string& edges_name_type) const;

public:

	virtual double BaseFunction(const double first, const double second) const = 0;

	virtual double StartValue() const = 0;

	Reducer(
		const std::string& block_type,
		const std::string& edges_name_type,
		const size_t edges_count
	);

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	virtual BlockBase* GetBlock(const std::string& type) const;

	virtual std::string Description() const;

};



/*	Sum	*/

class Sum : public Reducer {
public:
	Sum(const size_t edges_count);

	double BaseFunction(const double first, const double second) const;

	double StartValue() const;

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;


};

/*	Multiplication	*/

class Multiplication : public Reducer {
public:
	Multiplication(const size_t edges_count);

	double BaseFunction(const double first, const double second) const;

	double StartValue() const;

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};


/*	And	*/

class And : public Reducer {
public:
	And(const size_t edges_count);

	double BaseFunction(const double first, const double second) const;

	double StartValue() const;

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};


/*	Or	*/

class Or : public Reducer {
public:
	Or(const size_t edges_count);

	double BaseFunction(const double first, const double second) const;

	double StartValue() const;

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};

/*	Min	*/

class Min : public Reducer {
public:
	Min(const size_t edges_count);

	double BaseFunction(const double first, const double second) const;

	double StartValue() const;

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};

/*	Max	*/

class Max : public Reducer {
public:
	Max(const size_t edges_count);

	double BaseFunction(const double first, const double second) const;

	double StartValue() const;

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};


/*	EmptyBlock	*/

class EmptyBlock : public BlockBase {
private:

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

public:
	EmptyBlock();

	BlockBase* GetBlock(const std::string& block_type) const;

	std::string Description() const;
};


/*	Difference	*/

class Difference : public BlockBase {
public:
	Difference();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};

/*	Division	*/

class Division : public BlockBase {
public:
	Division();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};


/*	Threshold	*/

class Threshold : public BlockBase {
public:
	Threshold();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};


/*	Scale	*/

class Scale : public BlockBase {
public:
	Scale();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& type) const;

	std::string Description() const;
};



/*	PrintToLogs	*/

class PrintToLogs : public BlockBase {
private:

public:
	PrintToLogs();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& block_type) const;

	std::string Description() const;
};


/*	TimeShift	*/


class TimeShift : public BlockBase {
private:
public:
	TimeShift();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& block_type) const;

	std::string Description() const;

};


/*	SendEmail	*/


class SendEmail : public BlockBase {
private:
public:
	SendEmail();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	BlockBase* GetBlock(const std::string& block_type) const;

	std::string Description() const;

};



/*	TimePeriodAggregator	*/

class TimePeriodAggregator : public BlockBase {
private:
	std::unordered_map<std::time_t, int> points_count;
	std::unordered_map<std::time_t, double>  sums;

public:

	TimePeriodAggregator();

	Point Do(
		const std::unordered_map<std::string, Point>& values,
		const std::time_t& time
	);

	void CleanHistory(const int min_bucket_points);

	std::string ToString() const;

	void FromString(const std::string& elements_string);

	BlockBase* GetBlock(const std::string& block_type) const;

	std::string Description() const;
};

/*	BlockCacheUpdaterBuffer	*/

class BlockCacheUpdaterBuffer {
private:
	std::unordered_map<int, Block*> blocks;
	Table* blocks_table;
	std::time_t last_update_time;
	std::time_t timeout;
	size_t max_blocks_count;

public:
	BlockCacheUpdaterBuffer();

	BlockCacheUpdaterBuffer(Table* blocks_table);

	BlockCacheUpdaterBuffer&  SetTable(Table* table);

	void PushUpdate(const int block_id, Block* block);

	void Update();
};

/*	Block	*/

class Block {
private:
	BlockBase* block;
	int id;
	std::string block_name;
	std::unordered_map<std::time_t, std::unordered_map<std::string, Point> > data;
	std::vector<BlockBase*> blocks;
	BlockCacheUpdaterBuffer* block_buffer;

public:


	std::unordered_map<std::string, Edge*> incoming_edges;
	std::unordered_map<std::string, Edge*> outgoing_edges;

	BlockBase* GetBlock(const std::string& block_type) const;

	BlockBase* GetBlock() const;

	std::string GetAllBlocksDescriptions() const;

	Json::Value GetTableOfBlocksDescriptions() const;

	Block(
		const int id,
		const std::string& block_name,
		const std::string& block_type,
		BlockCacheUpdaterBuffer* block_buffer

	);

	void Load(const std::string& cache);

	std::string GetBlockType() const;

	int GetBlockId() const;

	std::string GetBlockName() const;

	void AddIncomingEdge(Edge* edge);

	void DeleteIncomingEdge(const std::string& edge_name);

	void AddOutgoingEdgeToTable(Edge* edge, Table* blocks_and_outgoing_edges_table);

	void AddOutgoingEdge(Edge* edge);

	void DeleteOutgoingEdge(const std::string& edge_name, Table* blocks_and_outgoing_edges_table);

	void Verification() const;

	bool DoesEdgeExist(const std::string& incoming_edge_name);

	Edge* GetOutgoingEdge(const std::string& edge_name);

	Edge* GetIncomingEdge(const std::string& edge_name);

	void DeleteEdge(
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	);


	void Insert(const Point& point, const std::string& edge_name);

	void SendByAllEdges(const Point& point) const;

	bool Check(const std::time_t& time) const;

	void DeleteEdge(Edge* edge);

	void AddParam(const std::string& param_name, const StringType& param_value);

	std::string ToString();

	std::vector<std::vector<std::string> > GetParams() const;

	std::vector<std::vector<std::string> > GetPossibleEdges() const;

	friend YAML::Emitter& operator<< (YAML::Emitter& out, const Block& block);

	~Block();

};


/*      Graph       */

class Graph {
private:
	int id;
	std::string graph_name;
	std::unordered_map<std::string, Block*> blocks;
	std::unordered_set<Edge*> edges;
	Table* graphs_and_blocks_table;
	Table* blocks_table;
	Table* edges_table;
	Table* blocks_and_outgoing_edges_table;
	Table* blocks_params_table;
	bool valid;
	BlockCacheUpdaterBuffer* block_buffer;



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
		const bool valid,
		BlockCacheUpdaterBuffer* block_buffer
	);

	Graph(
		const int id,
		const std::string& graph_name,
		Table* graphs_and_blocks_table,
		Table* blocks_table,
		Table* edges_table,
		Table* blocks_and_outgoing_edges_table,
		Table* blocks_params_table,
		const bool valid,
		BlockCacheUpdaterBuffer* block_buffer,
		const std::string& file_name
	);


	void Load();

	int GetGraphId() const;

	std::string GetGraphName() const;

	bool GetGraphValid() const;

	void CreateVertex(const std::string& block_type, const std::string& block_name);

	void Insert(const Point& point, const std::string& block_name, BlockCacheUpdaterBuffer* block_buffer);

	void Delete(const std::string& block_name);

	void Verification();

	std::string DFSFindCycle(
		std::unordered_map<std::string, int>* colors,
		std::string start_block
	) const;

	std::string RecDFSFindCycle(
		std::unordered_map<std::string, int>* colors,
		std::unordered_map<std::string, std::string>*  way,
		std::string& block_name
	) const;


	void AddBlockToTables(Block* block);

	Block* CreateBlock(
		const std::string& block_type,
		const int block_id,
		const std::string& block_name
	);

	bool In(const std::string& block_name) const;

 	void DeleteBlock(const std::string& block_name);

	void DeleteGraph();

	bool DoesEdgeExist(const std::string& block_name, const std::string& incoming_edge_name);

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

	std::vector<std::vector<std::string> > GetBlocksNames() const;

	std::vector<std::vector<std::string> > GetBlocksParams(const std::string& block_name) const;

	void AddParam(const std::string& param_name, const StringType& param_value, const std::string& block_name);

	std::vector<std::vector<std::string> > GetEdges() const;

	std::vector<std::vector<std::string> > GetPossibleEdges(const std::string& block_name) const;

	std::string GetBlockType(const std::string& block_name) const;

	void SaveGraphToFile(const std::string& file_name ) const;

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
	BlockCacheUpdaterBuffer block_buffer;


	class QueryActionBase {
	protected:
		const Json::Value* json_params;
		WorkSpace* work_space;
		bool ignore;
		Json::Value* answer;
		GANException exception;

		virtual void Action(const int object_id) = 0;

		void CheckIgnore();

		virtual bool Check() const = 0;

		virtual int GetId() const = 0;

	public:

		QueryActionBase(
		       	const Json::Value* json_params,
			WorkSpace* work_space,
			Json::Value* answer,
			const GANException& exception
		);

		void Execute();

	};



	class CreateGraphQuery : public QueryActionBase {
	private:
		void Action(const int graph_id);

		void CheckIgnore();

		bool Check() const;

		int GetId() const;

	public:
		CreateGraphQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};



	class CreateBlockQuery : public QueryActionBase {
	private:
		Graph* graph;

		void Action(const int block_id);

		void CheckIgnore();

		bool Check() const;

		int GetId() const;

	public:
		CreateBlockQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};


	class CreateEdgeQuery : public QueryActionBase {
	private:
		Graph* graph;

		void Action(const int edge_id);

		void CheckIgnore();

		bool Check() const;

		int GetId() const;

	public:
		CreateEdgeQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};



	class DeleteGraphQuery : public QueryActionBase {
	private:
		Graph* graph;

		void Action(const int graph_id);

		void CheckIgnore();

		bool Check() const;

		int GetId() const;

	public:
		DeleteGraphQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};



	class DeleteBlockQuery : public QueryActionBase {
	private:
		Graph* graph;

		void Action(const int block_id);

		void CheckIgnore();

		bool Check() const;

		int GetId() const;

	public:
		DeleteBlockQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};


	class DeleteEdgeQuery : public QueryActionBase {
	private:
		Graph* graph;

		void Action(const int edge_id);

		void CheckIgnore();

		bool Check() const;

		int GetId() const;

	public:
		DeleteEdgeQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};






	class  QueryAction {
	public:
		QueryAction(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer);

	};



	struct IgnoreChecker {
		Json::Value* answer;
		bool ignore;

		IgnoreChecker(Json::Value* answer, const bool ignore);
	};

	void CheckIgnore(const IgnoreChecker& checker, const GANException& exception) const;

	std::string Respond(const std::string& query);

	Json::Value JsonRespond(const Json::Value& query);
public:

	void Load();

	WorkSpace();

	void AddGaphToTables(Graph* grpah);

	bool IsGraphExist(const std::string& graph_name) const;

	Graph* GetGraph(const std::string& graph_name) const;

	Graph* GetGraph(const int graph_id, const std::string& graph_name, const bool valid) const;

	Graph* CreateGraph(const int graph_id, const std::string& graph_name, const bool valid);

	void DeleteGraph(const std::string& graph_name);

	void ChangeGraphsValid(const std::string& graph_name, const int valid);

	void Verification(const std::string& graph_name);

	std::vector<std::vector<std::string> >  ConvertConfigToQueries(
		const std::string& file_name,
		const std::string& graph_name="<graph_name>",
		const std::vector<std::string>& first_queries={}
	) const;

	Json::Value LoadGraphFromFile(
		const std::string& file_name,
		const std::string& graph_name
	);

	std::vector<Json::Value> ConvertConfigToJsonQueries(
		const std::string& file_name,
		const std::string& graph_name
	) const;

	Table* GetTable(const std::string& object_type);

	~WorkSpace();

};
#endif
