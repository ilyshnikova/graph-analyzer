#include <boost/regex.hpp>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include "gan-exception.h"
#include "graph.h"
#include "logger.h"

Point::Point()
: series_name()
, value()
, time()
, is_empty(true)
{}


Point::Point(
	const std::string& series_name,
	const double value,
	const std::time_t& time
)
: series_name(series_name)
, value(value)
, time(time)
, is_empty(false)
{}

std::string Point::GetSeriesName() const {
	return series_name;
}

std::time_t Point::GetTime() const {
	return time;
}

double Point::GetValue() const {
	return value;
}

bool Point::IsEmpty()const  {
	return is_empty;
}

Point Point::Empty() {
	return Point();
}




/*  Edge   */
Edge::Edge(const int id, const std::string& name, Block* from, Block* to)
: id(id)
, edge_name(name)
, from(from)
, to(to)
{}

int Edge::GetEdgeId() const {
	return id;
}

Block* Edge::To() const {
	return to;
}

Block* Edge::From() const {
	return from;
}

std::string Edge::GetEdgeName() const {
	return edge_name;
}


/*   BlockBase    */
//
//BlockBase::BlockBase(
//	const std::unordered_set<std::string>& incoming_edges_names
//)
//: incoming_edges_names(incoming_edges_names)
//, block_type()
//{}

BlockBase::BlockBase(
	const std::unordered_set<std::string>& incoming_edges_names,
	const std::string& block_type,
	const std::unordered_set<std::string>& params_names,
	const std::unordered_map<std::string, StringType>& param_values
)
: incoming_edges_names(incoming_edges_names)
, block_type(block_type)
, params_names(params_names)
, param_values(param_values)
{}

std::string BlockBase::GetBlockType() const {
	return block_type;
}


std::string BlockBase::ToString() const {
	return std::string("");
}

void BlockBase::FromString(const std::string&) {}


std::string BlockBase::Join(const std::vector<std::string>& strings, const std::string separator) const {
	std::string result;
	for (size_t i = 0 ; i < strings.size(); ++i) {
		result += strings[i] + (i + 1 == strings.size() ? "" : separator);
	}

	return result;
}


std::string BlockBase::GetResultSeriesName(
	const std::unordered_map<std::string, Point>& values
) const {
	std::string series_name = block_type + std::string("(");

	std::vector<std::string> names;
	for (auto it = incoming_edges_names.begin(); it != incoming_edges_names.end(); ++it) {
		names.push_back(values.at(*it).GetSeriesName());
	}

	std::vector<std::string> params;

	for (auto it = param_values.cbegin(); it != param_values.cend(); ++it) {
		params.push_back(it->second);
	}

	return series_name + Join(names, ",") + (params.size() == 0 ? "" : ";" +  Join(params, ",")) + ")";
}

/*   EmptyTestBlock    */


EmptyBlock::EmptyBlock(const std::string& block_type)
: BlockBase({}, block_type, {}, {})
{}


Point EmptyBlock::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point::Empty();
}

/*	Sum	*/


std::unordered_set<std::string> Sum::CreateIncomingEdges(const int edges_count) const {
	std::unordered_set<std::string> edges;
	for (size_t i = 0; i < edges_count; ++i) {
		edges.insert("arg" + std::to_string(i + 1));
	}
	return edges;
}


Sum::Sum(const int edges_count, const std::string& block_type)
: BlockBase(CreateIncomingEdges(edges_count), block_type, {}, {})
{}


Point Sum::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	double res_value = double(0);
	for (auto it = values.cbegin(); it != values.cend(); ++it) {
		res_value += it->second.GetValue();
	}

	return Point(GetResultSeriesName(values), res_value, time);

}


/*	PrintToLogs	*/

PrintToLogs::PrintToLogs(const std::string& block_type)
: BlockBase({"to_print"}, block_type, {}, {})
{}


Point PrintToLogs::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	for (auto it = values.cbegin(); it != values.cend(); ++it) {
		logger <<
			"Point: series name: "
			+ it->second.GetSeriesName()
			+ " value: "
			+ std::to_string(it->second.GetValue())
			+ " time: "
			+ std::to_string(it->second.GetTime());
	}
	return Point::Empty();
}

/*	TimeShift	*/


TimeShift::TimeShift(const std::string& block_type)
:BlockBase({"to_shift"}, block_type, {"time_shift"}, {})
{}

Point TimeShift::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	Point point = values.cbegin()->second;
	return Point(
		GetResultSeriesName(values),
		point.GetValue(),
		point.GetTime() + int(param_values.at("time_shift"))
	);
}

/*	TimePeriodAggregator	*/

//* 1) Опечатка, надо писать Aggregator, тоже самое со словом aggregator
//* исправлено
//* 2) Имя блока надо пробросить из Block::Block(), а не тут
//* исправлено
TimePeriodAggregator::TimePeriodAggregator(const std::string& block_type)
: BlockBase(
	{"to_aggregate"},
	block_type,
	{"round_time", "keep_history_interval", "min_bucket_points"},
	{{"round_time", 3600}, {"keep_history_interval", 3600 * 24}}
)
{}


void TimePeriodAggregator::CleanHistory(const int keep_history_interval) {
	std::vector<std::time_t> times_to_delete;
	for (auto it = sums.begin(); it != sums.end(); ++it) {
		if (std::time(0) - it->first > keep_history_interval) {
			times_to_delete.push_back(it->first);
		}
	}

	for (size_t i = 0; i < times_to_delete.size(); ++i) {
		sums.erase(times_to_delete[i]);
		points_count.erase(times_to_delete[i]);
	}
}


Point TimePeriodAggregator::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	int round_time = param_values.at("round_time");
	int keep_history_interval = param_values.at("keep_history_interval");
	int min_bucket_points = param_values.at("min_bucket_points");

	for (auto it = values.cbegin(); it != values.cend(); ++it) {
		Point point = it->second;
		std::time_t rounded_time = point.GetTime() - point.GetTime() % round_time;

		sums[rounded_time] += point.GetValue();
		logger <<
			"Do in block TimePeriodAggregator: points count -- "
			+ std::to_string(points_count[rounded_time])
			+ "  with value -- "
			+ std::to_string(sums[rounded_time]);

		//* Тут надо >= вместо строгого неравенства
		//* исправлено
		if (++points_count[rounded_time] >= min_bucket_points) {
			Point point_to_return(
				GetResultSeriesName(values),
				sums[rounded_time],
				rounded_time
			);
			sums.erase(rounded_time);
			points_count.erase(rounded_time);
			CleanHistory(keep_history_interval);
			return point_to_return;
		}
	}
	return Point::Empty();
}

std::string TimePeriodAggregator::ToString() const {
	return StringType(points_count, sums);
}

void TimePeriodAggregator::FromString(const std::string& elements_string) {
	StringType(elements_string).FromString(&points_count, &sums);
}




/*	BlockCacheUpdaterBuffer	*/

BlockCacheUpdaterBuffer::BlockCacheUpdaterBuffer()
: blocks()
, blocks_table(NULL)
, last_update_time(time(0))
, timeout(60)
, max_blocks_count(5000)
{}


BlockCacheUpdaterBuffer&  BlockCacheUpdaterBuffer::SetTable(Table* table) {
	blocks_table = table;
	return *this;
}

void BlockCacheUpdaterBuffer::PushUpdate(const int block_id, Block* block) {
	blocks[block_id] = block;
	logger <<
		"timeout = "
		+ std::to_string(std::time(0) - last_update_time)
		+ "(" + std::to_string(std::time(0)) + ", " + std::to_string(last_update_time)  + ")"
		+ " blocks count = "
		+ std::to_string(blocks.size());
	if (std::time(0) - last_update_time > timeout || max_blocks_count < blocks.size()) {
		Update();
		last_update_time = std::time(0);
	}
}

void BlockCacheUpdaterBuffer::Update() {
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		blocks_table->Insert(
			it->first,
			it->second->GetBlockName(),
			it->second->GetBlockType(),
			it->second->ToString()
		);
	}
	blocks_table->Execute();
	blocks.clear();
}




/*	Block	*/


Block::Block(
	const int id,
	const std::string& block_name,
	const std::string& block_type,
	Table* blocks_table
)
: block()
, id(id)
//* Эту переменную надо удалить, хранить имя блока в публичной переменной в BlockBase
//* удалено
, block_name(block_name)
, data()
, outgoing_edges()
, incoming_edges()
, blocks_table(blocks_table)
{
	boost::smatch match;
	if (
		boost::regex_match(
			block_type,
			match,
			boost::regex("Sum(\\d+)")
		)
	) {
		block = new Sum(std::stoi(match[1]), block_type);
	} else if (block_type == "PrintToLogs") {
		block = new PrintToLogs(block_type);
	} else if (block_type == "EmptyBlock") {
		block = new EmptyBlock(block_type);
	} else if (block_type == "TimeShift") {
		block = new TimeShift(block_type);
	} else if (block_type == "TimePeriodAggregator") {
		block = new TimePeriodAggregator(block_type);
		//* В общем коде не должно быть логики, относящейся к частным блокам,
		//* надо перенести эти две переменные в публичную часть BlockBase
		//* перенесла
	} else {
		throw GANException(649264, "Type " + block_type + " is incorret block type.");
	}

}


void Block::Load(const std::string& cache) {
	block->FromString(cache);
}



std::string Block::GetBlockType() const {
	return block->GetBlockType();
}

int Block::GetBlockId() const {
	return id;
}

std::string Block::GetBlockName() const {
	return block_name;
}

void Block::Verification() const {
	for (auto it = block->incoming_edges_names.begin(); it != block->incoming_edges_names.end(); ++it) {
		if (incoming_edges.count(*it) == 0) {
			throw GANException(529716, "Block " + block_name + " does not has all incoming edges.");

		}
	}
	for (auto it = block->params_names.begin(); it != block->params_names.end(); ++it) {
		if (block->param_values.count(*it) == 0) {
			throw GANException(29752, "Block " + block_name + " does not has all params.");
		}
	}
}

bool Block::DoesEdgeExist(std::string& incoming_edge_name) {
	if (block->incoming_edges_names.count(incoming_edge_name) == 0) {
		throw GANException(
			519720,
			"Edge with name " + incoming_edge_name  + " can't incoming to block " +  block_name + "."
		);
	}

	if (
		block->incoming_edges_names.count(incoming_edge_name) != 0 &&
		incoming_edges.count(incoming_edge_name) != 0
	) {
		return true;
	}
	return false;

}

bool Block::CanEdgeExist(std::string& incoming_edge_name) {
	if (block->incoming_edges_names.count(incoming_edge_name) == 0) {
		throw GANException(
			512320,
			"Edge with name " + incoming_edge_name  + " can't incoming to block " + block_name +  "."
		);
	}

	if (
		block->incoming_edges_names.count(incoming_edge_name) != 0 &&
		incoming_edges.count(incoming_edge_name) == 0
	) {
		return true;
	}
	return false;

}


void Block::AddIncomingEdge(Edge* edge) {
	std::string edge_name = edge->GetEdgeName();
	if (
		block->incoming_edges_names.count(edge_name) != 0 &&
		incoming_edges.count(edge_name) == 0
	) {
		incoming_edges[edge_name] = edge;
	} else {
		if (block->incoming_edges_names.count(edge_name) == 0) {
			throw GANException(
				238536,
				"Edge with name " + edge_name  +  " can't enter to block " + block->block_type + "."
			);
		} else {
			throw GANException(194527, "Edge with name " + edge_name  +  " already exist" );
		}
	}

}

void Block::DeleteIncomingEdge(const std::string& edge_name) {
	if (incoming_edges.count(edge_name) != 0) {
		incoming_edges.erase(edge_name);
	} else {
		throw GANException(264913, "Incoming edge with name " + edge_name  + " does not exist.");
	}
}

void Block::AddOutgoingEdgeToTable(Edge* edge, Table* blocks_and_outgoing_edges_table) {
	int edge_id = edge->GetEdgeId();
	logger <<
		"Insert intp table BlocksAndOutgoingEdges BlockId:"
		+ std::to_string(id)
		+ " EdgeId:"
		+ std::to_string(edge_id);
	blocks_and_outgoing_edges_table->Insert(id, edge_id);
	blocks_and_outgoing_edges_table->Execute();

}


void Block::AddOutgoingEdge(Edge* edge, Table* blocks_and_outgoing_edges_table) {
	outgoing_edges[edge->GetEdgeName()] = edge;

}


void Block::DeleteOutgoingEdge(const std::string& edge_name, Table* blocks_and_outgoing_edges_table) {
	if (outgoing_edges.count(edge_name) != 0) {
		outgoing_edges.erase(edge_name);

		logger << "DeleteOutgoingEdge from BlocksAndOutgoingEdges where BlockID = " + std::to_string(id);
		blocks_and_outgoing_edges_table->Delete("BlockID = " + std::to_string(id));
	} else {
		throw GANException(264193, "Outgoing edge with name " + edge_name  + " does not exist.");
	}
}

Edge* Block::GetOutgoingEdge(const std::string& edge_name) {
	if (outgoing_edges.count(edge_name) != 0) {
		return outgoing_edges[edge_name];
	} else {
		throw GANException(166231, "Edge with name " + edge_name  +  " does not exist between such blocks.");
	}
}

Edge* Block::GetIncomingEdge(const std::string& edge_name) {
	if (incoming_edges.count(edge_name) != 0) {
		return incoming_edges[edge_name];
	} else {
		throw GANException(161031, "Edge with name " + edge_name  + " does not exist between such block");
	}
}

bool Block::Check(const std::time_t& time) const {
	for (
		auto it = block->incoming_edges_names.begin();
		it != block->incoming_edges_names.end();
		++it
	) {
		if (data.at(time).count(*it) == 0) {
			return false;
		}
	}
	return true;
}

void Block::Insert(const Point& point, const std::string& edge_name, BlockCacheUpdaterBuffer* block_buffer) {
	std::time_t time = point.GetTime();
	data[time][edge_name] = point;
	if (Check(time)) {
		Point result =  block->Do(data[time], time);
		block_buffer->PushUpdate(id, this);
		data.erase(time);

		if (!result.IsEmpty()) {
			SendByAllEdges(result, block_buffer);
		}
	}
}


void Block::SendByAllEdges(const Point& point, BlockCacheUpdaterBuffer* block_buffer) const {
	for (
		auto it = outgoing_edges.begin();
		it != outgoing_edges.end();
		++it
	) {
		it->second->To()->Insert(point, it->first, block_buffer);
	}

}


void Block::AddParam(const std::string& param_name, const StringType& param_value) {
	if (block->params_names.count(param_name) == 0) {
		throw GANException(
			382654,
			"Param with name " + param_name + " in block " + block_name + " with type " +  block->block_type + " does not exist."
		);
	}
	block->param_values[param_name] = param_value;
}


std::string Block::ToString() {
	return block->ToString();
}


Block::~Block() {
	for (auto it = outgoing_edges.begin(); it != outgoing_edges.end(); ++it) {
		delete it->second;
	}
}



/*     Graph     */

Graph::Graph(
	const int id,
	const std::string& graph_name,
	Table* graphs_and_blocks_table,
	Table* blocks_table,
	Table* edges_table,
	Table* blocks_and_outgoing_edges_table,
	Table* blocks_params_table,
	const bool valid,
	BlockCacheUpdaterBuffer* block_buffer
)
: id(id)
, graph_name(graph_name)
, blocks()
, edges()
, graphs_and_blocks_table(graphs_and_blocks_table)
, blocks_table(blocks_table)
, edges_table(edges_table)
, blocks_and_outgoing_edges_table(blocks_and_outgoing_edges_table)
, blocks_params_table(blocks_params_table)
, valid(valid)
, block_buffer(block_buffer)
{
	Load();
}


std::string Graph::GetGraphName() const {
	return graph_name;
}

bool Graph::GetGraphValid() const {
	return valid;
}


void Graph::Load() {
	for (
		auto it = graphs_and_blocks_table->Select("GraphId = " + std::to_string(id));
		it != graphs_and_blocks_table->SelectEnd();
		++it
	){
		auto block_info = blocks_table->Select("Id = " + std::to_string(it["BlockId"]));
		Block* block = CreateBlock(block_info["Type"], block_info["Id"], block_info["BlockName"]);
		if (std::string(block_info["Cache"]) != std::string("")) {
			block->Load(
				block_info["Cache"]
			);
		}

	}
	for (auto block_it = blocks.begin(); block_it != blocks.end(); ++block_it) {
		Block* block = block_it->second;
		for (
			auto it = blocks_and_outgoing_edges_table->Select(
				"BlockId = " + std::to_string(block->GetBlockId())
			);
			it != blocks_and_outgoing_edges_table->SelectEnd();
			++it
		) {
			auto edge_info = edges_table->Select("Id = " + std::to_string(it["EdgeId"]));
			std::string edge_name = edge_info["EdgeName"];
			std::string to_name =
				blocks_table->Select("Id = " + std::to_string(edge_info["ToBlock"]))["BlockName"];

			CreateEdge(it["EdgeId"], edge_name, block->GetBlockName(), to_name);

		}

		for (
			auto it = blocks_params_table->Select("BlockId = " + std::to_string(block->GetBlockId()));
			it != blocks_params_table->SelectEnd();
			++it
		) {
			block->AddParam(it["ParamName"], it["ParamValue"]);
		}
	}

}


void Graph::AddBlockToTables(Block* block) {
	int block_id = block->GetBlockId();
	std::string block_name = block->GetBlockName();
	std::string block_type = block->GetBlockType();
	logger <<
		"Insert into table GraphsAndBlocks GraphId:"
		+ std::to_string(id)
		+ " BlockId:"
		+ std::to_string(block_id);

	graphs_and_blocks_table->Insert(id, block_id);
	graphs_and_blocks_table->Execute();

	logger <<
		"Insert into table Blocks Id:"
		+ std::to_string(block_id)
		+ " BlockName:"
		+ block_name
		+ " Type:"
		+ block_type
		+ " State:";

	blocks_table->Insert(block_id, block_name, block_type, std::string(""));
	blocks_table->Execute();
}


Block* Graph::CreateBlock(
	const std::string& block_type,
	const int block_id,
	const std::string& block_name
) {

	if (blocks.count(block_name) == 0) {
		Block* block = new Block(block_id, block_name, block_type, blocks_table);
		blocks[block_name] = block;
		return block;
	} else {
		return	blocks[block_name];
	}
}


void Graph::DeleteBlock(const std::string& block_name) {
	if (blocks.count(block_name) != 0) {
		Block* block = blocks[block_name];
		std::vector<Edge*> edges;
		for (
			auto it = block->incoming_edges.begin();
			it != block->incoming_edges.end();
			++it
		) {
			edges.push_back(it->second);
		}

		for (
			auto it = block->outgoing_edges.begin();
			it != block->outgoing_edges.end();
			++it
		) {
			edges.push_back(it->second);
		}

		for (size_t i = 0; i < edges.size(); ++i) {
			DeleteEdge(edges[i]);
		}

		int block_id = block->GetBlockId();
		delete block;
		blocks.erase(block_name);

		logger <<
			"Delete from GraphsAndBlocks where GraphId = "
			+ std::to_string(id) +
			" and BlockId = "
			+ std::to_string(block_id);

		graphs_and_blocks_table->Delete(
			"GraphId = "
			+ std::to_string(id)
			+ " and BlockId = "
			+ std::to_string(block_id)
		);

		logger << "Delete from Blocks where Id = " + std::to_string(block_id);
		blocks_table->Delete("Id = " + std::to_string(block_id));


	} else {
		throw GANException(529471, "Block with name " + block_name  + " does not exist.");
	}
}


void Graph::DeleteGraph() {
	std::vector<std::string> blocks_names;
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		blocks_names.push_back(it->first);
	}

	for (size_t i = 0; i < blocks_names.size(); ++i) {
		DeleteBlock(blocks_names[i]);
	}


}

bool Graph::In(const std::string& block_name) const {
	if (blocks.count(block_name) != 0) {
		return true;
	}
	return false;
}

void Graph::AddEdgeToTables(Edge* edge) {
	int edge_id = edge->GetEdgeId();
	std::string edge_name = edge->GetEdgeName();
	int to_id = edge->To()->GetBlockId();

	logger << "Insert into Edges Id:"
		+ std::to_string(edge_id)
		+ " EdgeName:"
		+ edge_name
		+ " BlockTo:"
		+ std::to_string(to_id);
	edges_table->Insert(edge_id, edge_name, to_id);
	edges_table->Execute();
	edge->From()->AddOutgoingEdgeToTable(edge, blocks_and_outgoing_edges_table);

}

bool Graph::DoesEdgeExist(const std::string& block_name, std::string& incoming_edge_name) {
	if (blocks.count(block_name)) {
		return blocks[block_name]->DoesEdgeExist(incoming_edge_name);
	}
	throw GANException(220561, "Block with name " + block_name  +  " does not exist.");
}


bool Graph::CanEdgeExist(const std::string& block_name, std::string& incoming_edge_name) {
	if (blocks.count(block_name)) {
		return blocks[block_name]->CanEdgeExist(incoming_edge_name);
	}
	throw GANException(283561, "Block with name " + block_name  +  " does not exist.");
}

Edge* Graph::CreateEdge(
		const int edge_id,
		const std::string& edge_name,
		const std::string& from,
		const std::string& to
	) {
	if (blocks.count(from) != 0 && blocks.count(to) != 0) {
		Block* block_to = blocks[to];
		Block* block_from = blocks[from];
		Edge* edge = new Edge(edge_id, edge_name, block_from, block_to);

		block_to->AddIncomingEdge(edge);
		block_from->AddOutgoingEdge(edge, blocks_and_outgoing_edges_table);

		return edge;
	}
		throw GANException(239185, "Block with name "
			+ (blocks.count(from) == 0 ? from  : to)
			+ " does not exist."
		);

}


void Graph::DeleteEdge(
	const std::string& edge_name,
	const std::string& from,
	const std::string& to
) {
	if (blocks.count(from) != 0 && blocks.count(to) != 0) {
		Block* block_to = blocks[to];
		Block* block_from = blocks[from];

		Edge* edge = block_from->GetOutgoingEdge(edge_name);
		Edge* second_edge = block_to->GetIncomingEdge(edge_name);
		if (edge->GetEdgeId() != second_edge->GetEdgeId()) {
			throw GANException(258259, "Edge " + edge_name + " between blocks " + from + " and " + to + " does not exist.");
		}
		block_from->DeleteOutgoingEdge(edge_name, blocks_and_outgoing_edges_table);
		block_to->DeleteIncomingEdge(edge_name);

		logger <<
			"Delete from Edges where Id:"
			+ std::to_string(edge->GetEdgeId());

		edges_table->Delete("Id = " + std::to_string(edge->GetEdgeId()));
	} else {

		throw GANException(419248, "Block with name "
			+ (blocks.count(from) == 0 ? from  : to)
			+ " does not exist."
		);
	}

}


void Graph::DeleteEdge(Edge* edge) {
	DeleteEdge(edge->GetEdgeName(), edge->From()->GetBlockName(), edge->To()->GetBlockName());
}


int Graph::GetGraphId() const {
	return id;
}


std::string Graph::BFSFindCycle(
	std::unordered_map<std::string, bool>* used,
	const std::string& start_block
) {
	std::unordered_map<std::string, std::string> way;
	std::vector<std::string> use_now;
	use_now.push_back(start_block);
	std::unordered_map<std::string, bool> local_used;
	while (!use_now.empty()) {
		std::vector<std::string> new_use_now;
		for (size_t i = 0; i < use_now.size(); ++i) {
			Block* block = blocks[use_now[i]];
			for (
				auto edge = block->outgoing_edges.begin();
				edge != block->outgoing_edges.end();
				++edge
			) {
				std::string to = edge->second->To()->GetBlockName();
				if (!local_used[to]) {
					local_used[to] = true;
					used->operator[](to) = true;
					way[to] = block->GetBlockName();
					new_use_now.push_back(to);
				} else {
					std::string tmp = way[to];
					std::string sway = "\n" + tmp;

					while (tmp != to) {
						tmp = way[tmp];
						sway += "\n" + tmp;

					}
					return sway;
				}
			}
		}
		use_now = new_use_now;
	}

	return std::string("\0");

}


void Graph::Verification() {
	std::unordered_map<std::string, bool> used;
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		it->second->Verification();
		used[it->first] = false;
	}

	for (auto it = used.begin(); it != used.end(); ++it) {
		if (!it->second) {
			std::string cycle = BFSFindCycle(&used, it->first);
			if (cycle.size() != 0) {
				throw GANException(164920 ,"Graph has cycle: " + cycle);
			}
		}

	}

}


size_t Graph::IncomingEdgesCount(const std::string& block_name) const {
	if (blocks.count(block_name) != 0) {
		return blocks.at(block_name)->incoming_edges.size();
	} else {
		throw GANException(226503, "Block with name " + block_name + " does not exist.");
	}
}


void Graph::InsertPoint(const Point& point, const std::string& block_name) {
	if (!valid) {
		throw GANException(207530, "Graph " + graph_name + " is not valid, run 'deploy graph " + graph_name + "'.");
	}

	if (blocks.count(block_name) != 0) {
		if (IncomingEdgesCount(block_name) != 0) {
			Block* block = blocks[block_name];
			std::string edges = "";
			for (auto it = block->incoming_edges.begin();
				it != block->incoming_edges.end();
				++it
			) {
				edges += "\n" + it->first;
			}
			throw GANException(197529, "Block " + block_name  +  " has incoming edges:" + edges);
		}
		blocks[block_name]->SendByAllEdges(point, block_buffer);
	} else {
		throw GANException(287103, "Block with name " + block_name + " does not exist.");
	}
}


void Graph::InsertPointToAllPossibleBlocks(const Point& point) {
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		if (IncomingEdgesCount(it->first) == 0) {
			InsertPoint(point, it->first);
		}
	}
}

void Graph::ChangeGraphsValid(const bool new_valid) {
	valid = new_valid;
}

void Graph::AddParamToTable(
	const std::string& param_name,
	const StringType& param_value,
	const std::string& block_name
) {
	if (blocks.count(block_name) == 0) {
		throw GANException(283719, "Block with name " + block_name + "does not exist.");
	}

	int block_id = blocks[block_name]->GetBlockId();
	logger <<
		"Insert into table BlocksParams BlockId:"
		+ std::to_string(block_id)
		+ " ParamName:"
		+ param_name
		+ " ParamValue:"
		+ std::string(param_value);


	blocks_params_table->Insert(block_id, param_name, std::string(param_value));
	blocks_params_table->Execute();
}

void Graph::AddParam(const std::string& param_name, const StringType& param_value, const std::string& block_name) {
	if (blocks.count(block_name) == 0) {
		throw GANException(283719, "Block with name " + block_name + "does not exist.");
	}
	blocks[block_name]->AddParam(param_name, param_value);


}


Graph::~Graph() {
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		delete it->second;
	}
}


/*   WorkSpace    */

void WorkSpace::Load() {
	logger << "Load in WorkSpace";
	for (Table::rows it = graphs_table.Select("1"); it != graphs_table.SelectEnd(); ++it) {
		CreateGraph(int(it["Id"]), std::string(it["GraphName"]), (it["Valid"] == 0 ? false : true));
	}
}

WorkSpace::WorkSpace()
: graphs()
, graphs_table("GraphsTable|Id:int|GraphName:string,Valid:int")
, graphs_and_blocks_table("GraphsAndBlocks|GraphId:int,BlockId:int|")
, blocks_table("Blocks|Id:int|BlockName:string,Type:string,Cache:string")
, edges_table("Edges|Id:int|EdgeName:string,ToBlock:int")
, blocks_and_outgoing_edges_table("BlocksAndOutgoingEdges|BlockId:int,EdgeId:int|")
, blocks_params_table("BlocksParams|BlockId:int,ParamName:string|ParamValue:string")
, DaemonBase("127.0.0.1", "8081", 0)
, block_buffer()
{
	block_buffer.SetTable(&blocks_table);
	Load();
	Daemon();
}


std::string WorkSpace::Respond(const std::string& query)  {
	logger << "query = " + query;
	boost::smatch match;
	if (query == "") {
		return "";
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		int graph_id = graphs_table.MaxValue("Id") + 1;
		std::string graph_name = match[1];

		if (graphs.count(graph_name) != 0) {
			throw GANException(128463, "Graph with name " + graph_name   +  " already exists.");
		}

		AddGaphToTables(CreateGraph(graph_id, graph_name, 0));
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+graph\\s+if\\s+not\\s+exists\\s+(\\w+)\\s*")
		)
	) {
		int graph_id = graphs_table.MaxValue("Id") + 1;
		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			AddGaphToTables(CreateGraph(graph_id, graph_name, 0));
		}
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			throw GANException(483294, "Graph with name " + graph_name  +  " does not exist.");
		}

		int graph_id = graphs.at(graph_name)->GetGraphId();
		DeleteGraph(graph_id, graph_name);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+graph\\s+if\\s+exists\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];

		if (graphs.count(graph_name) != 0) {
			int graph_id = graphs.at(graph_name)->GetGraphId();
			DeleteGraph(graph_id, graph_name);
		}

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+vertex\\s+(\\w+):(\\w+)\\s+in\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string block_type = match[2];
		std::string graph_name = match[3];
		if (graphs.count(graph_name) == 0) {
			throw GANException(419294, "Graph with name " + graph_name  +  " does not exist.");

		}
		Graph* graph = graphs[graph_name];
		if (graph->In(block_name)) {
			throw GANException(
				428352,
				"Block with name" + block_name  + " already exists in graph " + graph_name +  "."
			);
		}

		int block_id = blocks_table.MaxValue("Id") + 1;
		graph->AddBlockToTables(
			graph->CreateBlock(block_type, block_id, block_name)
		);
		ChangeGraphsValid(graph_name, 0);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+vertex\\s+if\\s+not\\s+exists\\s+(\\w+):(\\w+)\\s+in\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string block_type = match[2];
		std::string graph_name = match[3];
		if (graphs.count(graph_name) == 0) {
			throw GANException(483294, "Graph with name " + graph_name  +  " does not exist.");

		}
		Graph* graph = graphs[graph_name];
		if (!graph->In(block_name)) {
			int block_id = blocks_table.MaxValue("Id") + 1;
			graph->AddBlockToTables(
				graph->CreateBlock(block_type, block_id, block_name)
			);

			ChangeGraphsValid(graph_name, 0);
		}
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+vertex\\s+(\\w+)\\s+in\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string graph_name = match[2];

		if (graphs.count(graph_name) == 0) {
			throw GANException(375920, "Graph with name " + graph_name  +  " does not exist.");
		}

		graphs[graph_name]->DeleteBlock(block_name);
		ChangeGraphsValid(graph_name, 0);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+vertex\\s+if\\s+exists\\s+(\\w+)\\s+in\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string graph_name = match[2];

		if (graphs.count(graph_name) != 0) {
			graphs[graph_name]->DeleteBlock(block_name);
			ChangeGraphsValid(graph_name, 0);
		}

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+edge\\s+(\\w+)\\s+in\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
		)
	) {
		std::string edge_name = match[1];
		std::string graph_name = match[2];
		std::string from_name = match[3];
		std::string to_name = match[4];
		int id = edges_table.MaxValue("Id") + 1;

		if (graphs.count(graph_name) == 0) {
			throw GANException(362796, "Graph with name " + graph_name  +  " does not exist.");
		}
		Graph* graph = graphs[graph_name];
		graph->AddEdgeToTables(
			graph->CreateEdge(id, edge_name, from_name, to_name));
		ChangeGraphsValid(graph_name, 0);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+edge\\s+if\\s+not\\s+exists\\s+(\\w+)\\s+in\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
		)
	)  {
		std::string edge_name = match[1];
		std::string graph_name = match[2];
		std::string from_name = match[3];
		std::string to_name = match[4];
		int id = edges_table.MaxValue("Id") + 1;

		if (graphs.count(graph_name) == 0) {
			throw GANException(362796, "Graph with name " + graph_name  +  " does not exist.");
		}
		Graph* graph = graphs[graph_name];
		if (graph->CanEdgeExist(to_name, edge_name)) {
			graph->AddEdgeToTables(
				graph->CreateEdge(id, edge_name, from_name, to_name));

			ChangeGraphsValid(graph_name, 0);
		}
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+edge\\s+(\\w+)\\s+in\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
		)
	) {
		std::string edge_name = match[1];
		std::string graph_name = match[2];
		std::string from_name = match[3];
		std::string to_name = match[4];


		if (graphs.count(graph_name) == 0) {
			throw GANException(362796, "Graph with name " + graph_name  +  " does not exist.");
		}

		graphs[graph_name]->DeleteEdge(edge_name, from_name, to_name);
		ChangeGraphsValid(graph_name, 0);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+edge\\s+if\\s+exists\\s+edge\\s+(\\w+)\\s+in\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
		)
	) {
		std::string edge_name = match[1];
		std::string graph_name = match[2];
		std::string from_name = match[3];
		std::string to_name = match[4];


		if (graphs.count(graph_name) == 0) {
			throw GANException(362796, "Graph with name " + graph_name  +  " does not exist.");
		}
		Graph* graph = graphs[graph_name];
		if (graph->DoesEdgeExist(to_name, edge_name)) {
			graph->DeleteEdge(edge_name, from_name, to_name);
			ChangeGraphsValid(graph_name, 0);

		}
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*deploy\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];
		if (graphs.count(graph_name) != 0) {
			Verification(graph_name);
		} else {
			throw GANException(263702, "Graph with name " + graph_name  +  " does not exist.");
		}

	} else if (
		boost::regex_match(
			query,
			match,
			//* Название серии может содержать любые символы, даже пробельные, здесь \\w не годится
			//* исправлено
			boost::regex("\\s*insert\\s+point\\s+'([\\w|\\s]+)':(\\d+):(\\-{0,1}\\d*.{0,1}\\d*)\\s+into\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string series_name = match[1];
		std::time_t time = std::time_t(std::stoi(match[2]));
		double value = std::stod(match[3]);
		std::string block_name = match[4];
		std::string graph_name = match[5];

		if (graphs.count(graph_name) == 0) {
			throw GANException(195702, "Graph with name " + graph_name  +  " does not exist.");
		}

		graphs[graph_name]->InsertPoint(Point(series_name, value, time), block_name);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*insert\\s+point\\s+'([\\w|\\s]+)':(\\d+):(\\-{0,1}\\d*.{0,1}\\d*)\\s+into\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string series_name = match[1];
		std::time_t time = std::time_t(std::stoi(match[2]));
		double value = std::stod(match[3]);
		std::string graph_name = match[4];

		if (graphs.count(graph_name) == 0) {
			throw GANException(195702, "Graph with name " + graph_name  +  " does not exist.");
		}
		graphs[graph_name]->InsertPointToAllPossibleBlocks(Point(series_name, value, time));
	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*modify\\s+param\\s+(\\w+)\\s+to\\s+(\\w+)\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string param_name = match[1];
		StringType param_value = std::string(match[2]);
		std::string block_name = match[3];
		std::string graph_name = match[4];

		if (graphs.count(graph_name) == 0) {
			throw GANException(195702, "Graph with name " + graph_name  +  " does not exist.");
		}
		Graph* graph = graphs[graph_name];
		graph->AddParam(param_name, param_value, block_name);
		graph->AddParamToTable(param_name, param_value, block_name);
		ChangeGraphsValid(graph_name, 0);


	} else {
		throw GANException(529352, "Incorrect query");
	}
	return "Ok\0";
}


void WorkSpace::AddGaphToTables(Graph* graph) {
	int graph_id = graph->GetGraphId();
	std::string graph_name = graph->GetGraphName();
	logger <<
		"Insert into table Graphs Id:"
		+ std::to_string(graph_id)
		+ " GraphName:"
		+ graph_name
		+ " Valid:0";

	graphs_table.Insert(graph->GetGraphId(), graph->GetGraphName(), 0);
	graphs_table.Execute();

}

Graph* WorkSpace::CreateGraph(const int graph_id, const std::string& graph_name, const bool valid) {
	Graph* graph = new Graph(
		graph_id,
		graph_name,
		&graphs_and_blocks_table,
		&blocks_table,
		&edges_table,
		&blocks_and_outgoing_edges_table,
		&blocks_params_table,
		valid,
		&block_buffer
	);
	graphs[graph_name] = graph;
	return graph;
}


void WorkSpace::DeleteGraph(const int graph_id, const std::string& graph_name) {
	Graph* graph = graphs[graph_name];
	graph->DeleteGraph();
	delete graph;
	graphs.erase(graph_name);

	logger << "Delete from Graphs where Id = " + std::to_string(graph_id);
	graphs_table.Delete("Id = " + std::to_string(graph_id));
	graphs_and_blocks_table.Delete("GraphId = " + std::to_string(graph_id));

}

void WorkSpace::ChangeGraphsValid(const std::string& graph_name, const int graphs_valid) {
	graphs[graph_name]->ChangeGraphsValid((graphs_valid == 0 ? false : true));
	int graph_id = graphs[graph_name]->GetGraphId();
	logger << "Insert into tablse Graphs Id:"
		+ std::to_string(graph_id)
		+ " GraphName:"
		+ graph_name
		+ " Valid:"
		+ std::to_string(graphs_valid);
	graphs_table.Insert(graph_id, graph_name, graphs_valid);
	graphs_table.Execute();
}


void WorkSpace::Verification(const std::string& graph_name) {
	graphs[graph_name]->Verification();
	ChangeGraphsValid(graph_name, 1);
}


WorkSpace::~WorkSpace() {
	for (auto it = graphs.begin(); it != graphs.end(); ++it) {
		delete it->second;
	}
}
