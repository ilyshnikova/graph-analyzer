#include <boost/regex.hpp>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <fstream>
#include "gan-exception.h"
#include "graph.h"
#include "logger.h"
#include "execute.h"
#include "yaml-cpp/yaml.h"

/*	AnswerTable	*/


std::string AnswerTable::ToString() const {
	std::vector<std::string> string_rows;
	for (size_t i = 0; i < rows.size(); ++i) {
		string_rows.push_back(BlockBase::Join(rows[i], std::string("\t")));
	}

	return 	status
		+ (head.size() == 0 ? "" : std::string("\n") + BlockBase::Join(head, std::string("\t")))
		+ (rows.size() == 0 ? "" : std::string("\n"))
		+ BlockBase::Join(string_rows, std::string("\n"));

}


/*	Point	*/

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

bool Point::operator==(const Point& other) const {
	return series_name == other.GetSeriesName() && value == other.GetValue() && time == other.GetTime();
}

bool Point::operator!=(const Point& other) const {
	return !(*this == other);
}

Point Point::Empty() {
	return Point();
}

Point::operator std::string() const {
        std::time_t current_time(1440930405);
        char mbstr[100];
        if (std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&current_time))) {
	return std::string(" Point(")
		+ "series_name: "
		+ series_name
		+ " value: "
		+ std::to_string(value)
		+ " time: "
		+ std::string(mbstr)
		+ ") ";
	}
}


std::ostream& operator<< (std::ostream& out, const Point& point) {
	out << std::string(point);
	return out;
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

YAML::Emitter& operator << (YAML::Emitter& out, const Edge& edge) {
	logger << "edge";
	out << YAML::BeginMap;
	out << YAML::Key << "name" << YAML::Value << edge.GetEdgeName();
	out << YAML::Key << "from" << YAML::Value << edge.From()->GetBlockName();
	out << YAML::Key << "to" << YAML::Value << edge.To()->GetBlockName();
	out << YAML::EndMap;
	return out;
}


/*   BlockBase    */


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


std::string BlockBase::Join(const std::vector<std::string>& strings, const std::string separator) {
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


/*	Reducer 	*/

std::unordered_set<std::string> Reducer::CreateIncomingEdges(const int edges_count, const std::string& edges_name_type) const {
	std::unordered_set<std::string> edges;
	for (size_t i = 0; i < edges_count; ++i) {
		edges.insert(edges_name_type + std::to_string(i + 1));
	}
	return edges;

}

Reducer::Reducer(
	const std::string& block_type,
	const std::string& edges_name_type,
	const int edges_count
)
: base_block_type(block_type)
, BlockBase(CreateIncomingEdges(edges_count, edges_name_type), block_type + std::to_string(edges_count), {}, {})
{}

Point Reducer::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	double res_value = StartValue();
	for (auto it = values.cbegin(); it != values.cend(); ++it) {
		res_value = BaseFunction(res_value, it->second.GetValue());
	}

	return Point(GetResultSeriesName(values), res_value, time);

}

BlockBase* Reducer::GetBlock(const std::string& type) const {
	return NULL;
}

std::string Reducer::Description() const {
	return "";

}

/*	Sum	*/

Sum::Sum(const int edges_count)
: Reducer("Sum", "arg", edges_count)
{}

double Sum::BaseFunction(const double first, const double second) const {
	return first + second;
}

double Sum::StartValue() const {
	return double(0);
}

BlockBase* Sum::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (
		boost::regex_match(
			type,
			match,
			boost::regex("Sum(\\d+)")
		)
	) {
		return new Sum(std::stoi(match[1]));
	}
	return NULL;

}

std::string Sum::Description() const {
	return std::string("\tSumN : This block is used for summarize incoming  points values.\n")
		+ "\t\tN - param that specifying count of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}


/*	And	*/

And::And(const int edges_count)
: Reducer("And", "arg", edges_count)
{}

double And::BaseFunction(const double first, const double second) const {
	return first && second;
}

double And::StartValue() const {
	return double(1);
}

BlockBase* And::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (
		boost::regex_match(
			type,
			match,
			boost::regex("And(\\d+)")
		)
	) {
		return new And(std::stoi(match[1]));
	}
	return NULL;

}

std::string And::Description() const {
	return std::string("\tAndN : This block is used for logical and with incoming points values.\n")
		+ "\t\tN - param that specifying count of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}


/*	Or	*/

Or::Or(const int edges_count)
: Reducer("Or", "arg", edges_count)
{}

double Or::BaseFunction(const double first, const double second) const {
	return first || second;
}

double Or::StartValue() const {
	return double(0);
}

BlockBase* Or::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (
		boost::regex_match(
			type,
			match,
			boost::regex("Or(\\d+)")
		)
	) {
		return new Or(std::stoi(match[1]));
	}
	return NULL;

}

std::string Or::Description() const {
	return std::string("\tOrN : This block is used for logical or with incoming points values.\n")
		+ "\t\tN - param that specifying count of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}

/*	Min	*/

Min::Min(const int edges_count)
: Reducer("Min", "arg", edges_count)
{}

double Min::BaseFunction(const double first, const double second) const {
	if (first < second) {
		return first;
	}
	return second;
}

double Min::StartValue() const {
	return std::numeric_limits<double>::max();
}

BlockBase* Min::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (
		boost::regex_match(
			type,
			match,
			boost::regex("Min(\\d+)")
		)
	) {
		return new Min(std::stoi(match[1]));
	}
	return NULL;

}

std::string Min::Description() const {
	return std::string("\tMinN : This block returns incoming points with min values.\n")
		+ "\t\tN - param that specifying count of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}


/*	Max	*/

Max::Max(const int edges_count)
: Reducer("Max", "arg", edges_count)
{}

double Max::BaseFunction(const double first, const double second) const {
	if (first > second) {
		return first;
	}
	return second;
}

double Max::StartValue() const {
	return std::numeric_limits<double>::min();
}

BlockBase* Max::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (
		boost::regex_match(
			type,
			match,
			boost::regex("Max(\\d+)")
		)
	) {
		return new Max(std::stoi(match[1]));
	}
	return NULL;

}

std::string Max::Description() const {
	return std::string("\tMaxN : This block returns incoming points with max values..\n")
		+ "\t\tN - param that specifying count of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}

/*	Multiplication	*/

Multiplication::Multiplication(const int edges_count)
: Reducer("Multiplication", "arg", edges_count)
{}

double Multiplication::BaseFunction(const double first, const double second) const {
	return first * second;
}

double Multiplication::StartValue() const {
	return double(1);
}

BlockBase* Multiplication::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (
		boost::regex_match(
			type,
			match,
			boost::regex("Multiplication(\\d+)")
		)
	) {
		return new Multiplication(std::stoi(match[1]));
	}
	return NULL;

}

std::string Multiplication::Description() const {
	return std::string("\tMultiplicationN : This block is used for multiplicate incoming  points values.\n")
		+ "\t\tN - param that specifying count of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";

}




/*   EmptyBlock    */


EmptyBlock::EmptyBlock()
: BlockBase({}, "EmptyBlock", {}, {})
{}


Point EmptyBlock::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point::Empty();
}

BlockBase* EmptyBlock::GetBlock(const std::string& type) const {
	if (block_type == type) {
		return new EmptyBlock();
	}
	return NULL;
}

std::string EmptyBlock::Description() const {
	return "\tEmptyBlock: This block is used for inserting points into graph.";
}


/*	Difference	*/

Difference::Difference()
: BlockBase({"minuend", "subtrahend"}, std::string("Difference"), {}, {})
{}


Point Difference::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point(GetResultSeriesName(values), values.at("minuend").GetValue() - values.at("subtrahend").GetValue(), time);
}


BlockBase* Difference::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (type == block_type) {
		return new Difference();
	}
	return NULL;
}


std::string Difference::Description() const {
	return std::string("\tDifference : This block is used for subtracting incoming points values.\n")
		+ "\t\tIncoming edges: minuend,\n"
		+ "\t\t\t\tsubtracting.";
}


/*	Division	*/

Division::Division()
: BlockBase({"dividend", "divisor"}, std::string("Division"), {}, {})
{}


Point Division::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point(GetResultSeriesName(values), values.at("dividend").GetValue() / values.at("divisor").GetValue(), time);
}


BlockBase* Division::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (type == block_type) {
		return new Division();
	}
	return NULL;
}


std::string Division::Description() const {
	return std::string("\tDivision : This block is used for division incoming points values.\n")
		+ "\t\tIncoming edges: minuend,\n"
		+ "\t\t\t\tsubtracting.";
}


/*	Threshold	*/

Threshold::Threshold()
: BlockBase({"value"}, std::string("Threshold"), {"bound"}, {})
{}


Point Threshold::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	logger << "time : " + std::to_string(time);

	return Point(GetResultSeriesName(values), (values.at("value").GetValue() > param_values["bound"] ? 1 : 0), time);
}


BlockBase* Threshold::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (type == block_type) {
		return new Threshold();
	}
	return NULL;
}


std::string Threshold::Description() const {
	return std::string("\tThreshold : This block return point with value 1 if incoming points value grater than 'bound' param.\n")
		+ "\t\tIncoming edges: value.\n"
		+ "\t\tParams: bound.";
}


/*	Scale	*/

Scale::Scale()
: BlockBase({"to_scale"}, std::string("Scale"), {"value"}, {})
{}


Point Scale::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point(GetResultSeriesName(values), values.at("to_scale").GetValue() + param_values["value"], time);
}


BlockBase* Scale::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (type == block_type) {
		return new Scale();
	}
	return NULL;
}


std::string Scale::Description() const {
	return std::string("\tScale : This block summarize incoming points with value param 'value'.\n")
		+ "\t\tIncoming edges: to_scale.\n"
		+ "\t\tParams: value.";
}



/*	PrintToLogs	*/

PrintToLogs::PrintToLogs()
: BlockBase({"to_print"}, "PrintToLogs", {}, {})
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

BlockBase* PrintToLogs::GetBlock(const std::string& type) const {
	if (block_type == type) {
		return new PrintToLogs();
	}
	return NULL;
}


std::string PrintToLogs::Description() const {
	return std::string("\tPrintToLogs : This block is used for ptinting point to log.\n")
		+ "\t\tIncoming edges: to_print.";
}



/*	TimeShift	*/

TimeShift::TimeShift()
:BlockBase({"to_shift"}, "TimeShift", {"time_shift"}, {})
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

BlockBase* TimeShift::GetBlock(const std::string& type) const {
	if (block_type == type) {
		return new TimeShift();
	}
	return NULL;
}


std::string TimeShift::Description() const {
	return std::string("\tTimeShift : This block is used for shifting points times to some value.\n")
		+ "\t\tIncoming edges: to_print.\n"
		+ "\t\tPrams: time_shift -- shift value.\n";
}


/*	SendEmail	*/

SendEmail::SendEmail()
:BlockBase({"to_send"}, "SendEmail", {"email"}, {})
{}

Point SendEmail::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	for (auto it = values.begin(); it != values.end(); ++it) {
		if (it->second.GetValue()) {
			std::string message = std::string("echo \"")
				+ std::string(it->second)
				+ "\" | mail -s \" GAN massage \" \"" + std::string(param_values["email"]) + "\"";
			logger << "send email : " + message;
			ExecuteHandler eh(
				message.c_str()
			);
			return it->second;
		}
	}
}

BlockBase* SendEmail::GetBlock(const std::string& type) const {
	if (block_type == type) {
		return new SendEmail();
	}
	return NULL;
}


std::string SendEmail::Description() const {
	return std::string("\tSendEmail : This block is used for sending email if points series name == true\n")
		+ "\t\tIncoming edges: to_send.\n"
		+ "\t\tPrams: mail.\n";
}




/*	TimePeriodAggregator	*/

TimePeriodAggregator::TimePeriodAggregator()
: BlockBase(
	{"to_aggregate"},
	"TimePeriodAggregator",
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


BlockBase* TimePeriodAggregator::GetBlock(const std::string& type) const {
	if (block_type == type) {
		return new TimePeriodAggregator();
	}
	return NULL;
}


std::string TimePeriodAggregator::Description() const {
	return std::string("\tTimePeriodAggregator : This block rounding points time and summarize points values with same worth\n")
		+ "\t\tIncoming edges: to_aggregate.\n"
		+ "\t\tParams: round_time -- rounding precision,\n"
		+ "\t\t\tkeep_history_interval -- points with greater than this time will not be taken into account,\n"
		+ "\t\t\tmin_bucket_points -- minimum number of points with same time, which required for creating outgoing point.";
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
	BlockCacheUpdaterBuffer* block_buffer
)
: block()
, id(id)
, block_name(block_name)
, data()
, outgoing_edges()
, incoming_edges()
, block_buffer(block_buffer)
, blocks({
	new Sum(1),
	new PrintToLogs(),
	new EmptyBlock(),
	new TimeShift(),
	new TimePeriodAggregator(),
	new Difference(),
	new Division(),
	new Multiplication(1),
	new And(1),
	new Or(1),
	new Min(1),
	new Max(1),
	new Scale(),
	new Threshold(),
	new SendEmail(),
	})
{
	for (size_t i = 0; i < blocks.size(); ++i) {
		block = blocks[i]->GetBlock(block_type);
		if (block != NULL) {
			return;
		}
	}
	throw GANException(649264, "Type " + block_type + " is incorret block type.");
}


BlockBase* Block::GetBlock() const {
	return block;
}

std::string Block::GetAllBlocksDescriptions() const {
	std::string res;
	for (size_t i = 0; i < blocks.size(); ++i) {
		res += blocks[i]->Description() + "\n";
	}
	return res;

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


void Block::AddOutgoingEdge(Edge* edge) {
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



void Block::Insert(const Point& point, const std::string& edge_name) {
	std::time_t time = point.GetTime();
	data[time][edge_name] = point;
	if (Check(time)) {
		Point result =  block->Do(data[time], time);
		block_buffer->PushUpdate(id, this);
		data.erase(time);

		if (!result.IsEmpty()) {
			SendByAllEdges(result);
		}
	}
}


void Block::SendByAllEdges(const Point& point) const {
	for (
		auto it = outgoing_edges.begin();
		it != outgoing_edges.end();
		++it
	) {
		it->second->To()->Insert(point, it->first);
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

std::vector<std::vector<std::string> > Block::GetParams() const {
	std::vector<std::vector<std::string> > params;

	for (auto it = block->params_names.begin(); it != block->params_names.end(); ++it) {
		params.push_back({
			*it,
			(block->param_values.count(*it) == 0 ? std::string("") : std::string(block->param_values[*it]))
		});
	}

	return params;
}

std::vector<std::vector<std::string> > Block::GetPossibleEdges() const {
	std::vector<std::vector<std::string> > possible_edges;
	for (auto it = block->incoming_edges_names.cbegin(); it != block->incoming_edges_names.cend(); ++it) {
		possible_edges.push_back({*it});
	}
	return possible_edges;
}

YAML::Emitter& operator << (YAML::Emitter& out, const Block& block) {
	logger << "block";
	std::map<std::string, std::string> params;
	for (auto it = block.GetBlock()->param_values.cbegin(); it !=  block.GetBlock()->param_values.cend(); ++it) {
		params[it->first] = std::string(it->second);
	}
	out << YAML::BeginMap;
	out << YAML::Key << "name" << YAML::Value << block.GetBlockName();
	out << YAML::Key << "type" << YAML::Value << block.GetBlockType();
	out << YAML::Key << "params" << YAML::Value << params;
	out << YAML::EndMap;
	return out;
}


Block::~Block() {
	for (auto it = outgoing_edges.begin(); it != outgoing_edges.end(); ++it) {
		delete it->second;
	}
	for (size_t i = 0; i < blocks.size(); ++i) {
		delete blocks[i];
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
		Block* block = new Block(block_id, block_name, block_type, block_buffer);
		blocks[block_name] = block;
		return block;
	} else {
		return	blocks[block_name];
	}
}


void Graph::DeleteBlock(const std::string& block_name) {
	if (blocks.count(block_name) != 0) {
		Block* block = blocks[block_name];
		std::vector<Edge*> blocks_edges;

		for (
			auto it = block->outgoing_edges.begin();
			it != block->outgoing_edges.end();
			++it
		) {
			blocks_edges.push_back(it->second);
		}

		for (
			auto it = block->incoming_edges.begin();
			it != block->incoming_edges.end();
			++it
		) {
			blocks_edges.push_back(it->second);
		}


		for (size_t i = 0; i < blocks_edges.size(); ++i) {
			if (
				block->incoming_edges.count(blocks_edges[i]->GetEdgeName()) != 0
				|| block->outgoing_edges.count(blocks_edges[i]->GetEdgeName()) != 0
			) {
				edges.erase(blocks_edges[i]);
				DeleteEdge(blocks_edges[i]);
			}
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
		block_from->AddOutgoingEdge(edge);

		edges.insert(edge);
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
		logger << "delete edge " + edge_name;
		Block* block_to = blocks[to];
		Block* block_from = blocks[from];

		Edge* edge = block_from->GetOutgoingEdge(edge_name);
		Edge* second_edge = block_to->GetIncomingEdge(edge_name);

		logger << edge << second_edge;

		if (edge->GetEdgeId() != second_edge->GetEdgeId()) {
			throw GANException(258259, "Edge " + edge_name + " between blocks " + from + " and " + to + " does not exist.");
		}
		edges.erase(edge);
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

// white == 0
// grey == 1
// black == 2

std::string Graph::RecDFSFindCycle(
	std::unordered_map<std::string, int>* colors,
	std::unordered_map<std::string, std::string>*  way,
	std::string& block_name
) const {
	Block* block = blocks.at(block_name);
	for (auto edge = block->outgoing_edges.cbegin(); edge != block->outgoing_edges.cend(); ++edge) {
		std::string to = edge->second->To()->GetBlockName();
		if (colors->operator[](to) == 0) {
			colors->operator[](to) = 1;
			way->operator[](to) = block->GetBlockName();
			std::string res = RecDFSFindCycle(colors, way, to);
			colors->operator[](to) = 2;
			if (res != std::string("\0")) {
				return res;
			}
		} else if (colors->operator[](to) == 1) {
			way->operator[](to) = block->GetBlockName();
			return to;
		}
	}
	return std::string("\0");
}


std::string Graph::DFSFindCycle(
	std::unordered_map<std::string, int>* colors,
	std::string start_block
) const {
	std::unordered_map<std::string, std::string> way;
	colors->operator[](start_block) = 1;
	std::string block_in_cycle = RecDFSFindCycle(colors, &way, start_block);
	colors->operator[](start_block) = 2;
	if (block_in_cycle != std::string("\0")) {
		std::string tmp = way[block_in_cycle];
		std::string sway = "\n" + tmp;
		while (tmp != block_in_cycle) {
			tmp = way[tmp];
			sway += "\n" + tmp;
		}
		return sway;
	}

	return std::string("\0");
}



void Graph::Verification() {
	std::unordered_map<std::string, int> used;
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		it->second->Verification();
		used[it->first] = 0;
	}

	for (auto it = used.begin(); it != used.end(); ++it) {
		if (it->second == 0) {
			logger << "start DFSFindCycle";
			std::string cycle = DFSFindCycle(&used, it->first);
			if (cycle != std::string("\0")) {
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
		blocks[block_name]->SendByAllEdges(point);
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
		throw GANException(283719, "Block with name " + block_name + " does not exist.");
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


std::vector<std::vector<std::string> > Graph::GetBlocksNames() const {
	std::vector<std::vector<std::string> > blocks_names_table;

	for (auto it = blocks.cbegin(); it != blocks.cend(); ++it) {
		blocks_names_table.push_back({it->first, it->second->GetBlockType()});
	}
	return blocks_names_table;
}

std::vector<std::vector<std::string> > Graph::GetBlocksParams(const std::string& block_name) const {
	if (blocks.count(block_name) == 0) {
		throw GANException(234245, "Block with name " + block_name + " does not exist.");
	}
	return blocks.at(block_name)->GetParams();
}

std::vector<std::vector<std::string> > Graph::GetEdges() const {
	std::vector<std::vector<std::string> > edges_table;
	for (auto it = edges.cbegin(); it != edges.cend(); ++it) {
		Edge* edge = *it;
		edges_table.push_back({edge->From()->GetBlockName(), edge->GetEdgeName(), edge->To()->GetBlockName()});
	}

	return edges_table;
}


std::vector<std::vector<std::string> > Graph::GetPossibleEdges(const std::string& block_name) const {
	if (blocks.count(block_name) == 0) {
		throw GANException(234245, "Block with name " + block_name + " does not exist.");
	}

	return blocks.at(block_name)->GetPossibleEdges();
}


std::string Graph::GetBlockType(const std::string& block_name) const {
	if (blocks.count(block_name) == 0) {
		throw GANException(234245, "Block with name " + block_name + " does not exist.");
	}

	return blocks.at(block_name)->GetBlockType();
}


void Graph::SaveGraphToFile(const std::string& file_name) const {
	std::ofstream config;
	config.open(file_name);
	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "blocks";
	out << YAML::Value << YAML::BeginSeq;
	for (auto it = blocks.cbegin(); it != blocks.cend(); ++it) {
		out << *(it->second);
	}
	out << YAML::EndSeq;

	out << YAML::Key << "edges";
	out << YAML::Value;
	out <<  YAML::BeginSeq;
	logger << edges.size();
	for (auto it = edges.cbegin(); it != edges.cend(); ++it) {
		logger << "add edge";
		out << *(*it);
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;
	config  << out.c_str();
	config.close();
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
	if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*")
		)
	) {
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
			boost::regex("\\s*create\\s+block\\s+(\\w+):(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s*")
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
				"Block with name " + block_name  + " already exists in graph " + graph_name +  "."
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
			boost::regex("\\s*create\\s+block\\s+if\\s+not\\s+exists\\s+(\\w+):(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s*")
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
			boost::regex("\\s*delete\\s+block\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s*")
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
			boost::regex("\\s*delete\\s+block\\s+if\\s+exists\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s*")
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
			boost::regex("\\s*create\\s+edge\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
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
			boost::regex("\\s*create\\s+edge\\s+if\\s+not\\s+exists\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
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
			boost::regex("\\s*delete\\s+edge\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
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
			boost::regex("\\s*delete\\s+edge\\s+if\\s+exists\\s+edge\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
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
			boost::regex("\\s*insert\\s+point\\s+'(.+)':(\\d+):(\\-{0,1}\\d*.{0,1}\\d*)\\s+into\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
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
			boost::regex("\\s*insert\\s+point\\s+'(.+)':(\\d+):(\\-{0,1}\\d*.{0,1}\\d*)\\s+into\\s+graph\\s+(\\w+)\\s*")
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
			boost::regex("\\s*modify\\s+param\\s+(\\w+)\\s+to\\s+(.+)\\s+in\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
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

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+graphs\\s*")
		)
	) {
		AnswerTable ans;
		ans.head = {"GraphName"};

		for (auto it = graphs.begin(); it != graphs.end(); ++it) {
			ans.rows.push_back({it->first});
		}
		ans.status = "Ok";

		return  ans.ToString();

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+blocks\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			throw GANException(132464, "Graph with name " + graph_name  +  " does not exist.");
		}
		AnswerTable ans;
		ans.head = {"Name", "Type"};
		ans.rows = graphs[graph_name]->GetBlocksNames();
		ans.status = "Ok";
		return ans.ToString();

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+params\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string graph_name = match[2];
		if (graphs.count(graph_name) == 0) {
			throw GANException(131464, "Graph with name " + graph_name  +  " does not exist.");
		}

		AnswerTable ans;
		ans.head = {"Name", "Value"};
		ans.rows = graphs[graph_name]->GetBlocksParams(block_name);
		ans.status = "Ok";
		return ans.ToString();
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+edges\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			throw GANException(123264, "Graph with name " + graph_name  +  " does not exist.");
		}
		AnswerTable ans;
		ans.head = {"From", "EdgeName", "To"};
		ans.rows = graphs[graph_name]->GetEdges();
		ans.status = "Ok";
		return ans.ToString();
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*is\\s+graph\\s+(\\w+)\\s+deployed\\s*")
		)
	) {

		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			throw GANException(142264, "Graph with name " + graph_name  +  " does not exist.");
		}
		AnswerTable ans;
		ans.head = {"GraphDeployed"};
		ans.rows = {{((graphs[graph_name]->GetGraphValid() == 1) ? std::string("Yes") : std::string("No"))}};
		ans.status = "Ok";
		return ans.ToString();
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+possible\\s+edges\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string graph_name = match[2];
		if (graphs.count(graph_name) == 0) {
			throw GANException(135264, "Graph with name " + graph_name  +  " does not exist.");
		}

		AnswerTable ans;
		ans.head = {"EdgeName"};
		ans.rows = graphs[graph_name]->GetPossibleEdges(block_name);
		ans.status = "Ok";
		return ans.ToString();

	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+block\\s+type\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::string block_name = match[1];
		std::string graph_name = match[2];
		if (graphs.count(graph_name) == 0) {
			throw GANException(135224, "Graph with name " + graph_name  +  " does not exist.");
		}
		AnswerTable ans;
		ans.head = {"BlockType"};
		ans.rows = {{graphs[graph_name]->GetBlockType(block_name)}};
		ans.status = "Ok";
		return ans.ToString();
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*save\\s+graph\\s+(\\w+)\\s+to\\s+file\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];
		std::string file_name = match[2];

		if (graphs.count(graph_name) == 0) {
			throw GANException(131564, "Graph with name " + graph_name  +  " does not exist.");
		}

		graphs[graph_name]->SaveGraphToFile(file_name);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*load\\s+graph\\s+(\\w+)\\s+from\\s+file\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];
		std::string file_name = match[2];

//		if (graphs.count(graph_name) != 0) {
//			throw GANException(131564, "Graph with name " + graph_name  +  " already exist.");
//		}

//		AddGaphToTables(CreateGraph(graphs_table.MaxValue("Id") + 1, graph_name, 0));
		return LoadGraphFromFile(file_name, graph_name, {std::string("create graph ") + graph_name}).ToString();

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*load\\s+replace\\s+graph\\s+(\\w+)\\s+from\\s+file\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];
		std::string file_name = match[2];

//		Respond(std::string("delete graph if exists ") + graph_name);
//		AddGaphToTables(CreateGraph(graphs_table.MaxValue("Id") + 1, graph_name, 0));
		return LoadGraphFromFile(
			file_name,
			graph_name,
			{std::string("delete graph if exists ") + graph_name, std::string("create graph ") + graph_name}
			).ToString();

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*convert\\s+config\\s+(\\w+)\\s+to\\s+queries\\s*")
		)
	) {
		std::string file_name = match[1];
		AnswerTable ans;
		ans.head = {"Query"};
		ans.rows = ConvertConfigToQueries(file_name);
		ans.status = "Ok";
		return ans.ToString();
	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*load\\s+ignore\\s+graph\\s+(\\w+)\\s+from\\s+file\\s+(\\w+)\\s*")
		)
	) {
		std::string graph_name = match[1];
		std::string file_name = match[2];

		if (graphs.count(graph_name) == 0) {
			AddGaphToTables(CreateGraph(graphs_table.MaxValue("Id") + 1, graph_name, 0));
			return LoadGraphFromFile(
				file_name,
				graph_name,
				{std::string("create graph ") + graph_name}
			).ToString();
		}


	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*help\\s*")
		)
	) {
		return std::string("Ok\n")
			+ "Queries:\n\tCreation/Deletion of objects:\n"
			+ "\t\tcreate|delete graph [if [not] exists] <graph_name>\n"
			+ "\t\tcreate|delete block [if [not] exists] <vertes_name>[:<vertex_type>] in graph <graph_name>\n"
			+ "\t\tcreate|delete edge [if [not] exists] <edge_name> in graph <graph_name> from <from_vertex_name> to <to_vertex_name>\n"
			+ "\tWork with graph:\n"
			+ "\t\tdeploy graph <graph_name> -- verify the absence of cycles and availability of all incoming edges in blocks\n"
			+ "\t\tis graph <graph_name> deployed -- return result of last graphs verification\n"
			+ "\t\tinsert point '<series_name>':<time>:<double_value> into [block <block_name> of] graph <graph_name> -- insert point to all blocks without incoming edges or to current block\n"
			+ "\t\tmodify param <param_name> to <param_value> in block <block_name> of graph <graph_name>\n"
			+ "\tShow Graph Structure:\n"
			+ "\t\tshow graphs\n"
			+ "\t\tshow blocks|edges of graph <graph_name>\n"
			+ "\t\tshow params|possible edges of block <block_name> of graph <graph_name>\n"
			+ "\t\tshow block type of block <block_name> of graph <graph_name>\n"
			+ "\t\thelp\n"
			+ "\tOperation with graph:\n"
			+ "\t\tsave graph <graph_name> to file <file_name>\n"
			+ "\t\tconvert config <file_name> to queries\n"
			+ "\t\tload [replace|ignore] graph <grpah_name> from file file_name -- in this query file with config is converted to sequence of requests and executed step by step\n"
			+ "Blocks:\n" + Block(1,"","EmptyBlock",NULL).GetAllBlocksDescriptions();
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

std::vector<std::vector<std::string> >  WorkSpace::ConvertConfigToQueries(const std::string& file_name, const std::string& graph_name ) const {
	std::ifstream fin(file_name);
	YAML::Parser parser(fin);
	YAML::Node graph_config;
	parser.GetNextDocument(graph_config);

	const YAML::Node& blocks_node = graph_config["blocks"];
 	const YAML::Node& edges = graph_config["edges"];

	std::vector<std::vector<std::string> > queries;

	for (size_t i = 0; i < blocks_node.size(); ++i) {
		std::string block_name;
		blocks_node[i]["name"] >> block_name;
		std::string block_type;
		blocks_node[i]["type"] >> block_type;
			std::string query =
			std::string("create block ")
				+ block_name
				+ ":"
				+ block_type
				+ " in graph "
				+ graph_name;
			queries.push_back({query});

		std::map<std::string, std::string> params;
		blocks_node[i]["params"] >> params;

		for (auto it = params.begin(); it != params.end(); ++it) {
			std::string query =
				std::string("modify param ")
				+ it->first
				+ " to "
				+ it->second
				+ " in block "
				+ block_name
				+ " of graph "
				+ graph_name;

			queries.push_back({query});
		}
	}

	for (size_t i = 0; i < edges.size(); ++i) {
		std::string edge_name;
		edges[i]["name"] >> edge_name;
		std::string from;
		edges[i]["from"] >> from;
		std::string to;
		edges[i]["to"] >> to;

		std::string query =
			std::string("create edge ")
			+ edge_name
			+ " in graph "
			+ graph_name
			+ " from "
			+ from
			+ " to "
			+ to;
		queries.push_back({query});
	}

	return queries;

}


AnswerTable WorkSpace::LoadGraphFromFile(
	const std::string& file_name,
	const std::string& graph_name,
	const std::vector<std::string>& first_queries
)  {
	std::vector<std::vector<std::string> > queries = ConvertConfigToQueries(file_name, graph_name);
	queries.push_back({std::string("deploy graph ") + graph_name});
	AnswerTable ans;
	ans.head = {"Query"};

	try {
		for (size_t i = 0; i < first_queries.size(); ++i) {
			ans.rows.push_back({first_queries[i]});
			Respond(first_queries[i]);
		}

		for (size_t i = 0; i < queries.size(); ++i) {
			ans.rows.push_back({queries[i][0]});
			Respond(queries[i][0]);
		}

	} catch (std::exception& e) {
		ans.rows.push_back({e.what()});
		ans.status = "Not Ok";
		return ans;
	}

	ans.status = "Ok";
	return ans;
}


WorkSpace::~WorkSpace() {
	for (auto it = graphs.begin(); it != graphs.end(); ++it) {
		delete it->second;
	}
}
