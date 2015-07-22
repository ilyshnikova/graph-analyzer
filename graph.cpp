#include <boost/regex.hpp>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

#include "graph.h"

/*WorkSpaceExceptions*/

WorkSpace::WorkSpaceExceptions::WorkSpaceExceptions(const std::string& reason)
: id(927462)
, reason(reason)
{}


const char * WorkSpace::WorkSpaceExceptions::what() const throw() {
	return ("ERROR " + std::to_string(id) + " : " + reason).c_str();
}


WorkSpace::WorkSpaceExceptions::~WorkSpaceExceptions() throw()
{}


/*   WorkSpace    */

void WorkSpace::Recover() {
	for (Table::rows it = graphs_table.Select("1"); it != graphs_table.SelectEnd(); ++it) {
		CreateGraph(int(it["Id"]), std::string(it["GraphName"]));
	}
}


WorkSpace::WorkSpace()
: graphs()
, graphs_table("GraphsTable|Id:int|GraphName:string")
, graphs_and_blocks_table("GraphsAndBlocks|DraphId:int,GraphId:int|")
, DaemonBase("127.0.0.1", "8081", 0)
{
	Recover();
	Daemon();
}


std::string WorkSpace::Respond(const std::string& query)  {
	std::cout << "query = " << query << "\n";
	boost::smatch match;
	if (boost::regex_match(query, match, boost::regex("\\s*create\\s+graph\\s+(\\w+)"))) {
		int graph_id = graphs_table.MaxValue("Id") + 1;
		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			CreateGraph(graph_id, graph_name);
			return "Ok";
		} else {
			throw WorkSpaceExceptions("Graph with same name yet exists\n");
		}

	} else if (boost::regex_match(query, match, boost::regex("\\s*create\\s+graph\\s+if\\s+not\\s+exists\\s+(\\w+)"))) {
		int graph_id = graphs_table.MaxValue("Id") + 1;
		std::string graph_name = match[1];

		if (graphs.count(graph_name) == 0) {
			CreateGraph(graph_id, graph_name);
		}
		return "Ok";
	} else if (boost::regex_match(query, match, boost::regex("\\s*delete\\s+graph\\s+(\\w+)"))) {
		std::string graph_name = match[1];

		if (graphs.count(graph_name) != 0) {
			int graph_id = graphs.at(graph_name)->GetId();
			DeleteGraph(graph_id, graph_name);
			return "Ok";
		} else {
			throw WorkSpaceExceptions("Graph with same name does not exist\n");
		}
	}
	throw WorkSpaceExceptions("Incorrect query\n");
}



void WorkSpace::CreateGraph(const int graph_id, const std::string& graph_name) {
	graphs[graph_name] = new Graph(graph_id, graph_name);
	graphs_table.Insert(graph_id, graph_name).Execute();
}

void WorkSpace::DeleteGraph(const int graph_id, const std::string& graph_name) {
	delete graphs[graph_name];
	graphs.erase(graph_name);
	graphs_table.Delete("Id = " + std::to_string(graph_id));
}



WorkSpace::~WorkSpace() {
	for (auto it = graphs.begin(); it != graphs.end(); ++it) {
		delete it->second;
	}
}

/*     Graph     */


Graph::Graph(const int id, const std::string& graph_name)
: id(id)
, graph_name(graph_name)
, graph()
, edges()
//, blocks_table(graph_name + "Blocks|Id:int|BlockName:string,Type:string,State:string")
//, edges_table(graph_name + "Edges|Id:int|EdgeName:string,ToBlock:int")
//, outgoing_edges_and_blocks_table(graph_name + "OutgoingEdgesAndBlocks|BlockId:int,EdgeId")
{}

int Graph::GetId() const {
	return id;
}





