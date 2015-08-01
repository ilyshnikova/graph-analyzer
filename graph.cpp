#include <boost/regex.hpp>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include "gan-exception.h"
#include "graph.h"
#include "logger.h"

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

BlockBase::BlockBase(const std::unordered_set<std::string>& incoming_edges_names)
: incoming_edges_names(incoming_edges_names)
{}

/*   TestBlock   */

TestBlock::TestBlock()
: BlockBase({"test_edge"})
{}


/*   EmptyTestBlock    */

EmptyTestBlock::EmptyTestBlock()
: BlockBase({})
{}


/*	Block	*/


Block::Block(
	const int id,
	const std::string& block_name,
	const std::string& block_type
)
: block()
, id(id)
, block_name(block_name)
, data()
, outgoing_edges()
, incoming_edges()
, block_type(block_type)
{
	if (block_type == "TestBlock") {
		block = new TestBlock();
	} else if (block_type == "EmptyTestBlock") {
		block = new EmptyTestBlock();
	} else {
		throw GANException(649264, "Type " + block_type + " is incorret block type.");
	}
}

std::string Block::GetBlockType() const {
	return block_type;
}

int Block::GetBlockId() const {
	return id;
}

std::string Block::GetBlockName() const {
	return block_name;
}

bool Block::Verification() const {
	for (auto it = block->incoming_edges_names.begin(); it != block->incoming_edges_names.end(); ++it) {
		if (incoming_edges.count(*it) == 0) {
			return false;
		}
	}
	return true;
}

bool Block::DoesEdgeExist(std::string& incoming_edge_name) {
	if (block->incoming_edges_names.count(incoming_edge_name) == 0) {
		throw GANException(519720, "Edge with name " + incoming_edge_name  + " can't incoming to this block.");
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
		throw GANException(519720, "Edge with name " + incoming_edge_name  + " can't incoming to this block.");
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
		throw GANException(238536, "Edge with name " + edge_name  +  " can't enter to this block.");
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
	Table* blocks_and_outgoing_edges_table
)
: id(id)
, graph_name(graph_name)
, blocks()
, edges()
, graphs_and_blocks_table(graphs_and_blocks_table)
, blocks_table(blocks_table)
, edges_table(edges_table)
, blocks_and_outgoing_edges_table(blocks_and_outgoing_edges_table)

{
Load();
}


std::string Graph::GetGraphName() const {
	return graph_name;
}



void Graph::Load() {
	for (
		auto it = graphs_and_blocks_table->Select("GraphId = " + std::to_string(id));
		it != graphs_and_blocks_table->SelectEnd();
		++it
	){
		auto block_info = blocks_table->Select("Id = " + std::to_string(it["BlockId"]));
		CreateBlock(block_info["Type"], block_info["Id"], block_info["BlockName"]);

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
		Block* block = new Block(block_id, block_name, block_type);
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
		edge = block_to->GetIncomingEdge(edge_name);
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
			logger << use_now[i];
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
		if (!it->second->Verification()) {
			throw GANException(529716, "Block " + it->first + " does not has all incoming edges.");
		}
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

Graph::~Graph() {
	for (auto it = blocks.begin(); it != blocks.end(); ++it) {
		delete it->second;
	}
}


/*   WorkSpace    */

void WorkSpace::Load() {
	for (Table::rows it = graphs_table.Select("1"); it != graphs_table.SelectEnd(); ++it) {
		CreateGraph(int(it["Id"]), std::string(it["GraphName"]));
	}
}

WorkSpace::WorkSpace()
: graphs()
, graphs_table("GraphsTable|Id:int|GraphName:string,Valid:int")
, graphs_and_blocks_table("GraphsAndBlocks|GraphId:int,BlockId:int|")
, blocks_table("Blocks|Id:int|BlockName:string,Type:string,State:string")
, edges_table("Edges|Id:int|EdgeName:string,ToBlock:int")
, blocks_and_outgoing_edges_table("BlocksAndOutgoingEdges|BlockId:int,EdgeId:int|")
, DaemonBase("127.0.0.1", "8081", 0)
{
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

		AddGaphToTables(CreateGraph(graph_id, graph_name));
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
			AddGaphToTables(CreateGraph(graph_id, graph_name));
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
			throw GANException(428352, "Block with name" + block_name  + " already exists in this graph.");
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

	} else {
		throw GANException(529352, "Incorrect query");
	}
	return "Ok";
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

Graph* WorkSpace::CreateGraph(const int graph_id, const std::string& graph_name) {
	Graph* graph = new Graph(
		graph_id,
		graph_name,
		&graphs_and_blocks_table,
		&blocks_table,
		&edges_table,
		&blocks_and_outgoing_edges_table

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

void WorkSpace::ChangeGraphsValid(const std::string& graph_name, const int valid) {
	int graph_id = graphs[graph_name]->GetGraphId();
	logger << "Insert into tablse Graphs Id:"
		+ std::to_string(graph_id)
		+ " GraphName:"
		+ graph_name
		+ " Valid:"
		+ std::to_string(valid);
	graphs_table.Insert(graph_id, graph_name, valid);
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
