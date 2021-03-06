#include <boost/regex.hpp>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <fstream>
#include <json/json.h>
#include <fstream>
#include <cstdio>
#include <cmath>
#include "gan-exception.h"
#include "graph.h"
#include "logger.h"
#include "execute.h"
#include "yaml-cpp/yaml.h"


/*	Json	*/


std::vector<std::map<std::string, std::string> > CreatreTable(
	const std::vector<std::vector<std::string> >& matrix,
	const std::vector<std::string>& head
) {
	logger << matrix.size() << head.size();
	std::vector<std::map<std::string, std::string> > table(matrix.size());
	for (size_t i = 0; i < matrix.size(); ++i) {
		for (size_t j = 0; j < matrix[i].size(); ++j) {
			std::string key = head[j];
			table[i][key] = matrix[i][j];
		}
	}
	return table;
}

Json::Value CreateJsonTable(
	const std::vector<std::vector<std::string> >& matrix,
	const std::vector<std::string>& head
) {
	return CreateJson(CreatreTable(matrix, head));
}


Json::Value CreateJson(const std::string& value) {
	return Json::Value(value);
}

Json::Value CreateJson(const double value) {
	return Json::Value(value);
}

Json::Value CreateJson(const int value) {
	return Json::Value(value);
}


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
	std::time_t current_time(time);
	char mbstr[100];
	if (std::strftime(mbstr, sizeof(mbstr), "%A %c", std::localtime(&current_time))) {
		return std::string("Point(")
			+ "series_name: "
			+ series_name
			+ " value: "
			+ std::to_string(value)
			+ " time: "
			+ std::string(mbstr)
			+ ") ";
	}
	return std::string("");
}


std::ostream& operator<< (std::ostream& out, const Point& point) {
	out << std::string(point);
	return out;
}

Json::Value CreateJson(const Point& point) {
	Json::Value jpoint;
	jpoint["series_name"] = point.GetSeriesName();
	jpoint["value"] = point.GetValue();
	jpoint["time"] = (long long int)point.GetTime();

	return jpoint;
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
	const std::unordered_map<std::string, StringType>& param_values,
	const std::string& block_name_for_definition
)
: block_type(block_type)
, block_name_for_definition((block_name_for_definition == "") ? block_type : block_name_for_definition)
, incoming_edges_names(incoming_edges_names)
, params_names(params_names)
, param_values(param_values)
{}

std::string BlockBase::GetBlockType() const {
	return block_type;
}


std::string BlockBase::ToString() const {
	return std::string("\0");
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

std::unordered_set<std::string> Reducer::CreateIncomingEdges(const size_t edges_count, const std::string& edges_name_type) const {
	std::unordered_set<std::string> edges;
	for (size_t i = 0; i < edges_count; ++i) {
		edges.insert(edges_name_type + std::to_string(i + 1));
	}
	return edges;

}

Reducer::Reducer(
	const std::string& block_type,
	const std::string& edges_name_type,
	const size_t edges_count
)
: BlockBase(
	CreateIncomingEdges(edges_count, edges_name_type),
	block_type + std::to_string(edges_count),
	std::unordered_set<std::string>(),
	std::unordered_map<std::string, StringType>(),
	block_type + "\%d"
)

, base_block_type(block_type)
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

BlockBase::~BlockBase() {}

/*	Sum	*/

Sum::Sum(const size_t edges_count)
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
	return std::string("\tThis block is used for summing incoming  points' values.\n")
		+ "\t\t\%d - param that specifies number of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}


/*	And	*/

And::And(const size_t edges_count)
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
	return std::string("\tThis block is used for logical \"and\" between incoming points' values.\n")
		+ "\t\t\%d - param that specifying number of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}


/*	Or	*/

Or::Or(const size_t edges_count)
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
	return std::string("\tThis block is used for logical \"or\" between incoming points' values.\n")
		+ "\t\t\%d - param that specifying number of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}

/*	Min	*/

Min::Min(const size_t edges_count)
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
	return std::string("\tThis block returns incoming points with min values.\n")
		+ "\t\t\%d - param that specifying number of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}


/*	Max	*/

Max::Max(const size_t edges_count)
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
	return std::string("\tThis block returns incoming points with max values..\n")
		+ "\t\t\%d - param that specifying number of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";
}

/*	Multiplication	*/

Multiplication::Multiplication(const size_t edges_count)
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
	return std::string("\tThis block is used for multiply incoming  points' values.\n")
		+ "\t\t\%d - param that specifying number of edges.\n"
		+ "\t\tIncoming edges: arg1,\n"
		+ "\t\t\t\t...,\n"
		+ "\t\t\t\targN.";

}




/*   EmptyBlock    */


EmptyBlock::EmptyBlock()
: BlockBase(
	std::unordered_set<std::string>(),
	"EmptyBlock",
	std::unordered_set<std::string>(),
	std::unordered_map<std::string, StringType>()
)
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
	return "\tThis block is used for inserting points into graph.";
}


/*	Difference	*/

Difference::Difference()
: BlockBase(
	{"minuend", "subtrahend"},
	std::string("Difference"),
	std::unordered_set<std::string>(),
	std::unordered_map<std::string, StringType>()
)
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
	return std::string("\tThis block is used for subtracting incoming points' values.\n")
		+ "\t\tIncoming edges: minuend,\n"
		+ "\t\t\t\tsubtrahend.";
}


/*	Division	*/

Division::Division()
: BlockBase(
	{"dividend", "divisor"},
	std::string("Division"),
	std::unordered_set<std::string>(),
	std::unordered_map<std::string, StringType>()
)
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
	return std::string("\tThis block is used for division incoming points' values.\n")
		+ "\t\tIncoming edges: dividend,\n"
		+ "\t\t\t\tdivisor.";
}


/*	Threshold	*/

Threshold::Threshold()
: BlockBase(
	{"value"},
	std::string("Threshold"),
	{"bound"},
	std::unordered_map<std::string, StringType>()
)
{}


Point Threshold::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {

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
	return std::string("\tThis block return point with value 1 if incoming points' value grater than 'bound' param.\n")
		+ "\t\tIncoming edges: value.\n"
		+ "\t\tParams: bound.";
}


/*	Scale	*/

Scale::Scale()
: BlockBase(
	{"to_scale"},
	std::string("Scale"),
	{"value"},
	std::unordered_map<std::string, StringType>()
)
{}


Point Scale::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point(GetResultSeriesName(values), values.at("to_scale").GetValue() * param_values["value"], time);
}


BlockBase* Scale::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (type == block_type) {
		return new Scale();
	}
	return NULL;
}


std::string Scale::Description() const {
	return std::string("\tThis block multiply incoming points with value param 'value'.\n")
		+ "\t\tIncoming edges: to_scale.\n"
		+ "\t\tParams: value.";
}

/*	Move	*/

Move::Move()
: BlockBase(
	{"to_move"},
	std::string("Move"),
	{"value"},
	std::unordered_map<std::string, StringType>()
)
{}


Point Move::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	return Point(GetResultSeriesName(values), values.at("to_move").GetValue() + param_values["value"], time);
}


BlockBase* Move::GetBlock(const std::string& type) const {
	boost::smatch match;
	if (type == block_type) {
		return new Move();
	}
	return NULL;
}


std::string Move::Description() const {
	return std::string("\tThis block summarize incoming points with value param 'value'.\n")
		+ "\t\tIncoming edges: to_move.\n"
		+ "\t\tParams: value.";
}



/*	PrintToLogs	*/

PrintToLogs::PrintToLogs()
: BlockBase(
	{"to_print"},
	"PrintToLogs",
	std::unordered_set<std::string>(),
	std::unordered_map<std::string, StringType>()
	)
{}


Point PrintToLogs::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	for (auto it = values.cbegin(); it != values.cend(); ++it) {
		logger <<
			"PrintToLogs: Point: series name: "
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
	return std::string("\tThis block is used for printing point to log.\n")
		+ "\t\tIncoming edges: to_print.";
}



/*	TimeShift	*/

TimeShift::TimeShift()
:BlockBase(
	{"to_shift"},
	"TimeShift",
	{"time_shift"},
	std::unordered_map<std::string, StringType>()
)
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
	return std::string("\tThis block is used for shifting points' times to some value.\n")
		+ "\t\tIncoming edges: to_print.\n"
		+ "\t\tPrams: time_shift -- shift value.\n";
}


/*	SendEmail	*/

SendEmail::SendEmail()
:BlockBase(
	{"to_send"},
	"SendEmail",
	{"email"},
	std::unordered_map<std::string, StringType>()
)
{}

Point SendEmail::Do(
	const std::unordered_map<std::string, Point>& values,
	const std::time_t& time
) {
	for (auto it = values.begin(); it != values.end(); ++it) {
		logger << "send email: " + std::to_string(it->second.GetValue());
		if (it->second.GetValue()) {
			std::string message = std::string("echo \"")
				+ std::string(it->second)
				+ "\" | mail -s \" GAN massage \" \"" + std::string(param_values["email"]) + "\" &";
			logger << "send email : " + message;
			ExecuteHandler eh(
				"/root/gan-system/send_email \""  + std::string(it->second) + "\" " + "\"" + std::string(param_values["email"]) + "\""
			);
			return it->second;
		}
	}
	return Point::Empty();
}

BlockBase* SendEmail::GetBlock(const std::string& type) const {
	if (block_type == type) {
		return new SendEmail();
	}
	return NULL;
}


std::string SendEmail::Description() const {
	return std::string("\tThis block is used for sending email if points' value == true\n")
		+ "\t\tIncoming edges: to_send.\n"
		+ "\t\tPrams: email.\n";
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
			"Do in block TimePeriodAggregator: points number -- "
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
	return std::string("\tThis block rounding points' time and summarize points' values with same worth\n")
		+ "\t\tIncoming edges: to_aggregate.\n"
		+ "\t\tParams: round_time -- rounding precision,\n"
		+ "\t\t\tkeep_history_interval -- points with greater than this time will not be taken into account,\n"
		+ "\t\t\tmin_bucket_points -- minimal number of points with same time, which required for creating outgoing point.";
}



/*	BlockCacheUpdaterBuffer	*/

BlockCacheUpdaterBuffer::BlockCacheUpdaterBuffer()
: blocks()
, blocks_table(NULL)
, last_update_time(time(0))
, timeout(180)
, max_blocks_count(5000)
{}

BlockCacheUpdaterBuffer&  BlockCacheUpdaterBuffer::SetTable(Table* table) {
	blocks_table = table;
	return *this;
}

void BlockCacheUpdaterBuffer::RemoveBlock(const int block_id) {
	if (blocks.count(block_id)) {
		blocks.erase(block_id);
	}
}

void BlockCacheUpdaterBuffer::PushUpdate(const int block_id, Block* block) {
	blocks[block_id] = block;
	if (std::time(0) - last_update_time > timeout || max_blocks_count < blocks.size()) {
		Update();
		last_update_time = std::time(0);
	}
//	}
}

void BlockCacheUpdaterBuffer::Update() {
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		logger << "Handling block " << it->first << it->second;
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
	Graph* parent_graph,
	BlockCacheUpdaterBuffer* block_buffer
)
: block()
, id(id)
, block_name(block_name)
, parent_graph(parent_graph)
, data()
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
	new Move(),
	new Threshold(),
	new SendEmail(),
	})
, block_buffer(block_buffer)
, incoming_edges()
, outgoing_edges()

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
		res += blocks[i]->block_name_for_definition + " : " + blocks[i]->Description() + "\n";
	}
	return res;

}

std::vector<std::vector<std::string> > Block::GetTableOfBlocksDescriptions() const {
	std::vector<std::vector<std::string> > res(blocks.size());
	for (size_t i = 0; i < blocks.size(); ++i) {
		res[i].push_back(blocks[i]->block_name_for_definition);
		res[i].push_back(blocks[i]->Description());
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
			throw GANException(529716, "Block " + block_name + " does not have all incoming edges.");

		}
	}
	for (auto it = block->params_names.begin(); it != block->params_names.end(); ++it) {
		if (block->param_values.count(*it) == 0) {
			throw GANException(29752, "Block " + block_name + " does not have all params.");
		}
	}
}

bool Block::DoesEdgeExist(const std::string& incoming_edge_name) const {
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
		"Insert into table BlocksAndOutgoingEdges BlockId:"
		+ std::to_string(id)
		+ " EdgeId:"
		+ std::to_string(edge_id);
	blocks_and_outgoing_edges_table->Insert(id, edge_id);
	blocks_and_outgoing_edges_table->Execute();

}


void Block::AddOutgoingEdge(Edge* edge) {
	if (!outgoing_edges.count(edge->To()->GetBlockName())) {
		outgoing_edges[edge->To()->GetBlockName()] = std::unordered_map<std::string, Edge* >();
	}
	outgoing_edges[edge->To()->GetBlockName()][edge->GetEdgeName()] = edge;

}


void Block::DeleteOutgoingEdge(const std::string& to_block_name, const std::string& edge_name, Table* blocks_and_outgoing_edges_table) {
	if (
		outgoing_edges.count(to_block_name) != 0
		&& outgoing_edges[to_block_name].count(edge_name) != 0
	) {
		outgoing_edges[to_block_name].erase(edge_name);
		if (outgoing_edges[to_block_name].size() == 0) {
			outgoing_edges.erase(to_block_name);
		}

		logger << "DeleteOutgoingEdge from BlocksAndOutgoingEdges where BlockID = " + std::to_string(id);
		blocks_and_outgoing_edges_table->Delete("BlockID = " + std::to_string(id));
	} else {
		throw GANException(264193, "Outgoing edge with name " + edge_name  + " does not exist.");
	}
}

Edge* Block::GetOutgoingEdge(const std::string& to_block_name, const std::string& edge_name) {
	if (
		outgoing_edges.count(to_block_name) != 0
		&& outgoing_edges[to_block_name].count(edge_name) != 0
	) {
		return outgoing_edges[to_block_name][edge_name];
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
			if (parent_graph->IsDebugModeEnabled()) {
				Json::Value info = CreateJson(
					std::map<std::string, std::string>({
						{"type", "point_emmition"},
						{"block", block_name},
					})
				);


				info["point"] = CreateJson(result);
				Json::FastWriter writer;
				logger <<  writer.write(info);
				parent_graph->WriteToBedugInfo(writer.write(info));
			}
			SendByAllEdges(result);
		}
	}
}


void Block::SendByAllEdges(const Point& point) const {
	for (
		auto block_it = outgoing_edges.cbegin();
		block_it != outgoing_edges.cend();
		++block_it
	) {
		for (
			auto edge_it = outgoing_edges.at(block_it->first).cbegin();
			edge_it != outgoing_edges.at(block_it->first).cend();
			++edge_it
		) {
			edge_it->second->To()->Insert(point, edge_it->first);
		}
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


std::vector<std::vector<std::string> > Block::GetMissingEdges() const {
	std::vector<std::vector<std::string> > missing_edges;
	for (auto it = block->incoming_edges_names.cbegin(); it != block->incoming_edges_names.cend(); ++it) {
		if (!DoesEdgeExist(*it)) {
			missing_edges.push_back({*it});
		}
	}
	return missing_edges;
}



YAML::Emitter& operator << (YAML::Emitter& out, const Block& block) {
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
	if (block_buffer) {
		block_buffer->RemoveBlock(id);
	}

	for (
		auto block_it = outgoing_edges.begin();
		block_it != outgoing_edges.end();
		++block_it
	) {
		for (
			auto edge_it = outgoing_edges[block_it->first].begin();
			edge_it != outgoing_edges[block_it->first].end();
			++edge_it
		) {
			delete edge_it->second;
		}
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
, debug_mode(false)
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
		Block* block = new Block(block_id, block_name, block_type, this, block_buffer);
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
			auto block_it = block->outgoing_edges.begin();
			block_it != block->outgoing_edges.end();
			++block_it
		) {
			for (
				auto edge_it = block->outgoing_edges[block_it->first].begin();
				edge_it != block->outgoing_edges[block_it->first].end();
				++edge_it
			) {
				blocks_edges.push_back(edge_it->second);
			}
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
				|| (
					block->outgoing_edges.count(blocks_edges[i]->To()->GetBlockName()) != 0
					&& block->outgoing_edges[blocks_edges[i]->To()->GetBlockName()].count(blocks_edges[i]->GetEdgeName()) != 0
				)
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

		blocks_params_table->Delete(
			"BlockId = "
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

bool Graph::DoesEdgeExist(const std::string& block_name, const std::string& incoming_edge_name) {
	if (blocks.count(block_name)) {
		return blocks[block_name]->DoesEdgeExist(incoming_edge_name);
	}
	throw GANException(220561, "Block with name " + block_name  +  " does not exist.");
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
		Block* block_to = blocks[to];
		Block* block_from = blocks[from];

		Edge* edge = block_from->GetOutgoingEdge(to, edge_name);
		Edge* second_edge = block_to->GetIncomingEdge(edge_name);

		if (edge->GetEdgeId() != second_edge->GetEdgeId()) {
			throw GANException(258259, "Edge " + edge_name + " between blocks " + from + " and " + to + " does not exist.");
		}
		edges.erase(edge);
		block_from->DeleteOutgoingEdge(to, edge_name, blocks_and_outgoing_edges_table);
		block_to->DeleteIncomingEdge(edge_name);

		std::to_string(edge->GetEdgeId());

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
	for (
		auto block_it = block->outgoing_edges.cbegin();
		block_it != block->outgoing_edges.cend();
		++block_it
	) {
		for (
			auto edge = block->outgoing_edges[block_it->first].cbegin();
			edge != block->outgoing_edges[block_it->first].cend();
			++edge
		) {
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
	}
	return std::string("\0");
}


std::vector<std::vector<std::string> > Graph::DFSFindCycle(
	std::unordered_map<std::string, int>* colors,
	std::string start_block
) const {
	std::unordered_map<std::string, std::string> way;
	std::vector<std::vector<std::string> > vway;
	colors->operator[](start_block) = 1;
	std::string block_in_cycle = RecDFSFindCycle(colors, &way, start_block);
	colors->operator[](start_block) = 2;
	if (block_in_cycle != std::string("\0")) {
		std::string tmp = way[block_in_cycle];
		vway = {{tmp}};
		while (tmp != block_in_cycle) {
			tmp = way[tmp];
			vway.push_back({tmp});
		}
	}

	return vway;

}

std::vector<std::vector<std::string> > Graph::GetCycle() const {
	std::unordered_map<std::string, int> used;
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		used[it->first] = 0;
	}

	for (auto it = used.begin(); it != used.end(); ++it) {
		if (it->second == 0) {
			std::vector<std::vector<std::string> > cycle = DFSFindCycle(&used, it->first);
			if (cycle.size() != 0) {
				return cycle;
			}
		}

	}
	return std::vector<std::vector<std::string> >();

}


void  Graph::Verification() {
	std::unordered_map<std::string, int> used;
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		it->second->Verification();
		used[it->first] = 0;
	}

	for (auto it = used.begin(); it != used.end(); ++it) {
		if (it->second == 0) {
			std::vector<std::vector<std::string> > cycle = DFSFindCycle(&used, it->first);
			if (cycle.size() != 0) {
				throw GANException(245162, "Graph " + graph_name + " has cycle.");
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


bool Graph::DoesOutgoingEdgesExist(const std::string& block_name) const {
	if (blocks.count(block_name) != 0) {
		if (blocks.at(block_name)->outgoing_edges.size() == 0) {
			return false;
		} else {
			return true;
		}
	} else {
		throw GANException(226503, "Block with name " + block_name + " does not exist.");
	}
}


void Graph::InsertPoint(const Point& point, const std::string& block_name) {
	if (!valid) {
		throw GANException(207530, "Graph " + graph_name + " is not valid, try to deploy graph.");
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

		if (IsDebugModeEnabled()) {
			Json::Value info = CreateJson(
				std::map<std::string, std::string>({
					{"type", "point_emmition"},
					{"block", block_name},
				})
			);


			info["point"] = CreateJson(point);
			Json::FastWriter writer;
			WriteToBedugInfo(writer.write(info));
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


bool Graph::IsDebugModeEnabled() {
	return debug_mode;
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

std::vector<std::vector<std::string> > Graph::GetMissingEdges(const std::string& block_name) const {
	if (blocks.count(block_name) == 0) {
		throw GANException(825245, "Block with name " + block_name + " does not exist.");
	}

	return blocks.at(block_name)->GetMissingEdges();
}


std::string Graph::GetBlockType(const std::string& block_name) const {
	if (blocks.count(block_name) == 0) {
		throw GANException(234245, "Block with name " + block_name + " does not exist.");
	}

	return blocks.at(block_name)->GetBlockType();
}


void Graph::SaveGraphToFile(const std::string& file_name) const {
	std::ofstream config(file_name);
	if (!config.is_open()) {
		throw GANException(294563, "File with name " + file_name + " is not opening.");
	}

	YAML::Emitter out;

	out << YAML::BeginMap;

	out << YAML::Key << "blocks";
	out << YAML::Value << YAML::BeginSeq;
	logger << blocks.size();
	for (auto it = blocks.cbegin(); it != blocks.cend(); ++it) {
		out << *(it->second);
	}
	out << YAML::EndSeq;

	out << YAML::Key << "edges";
	out << YAML::Value;
	out <<  YAML::BeginSeq;
	for (auto it = edges.cbegin(); it != edges.cend(); ++it) {
		out << *(*it);
	}
	out << YAML::EndSeq;
	out << YAML::EndMap;
	config  << out.c_str();
	config.close();
}


void Graph::WriteToBedugInfo(const std::string& info) {
	logger << info;
	debug_info_file << info;
	debug_info_file.flush();
}



void Graph::EnableDebugMode() {
	debug_mode = true;
	logger << "create file /root/gan-system/debug_info/" + std::to_string(id);
	debug_info_file.open("/root/gan-system/debug_info/" + std::to_string(id), std::ofstream::out | std::ofstream::trunc);
}


void Graph::DisableDebugMode() {
	std::string file_name  =  std::string("/root/gan-system/debug_info/" + std::to_string(id));
	std::remove(file_name.c_str());
	debug_mode = false;
	debug_info_file.close();
}


std::vector<std::vector<std::string> > Graph::GetDebugInfo() {
	logger << std::string("Debug mode ")  + (debug_mode ? "true" : "false");
	if (!debug_mode) {
		throw GANException(124583, "Debug mode of graph " + graph_name  +  " disabled.");
	}
	std::vector<std::vector<std::string> > table;
	std::ifstream in;
	in.open("/root/gan-system/debug_info/" + std::to_string(id));
	std::string event_info;
	while (std::getline(in, event_info)) {
		table.push_back(std::vector<std::string>({event_info}));
	}
	return table;
}


Graph::~Graph() {
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		delete it->second;
	}
	if (debug_mode) {
		debug_info_file.close();

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
: DaemonBase("127.0.0.1", "8081", 0)
, graphs()
, graphs_table("GraphsTable|Id:int|GraphName:string,Valid:int")
, graphs_and_blocks_table("GraphsAndBlocks|GraphId:int,BlockId:int|")
, blocks_table("Blocks|Id:int|BlockName:string,Type:string,Cache:string")
, edges_table("Edges|Id:int|EdgeName:string,ToBlock:int")
, blocks_and_outgoing_edges_table("BlocksAndOutgoingEdges|BlockId:int,EdgeId:int|")
, blocks_params_table("BlocksParams|BlockId:int,ParamName:string|ParamValue:string")
, block_buffer()
{
	block_buffer.SetTable(&blocks_table);
	Load();
	Daemon();
}


Graph* WorkSpace::GetGraph(const std::string& graph_name) const {
	if (graphs.count(graph_name) == 0) {
		throw GANException(351852, "Graph with name " + graph_name + " does not exist.");
	}
	return graphs.at(graph_name);
}

bool WorkSpace::IsGraphExist(const std::string& graph_name) const {
	return graphs.count(graph_name) > 0;
}





WorkSpace::IgnoreChecker::IgnoreChecker(Json::Value* answer, const bool ignore)
: answer(answer)
, ignore(ignore)
{}




void WorkSpace::CheckIgnore(const IgnoreChecker& checker, const GANException& exception) const {
	if (!checker.ignore) {
		throw exception;
	} else {
		checker.answer->operator[]("status") = 1;
	}
}


/* 	QueryActionBase	*/

WorkSpace::QueryActionBase::QueryActionBase(
	const Json::Value* json_params,
       	WorkSpace* work_space,
	Json::Value* answer,
	const GANException& exception
)
: json_params(json_params)
, work_space(work_space)
, ignore(json_params->operator[]("ignore").asBool())
, answer(answer)
, exception(exception)
{}
void WorkSpace::QueryActionBase::Execute() {
	if (!Check()) {
		CheckIgnore();
	} else {
		Action(GetId());
		answer->operator[]("status") = 1;
	}

}


void WorkSpace::QueryActionBase::CheckIgnore() {
	if (ignore) {
		answer->operator[]("status") = 1;
	} else {
		throw exception;
	}
}


/*	CreateGraphQuery	*/
WorkSpace::CreateGraphQuery::CreateGraphQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer)
: QueryActionBase(
	json_params,
	work_space,
	answer,
	GANException(
		128463,
		(
		 	std::string("Graph with name ")
			+ json_params->operator[]("graph").asString()
			+  " already exists."
		)
	)
)
{}

void WorkSpace::CreateGraphQuery::Action(const int graph_id) {
	work_space->AddGaphToTables(work_space->CreateGraph(graph_id, json_params->operator[]("graph").asString(), 0));
}

bool WorkSpace::CreateGraphQuery::Check() const {
	return !work_space->IsGraphExist(json_params->operator[]("graph").asString());
}

int WorkSpace::CreateGraphQuery::GetId() const {
	return work_space->GetTable("graph")->MaxValue("Id") + 1;
}

/*	DeleteGraphQuery	*/
WorkSpace::DeleteGraphQuery::DeleteGraphQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer)
: QueryActionBase(
	json_params,
	work_space,
	answer,
	GANException(
		128463,
		(
		 	std::string("Graph with name ")
			+ json_params->operator[]("graph").asString()
			+  " does not exists."
		)
	)
)
, graph(NULL)
{
	graph = work_space->GetGraph(json_params->operator[]("graph").asString());
}


void WorkSpace::DeleteGraphQuery::Action(const int graph_id) {
	work_space->DeleteGraph(json_params->operator[]("graph").asString());
}

bool WorkSpace::DeleteGraphQuery::Check() const {
	return work_space->IsGraphExist(json_params->operator[]("graph").asString());
}

int WorkSpace::DeleteGraphQuery::GetId() const {
	return 0;
}



/*	CreateBlockQuery	*/
WorkSpace::CreateBlockQuery::CreateBlockQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer)
: QueryActionBase(
	json_params,
	work_space,
	answer,
	GANException(
		128463,
		(
		 std::string("Block with name ")
			+ json_params->operator[]("block").asString()
			+  " already exists in graph "
			+ json_params->operator[]("graph").asString()
			+ "."
		)
	)
)
, graph(NULL)
{
	graph = work_space->GetGraph(json_params->operator[]("graph").asString());
}

void WorkSpace::CreateBlockQuery::Action(const int block_id) {
	graph->AddBlockToTables(
		graph->CreateBlock(json_params->operator[]("block_type").asString(), block_id, json_params->operator[]("block").asString())
	);
	work_space->ChangeGraphsValid(json_params->operator[]("graph").asString(), 0);
}

bool WorkSpace::CreateBlockQuery::Check() const {
	return !graph->In(json_params->operator[]("block").asString());
}

int WorkSpace::CreateBlockQuery::GetId() const {
	return work_space->GetTable("block")->MaxValue("Id") + 1;
}


/*	DeleteBlockQuery	*/
WorkSpace::DeleteBlockQuery::DeleteBlockQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer)
: QueryActionBase(
	json_params,
	work_space,
	answer,
	GANException(
		128463,
		(
		 std::string("Block with name ")
			+ json_params->operator[]("block").asString()
			+  " does not exists in graph "
			+ json_params->operator[]("graph").asString()
			+ "."
		)
	)
)
, graph(NULL)
{
	graph = work_space->GetGraph(json_params->operator[]("graph").asString());
}


void WorkSpace::DeleteBlockQuery::Action(const int block_id) {
	graph->DeleteBlock(json_params->operator[]("block").asString());
	work_space->ChangeGraphsValid(json_params->operator[]("graph").asString(), 0);
}

bool WorkSpace::DeleteBlockQuery::Check() const {
	return graph->In(json_params->operator[]("block").asString());
}

int WorkSpace::DeleteBlockQuery::GetId() const {
	return 0;
}



/*	CreateEdgeQuery	*/
WorkSpace::CreateEdgeQuery::CreateEdgeQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer)
: QueryActionBase(
	json_params,
	work_space,
	answer,
	GANException(
		128463,
		(
		 std::string("Edge with name ")
			+ json_params->operator[]("block").asString()
			+  " already exists in graph "
			+ json_params->operator[]("graph").asString()
			+ " from "
		       	+ json_params->operator[]("from").asString()
			+ " to "
			+ json_params->operator[]("to").asString()
			+ "."
		)
	)
)
, graph(NULL)
{
	graph = work_space->GetGraph(json_params->operator[]("graph").asString());
}


void WorkSpace::CreateEdgeQuery::Action(const int edge_id) {
	graph->AddEdgeToTables(
		graph->CreateEdge(
			edge_id,
		       	json_params->operator[]("edge").asString(),
			json_params->operator[]("from").asString(),
			json_params->operator[]("to").asString())
	);
	work_space->ChangeGraphsValid(json_params->operator[]("graph").asString(), 0);
}

bool WorkSpace::CreateEdgeQuery::Check() const {
	return !graph->DoesEdgeExist(json_params->operator[]("to").asString(), json_params->operator[]("edge").asString());
}

int WorkSpace::CreateEdgeQuery::GetId() const {
	return work_space->GetTable("edge")->MaxValue("Id") + 1;
}

/*	DeleteEdgeQuery	*/
WorkSpace::DeleteEdgeQuery::DeleteEdgeQuery(const Json::Value* json_params, WorkSpace* work_space, Json::Value* answer)
: QueryActionBase(
	json_params,
	work_space,
	answer,
	GANException(
		128463,
		(
		 std::string("Edge with name ")
			+ json_params->operator[]("block").asString()
			+  " does not exist in graph "
			+ json_params->operator[]("graph").asString()
			+ " from "
		       	+ json_params->operator[]("from").asString()
			+ " to "
			+ json_params->operator[]("to").asString()
			+ "."
		)
	)
)
, graph(NULL)
{
	graph = work_space->GetGraph(json_params->operator[]("graph").asString());
}


void WorkSpace::DeleteEdgeQuery::Action(const int edge_id) {
	graph->DeleteEdge(
		json_params->operator[]("edge").asString(),
		json_params->operator[]("from").asString(),
		json_params->operator[]("to").asString()
	);
	work_space->ChangeGraphsValid(json_params->operator[]("graph").asString(), 0);
}

bool WorkSpace::DeleteEdgeQuery::Check() const {
	return graph->DoesEdgeExist(json_params->operator[]("to").asString(), json_params->operator[]("edge").asString());
}

int WorkSpace::DeleteEdgeQuery::GetId() const {
	return 0;
}

void WorkSpace::ChangeParam(
	const std::string& graph_name,
	const std::string& block_name,
	const std::string& param_name,
	const StringType& param_value
) {
	if (graphs.count(graph_name) == 0) {
		throw GANException(195702, "Graph with name " + graph_name  +  " does not exist.");
	}
	Graph* graph = graphs[graph_name];
	graph->AddParam(param_name, param_value, block_name);
	graph->AddParamToTable(param_name, param_value, block_name);
	ChangeGraphsValid(graph_name, 0);
}

void WorkSpace::InsertPoints(Graph* graph, const Json::Value& points) {
	for (size_t i = 0; i < points.size(); ++i) {
		Json::Value point = points[i];
		Point p(point["series"].asString(), point["value"].asDouble(), point["time"].asInt());

		logger << p;
		if (graph->IsDebugModeEnabled()) {
			Json::Value info;
			info["type"] = "insert_point";
			info["time"] = int(p.GetTime());
			if (!point["is_bad"].isNull()) {
				info["is_bad"] = point["is_bad"];
			}
			Json::FastWriter writer;
			graph->WriteToBedugInfo(writer.write(info));
		}
		if (point["block"].isNull()) {
			graph->InsertPointToAllPossibleBlocks(p);
		} else {
			graph->InsertPoint(p, point["block"].asString());
		}
	}

}



/*	QueryAction	*/

WorkSpace::QueryAction::QueryAction(const Json::Value*  json_params, WorkSpace* work_space, Json::Value* answer)
{
	std::string query_type = json_params->operator[]("type").asString();
	std::string object_type = json_params->operator[]("object").asString();

	if (query_type == "create") {
		if (object_type == "graph") {
			CreateGraphQuery(json_params, work_space, answer).Execute();
		} else if (object_type == "block") {
			CreateBlockQuery(json_params, work_space, answer).Execute();
		} else if (object_type == "edge") {
			CreateEdgeQuery(json_params, work_space, answer).Execute();
		}
	} else if (query_type == "delete") {
		if (object_type == "graph") {
			DeleteGraphQuery(
					json_params,
					work_space,
					answer).Execute();
		} else if (object_type == "block") {
			DeleteBlockQuery(json_params, work_space, answer).Execute();
		} else if (object_type == "edge") {
			DeleteEdgeQuery(json_params, work_space, answer).Execute();
		}

	} else {
		answer->operator[]("status") = 0;
		answer->operator[]("error") = "Incorrect json query.";
	}
}



Json::Value WorkSpace::JsonRespond(const Json::Value& query) {
	Json::Value answer;
	Json::StyledWriter styledWriter;
	logger << styledWriter.write(query);
	std::string query_type = query["type"].asString();

	std::string graph_name = query["graph"].asString();
	std::string objects_type = query["object"].asString();
	bool ignore = query["ignore"].asBool();
	answer["status"] = 0;
	answer["table"] = Json::Value(Json::arrayValue);
	if (
		!((query_type == "create" &&  objects_type == "graph") ||query_type == "load")
		&& graph_name != ""
		&& graphs.count(graph_name) == 0
	) {
		throw GANException(
			419294,
			"Graph with name " + graph_name  +  " does not exist."
	 	);
	}
	if (query_type == "empty_query") {
		answer["status"] = 1;
	} else if (query_type == "create" || query_type == "delete") {			// create or delete
		QueryAction(&query, this, &answer);
	} else if (query_type == "check_graph") { // check graph
		try {
			Verification(graph_name);
			answer["table"] = CreateJson(std::map<std::string, std::string>({
				{"Status","1"}
			}));

		} catch (std::exception& e) {
			answer["table"] = CreateJson(std::map<std::string, std::string>({
				{"Status","0"},
			       	{"Reason", e.what()}
			}));
		}
		answer["status"] = 1;
	} else if (query_type == "deploy") {				// deploy graph
		Verification(graph_name);
		answer["status"] = 1;
	} else if (query_type == "is_deployed") { 			// is graph deploy
		answer["head"].append("GraphDeployed");
		int is_deployed = ((graphs[graph_name]->GetGraphValid() == 1) ? 1 : 0);
		std::vector<std::map<std::string, int> >  table = {{{"GraphDeployed", is_deployed}},};
		answer["table"] = CreateJson(table);
		answer["status"] = 1;
	}  else if (query_type == "insert") {				// insert point
		InsertPoints(graphs[graph_name], query["points"]);
		answer["status"] = 1;
	} else if (query_type == "modify") {				// modify params
		if (objects_type == "param") {
			ChangeParam(
				graph_name,
				query["block"].asString(),
				query["name"].asString(),
				query["value"].asString()
			);
			answer["status"] = 1;
		} else if (objects_type == "params") {
			std::string block_name = query["block"].asString();
			if (graphs.count(graph_name) == 0) {
				throw GANException(195702, "Graph with name " + graph_name  +  " does not exist.");
			}

			Json::Value values = query["values"];

			for (size_t i = 0; i < values.size(); ++i) {
				Json::Value param = values[i];

				ChangeParam(
					graph_name,
					query["block"].asString(),
					param["name"].asString(),
					param["value"].asString()
				);
			}
			answer["status"] = 1;
		}

	} else if (query_type == "show") {				// show
		std::vector<std::string> head;
		std::vector<std::vector<std::string> > table;
		answer["status"] = 1;
		if (objects_type == "graphs") {					// show graphs
			head = {"GraphName"};
			for (auto it = graphs.begin(); it != graphs.end(); ++it) {
				table.push_back(std::vector<std::string>{it->first});
			}
		} else if (objects_type == "blocks") {				// show blocks
			head = {"Name", "Type"};
			table = graphs[graph_name]->GetBlocksNames();
		} else if (objects_type == "params") {				// show params
			std::string block_name = query["block"].asString();

			head = {"Name", "Value"};
			table = graphs[graph_name]->GetBlocksParams(block_name);
		} else if (objects_type == "edges") {				// show edges
			head =  {"From", "EdgeName", "To"};
			table = graphs[graph_name]->GetEdges();
		} else if (objects_type == "possible_edges") {			// show possible edges
			std::string block_name = query["block"].asString();

			head = {"EdgeName"};
			table = graphs[graph_name]->GetPossibleEdges(block_name);
		} else if (objects_type == "block_type") {			// show block type
			std::string block_name = query["block"].asString();

			head = {"BlockType"};
			table = std::vector<std::vector<std::string>>({{graphs[graph_name]->GetBlockType(block_name)}});
		} else if (objects_type == "types") {				// show blocks types
			head = { "Type", "Description"};
			table = Block(1,"","EmptyBlock",NULL, NULL).GetTableOfBlocksDescriptions();
		}  else if (objects_type == "cycle") {				// show cycle
			table = GetCycle(graph_name);
			head = {"BlockName"};
		} else if (objects_type == "missing_edges") {
		       	std::string block_name = query["block"].asString();
			table = graphs[graph_name]->GetMissingEdges(block_name);
			head = {"EdgeName"};

		} else {
			answer["status"] = 0;
			answer["error"] = "Incorrect json query.";
		}
		if (answer["status"].asInt() == 1) {
			answer["head"] = CreateJson(head);
			answer["table"] = CreateJsonTable(table, head);
			answer["status"] = 1;
		}

	} else if (query_type == "save")  {				// save graph
		std::string file_name = query["file"].asString();

		graphs[graph_name]->SaveGraphToFile(file_name);
		answer["status"] = 1;
	} else if (query_type == "convert") {			//convert
		std::string file_name = query["file"].asString();

		answer["head"].append("Query");
		answer["table"] = CreateJson(ConvertConfigToQueries(file_name));
		answer["status"] = 1;
	} else if (query_type == "load") {			// load graph
		bool replace = query["replace"].asBool();
		std::string file_name = query["file"].asString();
		std::vector<std::string> first_queries;
		if (replace) {
			Json::Value delete_query = CreateJson(
				std::map<std::string, std::string>({
					{"type", "delete"},
					{"object", "graph"},
					{"graph", graph_name}
				})
			);
			JsonRespond(delete_query);
		}
		if (graphs.count(graph_name) != 0) {
			IgnoreChecker checker(&answer, ignore);
			CheckIgnore(
				checker,
				GANException(
					128463,
					(
						std::string("Graph with name ")
						+ graph_name
						+  " already exists."
					)

				)
			);
		} else {

			Json::Value create_query = CreateJson(
				std::map<std::string, std::string>({
					{"type", "create"},
					{"object", "graph"},
					{"graph", graph_name}
				})
			);
			JsonRespond(create_query);
			answer = LoadGraphFromFile(
				file_name,
				graph_name
			);
		}
	} else if (query_type  == "debug") {  				// debug mode
		if (objects_type == "mode") {
			if (query["enable"].asBool()) {
				graphs[graph_name]->EnableDebugMode();
			} else {
				graphs[graph_name]->DisableDebugMode();
			}
			answer["status"] = 1;
		} else if (objects_type == "info") {
			std::vector<std::string> head = std::vector<std::string>({"Event"});
			answer["head"].append("Event");
			answer["table"] = CreateJsonTable(graphs[graph_name]->GetDebugInfo(), head);
			answer["status"] = 1;
		}
	} else if (query_type == "fit_graph") {
		IteraiveGraphBuilder builder(
			this,
			query["graph"].asString(),
			query["build_graph_method"].asString(),
			query["ml_method"].asString(),
			query["train_data"],
			query["iteration_count"].asInt()
		);
		builder.Fit();
		answer["status"] = 1;
	}  else if (query_type == "help") { 				// help
		std::vector<std::string> head = std::vector<std::string>({"Help"});
		std::vector<std::vector<std::string> > table;

		answer["head"] = CreateJson(head);
		std::string help = std::string("Queries:\n\tCreation/Deletion of objects:\n")
			+ "\t\tcreate|delete [ignore] graph  <graph_name>\n"
			+ "\t\tcreate|delete [ignore] block  <block_name>[:<block_type>] in graph <graph_name>\n"
			+ "\t\tcreate|delete [ignore] edge  <edge_name> in graph <graph_name> from <from_vertex_name> to <to_vertex_name>\n"
			+ "\tWork with graph:\n"
			+ "\t\tdeploy graph <graph_name> -- verify the absence of cycles and availability of all incoming edges in blocks\n"
			+ "\t\tis graph <graph_name> deployed -- return result of last graphs verification\n"
			+ "\t\tinsert point '<series_name>':<time>:<double_value> into [block <block_name> of] graph <graph_name> -- insert point to all blocks without incoming edges or to current block\n"
			+ "\t\tmodify param <param_name> to <param_value> in block <block_name> of graph <graph_name>\n"
			+ "\tShow Graph Structure:\n"
			+ "\t\tshow graphs\n"
			+ "\t\tshow blocks types\n"
			+ "\t\tshow blocks|edges|cycle of graph <graph_name>\n"
			+ "\t\tshow params|possible edges of block <block_name> of graph <graph_name>\n"
			+ "\t\tshow block type of block <block_name> of graph <graph_name>\n"
			+ "\t\thelp\n"
			+ "\tOperations with graph:\n"
			+ "\t\tsave graph <graph_name> to file <file_name>\n"
			+ "\t\tconvert config <file_name> to queries\n"
			+ "\t\tload [replace|ignore] graph <graph_name> from file <file_name> -- in this query file with config is converted to sequence of requests and executed step by step\n"
			+ "\t\t[en|dis]able debug mode on graph <graph_name>\n"
			+ "\t\tget debug info of graph <graph_name>"
			+ "Blocks:\n" + Block(1,"","EmptyBlock",NULL, NULL).GetAllBlocksDescriptions();

		table =  std::vector<std::vector<std::string> >{{help}};
		answer["table"] = CreateJsonTable(table, head);

		answer["status"] = 1;
	} else {
		answer["status"] = 0;
		answer["error"] = "Incorrect json query.";
	}
	return answer;

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


void WorkSpace::DeleteGraph(const std::string& graph_name) {
	int graph_id = graphs[graph_name]->GetGraphId();
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

std::vector<std::vector<std::string> > WorkSpace::GetCycle(const std::string& graph_name) const {
	return graphs.at(graph_name)->GetCycle();
}


void WorkSpace::Verification(const std::string& graph_name) {
	graphs[graph_name]->Verification();
	ChangeGraphsValid(graph_name, 1);
}

std::vector<std::vector<std::string> > WorkSpace::ConvertConfigToQueries(
	const std::string& file_name,
	const std::string& graph_name,
	const std::vector<std::string>& first_queries
) const {
	std::ifstream fin(file_name);
	if (!fin.is_open()) {
		throw GANException(243563, "File with name " + file_name + " is not opening.");
	}
	YAML::Parser parser(fin);
	YAML::Node graph_config;
	parser.GetNextDocument(graph_config);

	const YAML::Node& blocks_node = graph_config["blocks"];
 	const YAML::Node& edges = graph_config["edges"];

	std::vector<std::vector<std::string> > queries;
	for (size_t i = 0; i < first_queries.size(); ++i) {
		queries.push_back({first_queries[i]});
	}

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




std::vector<Json::Value> WorkSpace::ConvertConfigToJsonQueries(
	const std::string& file_name,
	const std::string& graph_name
) const {
	std::ifstream fin(file_name);
	if (!fin.is_open()) {
		throw GANException(243563, "File with name " + file_name + " is not opening.");
	}
	YAML::Parser parser(fin);
	YAML::Node graph_config;
	parser.GetNextDocument(graph_config);

	const YAML::Node& blocks_node = graph_config["blocks"];
 	const YAML::Node& edges = graph_config["edges"];

	std::vector<Json::Value> queries;

	for (size_t i = 0; i < blocks_node.size(); ++i) {
		std::string block_name;
		blocks_node[i]["name"] >> block_name;
		std::string block_type;
		blocks_node[i]["type"] >> block_type;
		Json::Value query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "block"},
				{"block", block_name},
				{"block_type", block_type},
				{"graph", graph_name}
			})
		);
		queries.push_back(query);
		std::map<std::string, std::string> params;
		blocks_node[i]["params"] >> params;

		for (auto it = params.begin(); it != params.end(); ++it) {
			Json::Value param_query = CreateJson(
				std::map<std::string, std::string>({
					{"type", "modify"},
					{"object", "param"},
					{"name", it->first},
					{"value", it->second},
					{"block", block_name},
					{"graph", graph_name}
				})
			);

			queries.push_back(param_query);
		}
	}

	for (size_t i = 0; i < edges.size(); ++i) {
		std::string edge_name;
		edges[i]["name"] >> edge_name;
		std::string from;
		edges[i]["from"] >> from;
		std::string to;
		edges[i]["to"] >> to;

		Json::Value query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "edge"},
				{"edge", edge_name},
				{"graph", graph_name},
				{"from", from},
				{"to", to}
			})
		);

		queries.push_back(query);
	}

	return queries;

}


Json::Value WorkSpace::LoadGraphFromFile(
	const std::string& file_name,
	const std::string& graph_name
)  {
	Json::Value answer;
	answer["head"].append("Query");
	Json::Value table;
	std::vector<Json::Value> queries = ConvertConfigToJsonQueries(file_name, graph_name);
	Json::Value deploy_query = CreateJson(
		std::map<std::string, std::string>({
			{"type", "deploy"},
			{"object", "graph"},
			{"graph", graph_name},
		})
	);

	queries.push_back(deploy_query);

	try {
		for (size_t i = 0; i < queries.size(); ++i) {
			Json::FastWriter fastWriter;
			table.append(
				CreateJson(
					std::vector<std::map<std::string, std::string> >({
						{{"Query", fastWriter.write(queries[i])}}
					})
				)
			);
			JsonRespond(queries[i]);
		}

	} catch (std::exception& e) {
		answer["error"] = e.what();
		answer["table"] = table;
		answer["status"] = 0;
		return answer;
	}
	answer["table"] = table;
	answer["status"] = 1;
	return answer;
}


Table* WorkSpace::GetTable(const std::string& object_type) {
	if (object_type == "graph") {
		return &graphs_table;
	} else if (object_type == "block") {
		return &blocks_table;
	} else if (object_type == "edge") {
		return &edges_table;
	}
	throw GANException(385341, "Incorrent object type " + object_type + ".");

}


void WorkSpace::CreateBlockThroughRespond(
	const std::string& graph_name,
	const std::string& block_name,
	const std::string& block_type
) {
	JsonRespond(
		CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "block"},
				{"block", block_name},
				{"block_type", block_type},
				{"graph", graph_name},
			})
		)

	);

}

void WorkSpace::DeleteBlockThroughRespond(const std::string& graph_name, const std::string& block_name) {
	JsonRespond(
		CreateJson(
			std::map<std::string, std::string>({
				{"type", "delete"},
				{"object", "block"},
				{"block", block_name},
				{"graph", graph_name}
			})
		)
	);

}



void WorkSpace::CreateEdgeThroughRespond(const std::string& graph_name, const std::string& from, const std::string& to, const std::string& edge_name) {
	JsonRespond(
		CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "edge"},
				{"edge", edge_name},
				{"graph", graph_name},
				{"from", from},
				{"to", to}
			})
		)
	);

}


void WorkSpace::ModifyParamsThroughRespond(const std::string& graph_name, const std::string& block_name, const std::string& param_name, const StringType& value) {
	JsonRespond(
		CreateJson(
			std::map<std::string, std::string>({
				{"type", "modify"},
				{"object", "param"},
				{"name", param_name},
				{"value", value},
				{"block", block_name},
				{"graph", graph_name}
			})
		)
	);


}


void WorkSpace::DeployGraphThroughRespond(const std::string& graph_name) {
	JsonRespond(
		CreateJson(
			std::map<std::string, std::string>({
				{"type", "deploy"},
				{"object", "graph"},
				{"graph", graph_name},
			})
		)
	);
}


WorkSpace::~WorkSpace() {
	for (auto it = graphs.begin(); it != graphs.end(); ++it) {
		delete it->second;
	}
}


/*	IterativeInitialGraphGeneratorBase	*/

Graph* IterativeInitialGraphGeneratorBase::BuildGraph() {
	return NULL;
}


IterativeInitialGraphGeneratorBase::IterativeInitialGraphGeneratorBase(
	WorkSpace* work_space,
	const std::string& graph_name
)
: work_space(work_space)
, graph()
, graph_name(graph_name)
{}


IterativeInitialGraphGeneratorBase::~IterativeInitialGraphGeneratorBase() {}


/* TakeInitialGraph */

Graph* TakeInitialGraph::BuildGraph() {
	return work_space->GetGraph(graph_name);
}


TakeInitialGraph::TakeInitialGraph(WorkSpace* work_space, const std::string& graph_name)
: IterativeInitialGraphGeneratorBase(work_space, graph_name)
{}


TakeInitialGraph::~TakeInitialGraph() {}

/*	IterativeInitialGraphGenerator	*/
IterativeInitialGraphGenerator::IterativeInitialGraphGenerator(
	WorkSpace* work_space,
	const std::string& graph_name,
	const std::string& build_graph_method
)
: graph_generator()
{
	if (build_graph_method == "take_initial") {
		graph_generator = new TakeInitialGraph(work_space, graph_name);
	}
}


Graph* IterativeInitialGraphGenerator::BuildGraph() {
	return graph_generator->BuildGraph();
}


IterativeInitialGraphGenerator::~IterativeInitialGraphGenerator() {
	delete graph_generator;
}

/*	IterativeLearningBase	*/

IterativeLearningBase::IterativeLearningBase(WorkSpace* work_space, const std::string& graph_name)
: work_space(work_space)
, graph(NULL)
, graph_name(graph_name)
, head()
, table()
, times_types()
{
	graph = work_space->GetGraph(graph_name);
	head = GetHorizontalHead();
}


std::vector<std::string> IterativeLearningBase::GetHorizontalHead() const {
	std::vector<std::vector<std::string> > blocks_info = graph->GetBlocksNames();
	std::vector<std::string> blocks_names(blocks_info.size());

	for (size_t i = 0; i < blocks_info.size(); ++i) {
		blocks_names[i] = blocks_info[i][0];
	}
	return blocks_names;
}




void IterativeLearningBase::CreateTrainTable(const Json::Value& points) {
	work_space->DeployGraphThroughRespond(graph_name);
	if (graph->IsDebugModeEnabled()) {
		graph->DisableDebugMode();
	}
	graph->EnableDebugMode();
	work_space->InsertPoints(graph, points);

	std::vector<std::vector<std::string> > debug_info = graph->GetDebugInfo();

	std::unordered_map<std::string, double> last_emmited_points;

	for (size_t i = 0; i < head.size(); ++i) {
		last_emmited_points[head[i]] = 0;
	}

	long long int current_time = 0;
	int is_bad = 0;

	for (size_t i = 0; i < debug_info.size(); ++i) {
		Json::Value event;
		Json::Reader reader;
		bool is_parsing_successful = reader.parse(debug_info[i][0], event);
		if (!is_parsing_successful) {
			throw GANException(256285, "Incorrect answer from server: " + debug_info[i][0] + ".");
		}

		bool is_end_of_row = (i + 1 == debug_info.size() ? true : false);


		if (event["type"] == "insert_point" && current_time < event["time"].asInt()) {
			current_time = event["time"].asInt();
			logger << "change cur time to " + std::to_string(current_time);

			if (i != 0) {
				is_end_of_row = true;
			}
		} else if (event["type"] == "point_emmition" || event["type"] == "insert_point") {
			last_emmited_points[event["block"].asString()] = event["point"]["value"].asDouble();
		}


		if (is_end_of_row) {
			table.push_back(std::vector<double>(head.size()));
			logger << "is_bad" + std::to_string(is_bad);
			times_types.push_back(is_bad);
			is_bad = 0;
			logger << "cur time:";
			logger << current_time;
			for (size_t i = 0; i < head.size(); ++i) {
				table[table.size() - 1][i]  = last_emmited_points[head[i]];
				logger << std::to_string(table.size() - 1) + ":" + std::to_string(i) + " ==> " + std::to_string(last_emmited_points[head[i]]);
			}

		}

		if (!event["is_bad"].isNull() && event["is_bad"].asInt() == 1) {
			logger << event["time"].asString();
			logger << "is_bad chaned to 1";
			is_bad = 1;
		}

	}
}

IterativeLearningBase::~IterativeLearningBase() {}


/*	SvnIterativeLearning	*/

SvnIterativeLearning::SvnIterativeLearning(WorkSpace* work_space, const std::string& graph_name)
: IterativeLearningBase(work_space, graph_name)
{}


void SvnIterativeLearning::FitGraph() {
	Json::FastWriter writer;
	std::string s_table = writer.write(CreateJson(table));
	std::string svm_query = (
		"python /root/gan-system/svm.py "
		+ s_table.substr(0, s_table.size() - 1)
		+ " "
		+ writer.write(CreateJson(times_types))
	);
	ExecuteHandler ex(svm_query);
	logger << svm_query;

	std::string json_coeffs;
	ex >> json_coeffs;


	std::string s_h = "";
	for (size_t i = 0; i < head.size(); ++i) {
		s_h += head[i] + ", ";
	}
	logger << s_h;
	logger << "svm coeffs: " + json_coeffs;

	Json::Reader reader;
	Json::Value svm_coeffs;
	reader.parse(json_coeffs, svm_coeffs);

	if (svm_coeffs["blocks_coeffs"].size() != head.size()) {
		throw GANException(214551, "Incorrect size of svm coeffs table " + std::to_string(svm_coeffs["blocks_coeffs"].size()) + "/" + std::to_string(head.size()));
	}


	if (svm_coeffs["blocks_coeffs"].size() != 0) {
		Json::Value blocks_coeffs = svm_coeffs["blocks_coeffs"];
		std::unordered_map<std::string, double> coeffs;

		double max_coeff = blocks_coeffs[0].asDouble();
		for (size_t i = 0; i < blocks_coeffs.size(); ++i) {
			double coeff = blocks_coeffs[i].asDouble();
			if (std::fabs(max_coeff) < std::fabs(coeff)) {
				max_coeff = coeff;
			}
			coeffs[head[i]] = coeff;
		}

		double  shift_coeff = - svm_coeffs["shift_coeff"].asDouble() / std::fabs(max_coeff);
		int deleted_blocks_count = -1;

		while(deleted_blocks_count != 0) {
			std::vector<std::string> blocks_to_erase;
			deleted_blocks_count = 0;
			for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
				logger << it->first + " " + std::to_string(it->second);
			}
			for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
				std::string block_name = it->first;
				double coeff = it->second;
				logger << block_name;
				if (
					std::fabs(coeff) < std::fabs(max_coeff) / 1000.
					&& !graph->DoesOutgoingEdgesExist(block_name)
				) {
					work_space->DeleteBlockThroughRespond(graph_name, block_name);
					logger <<
						"Delete block "
						+ block_name
						+ " with coeff "
						+ std::to_string(coeff)
						+ "(max coeff: "
						+ std::to_string(max_coeff)
						+ ") "
						+ std::to_string(std::fabs(coeff))
						+ " "
						+ std::to_string(std::fabs(max_coeff) / 1000.);
					blocks_to_erase.push_back(block_name);
					++deleted_blocks_count;
				} else  {
					logger <<
						"Dont delete block "
						+ block_name
						+ " with coeff "
						+ std::to_string(coeff)
						+ "(max coeff: "
						+ std::to_string(max_coeff)
						+ ")"
						+ std::to_string(std::fabs(coeff))
						+ " "
						+ std::to_string(std::fabs(max_coeff) / 1000.);

				}
			}

			for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
				logger << it->first + " " + std::to_string(it->second);
			}


			for (size_t i = 0; i < blocks_to_erase.size(); ++i) {
				logger << "delete block " + blocks_to_erase[i] + " from map.";
				coeffs.erase(blocks_to_erase[i]);
			}
			for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
				logger << it->first + " " + std::to_string(it->second);
			}


		}
			for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
				logger << it->first + " " + std::to_string(it->second);
			}


		std::string svm_block_name = "svm_sum" + std::to_string(coeffs.size());

		logger  << "FIT; FIT: Rebuild graph";

		logger  << "FIT; create block " + svm_block_name;
		work_space->CreateBlockThroughRespond(graph_name, svm_block_name, "Sum" + std::to_string(coeffs.size()));

		int block_index = 1;

		for (auto it = coeffs.begin(); it != coeffs.end(); ++it) {
			std::string block_name = it->first;
			double coeff = roundf((it->second) / std::fabs(max_coeff) * 1000) / 1000;
			std::string to_sum_block_name = block_name;
			if (coeff != 1) {
				std::string scale_block_name = "scale_block_" + block_name;


				logger  << "FIT; create block " + scale_block_name;
				work_space->CreateBlockThroughRespond(graph_name, scale_block_name, "Scale");

				logger  << "FIT; modify params of block " + scale_block_name;
				work_space->ModifyParamsThroughRespond(graph_name, scale_block_name, "value", std::to_string(coeff));


				logger  << "FIT; create edge to_scale from bolck " + block_name;
				work_space->CreateEdgeThroughRespond(graph_name, block_name, scale_block_name, "to_scale");

				to_sum_block_name = scale_block_name;
			}

			logger  << "FIT; create edge from block " + to_sum_block_name;
			work_space->CreateEdgeThroughRespond(graph_name, to_sum_block_name, svm_block_name, "arg" + std::to_string(block_index++));
		}

		std::string svm_threshold = "threshold_svm_linear_comb";

		logger  << "FIT; create block " + svm_threshold;
		work_space->CreateBlockThroughRespond(graph_name, svm_threshold, "Threshold");

		logger  << "FIT; modify params of block " +  svm_threshold;
		work_space->ModifyParamsThroughRespond(graph_name, svm_threshold, "bound", std::to_string(shift_coeff));

		logger  << "FIT; create edge value from block " + svm_block_name;
		work_space->CreateEdgeThroughRespond(graph_name, svm_block_name,  svm_threshold, "value");

		std::string alert_block_name = "alert";

		work_space->CreateBlockThroughRespond(graph_name, alert_block_name, "PrintToLogs");
		work_space->CreateEdgeThroughRespond(graph_name, svm_threshold, alert_block_name, "to_print");


		std::string send_email_block = "svn_send_email";
		work_space->CreateBlockThroughRespond(graph_name, send_email_block, "SendEmail");
		work_space->CreateEdgeThroughRespond(graph_name, svm_threshold, send_email_block, "to_send");
		work_space->ModifyParamsThroughRespond(graph_name, send_email_block, "email", std::string("ilyshnikova@yandex.ru"));

		work_space->DeployGraphThroughRespond(graph_name);

		graph->DisableDebugMode();
	}
}

 SvnIterativeLearning::~SvnIterativeLearning() {}


/*	IterativeLearning	*/

IterativeLearning::IterativeLearning(WorkSpace* work_space, const std::string& graph_name, const std::string& ml_method_name)
: learning_base()
{
	if (ml_method_name == "SVM") {
		learning_base = new SvnIterativeLearning(work_space, graph_name);
	} else {
		throw GANException(245411, "Incorrect ML methods name.");
	}
}

void IterativeLearning::Fit(const Json::Value& points) {
	learning_base->CreateTrainTable(points);
	learning_base->FitGraph();
}

IterativeLearning::~IterativeLearning() {
	delete learning_base;
}


/*	IteraiveGraphBuilder	*/



IteraiveGraphBuilder::IteraiveGraphBuilder(
	WorkSpace* work_space,
	const std::string& graph_name,
	const std::string& build_graph_method,
	const std::string& ml_method_name,
	const Json::Value& json_train_data,
	const size_t iteration_count
)
: graph_generator(new IterativeInitialGraphGenerator(work_space, graph_name, build_graph_method))
, iterative_learning_graph(new IterativeLearning(work_space, graph_name, ml_method_name))
, train_data(json_train_data)
, iteration_count(iteration_count)
{}

void IteraiveGraphBuilder::Fit() {
	graph_generator->BuildGraph();
	for (size_t i = 0; i < iteration_count; ++i) {
		iterative_learning_graph->Fit(train_data);
	}
}


IteraiveGraphBuilder::~IteraiveGraphBuilder(){
	delete iterative_learning_graph;
	delete graph_generator;
}

