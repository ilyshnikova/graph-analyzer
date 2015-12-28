#include <string>
#include <iostream>
#include <boost/regex.hpp>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <json/json.h>
#include "gan-exception.h"
#include "daemons.h"
#include "logger.h"
#include "graph.h"
#include "mysql.h"

/*    LibSocket     */

LibSocket::LibSocket(const std::string& ip, const std::string& port)
: ip(ip)
, port(port)
, host_info()
, host_info_list()
{}



void LibSocket::Prepare() {
	memset(&host_info, 0, sizeof host_info);

	host_info.ai_family = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;
}


int LibSocket::Start() {
	int status = getaddrinfo(ip.c_str(), port.c_str(), &host_info, &host_info_list);
	if (status != 0) {
		throw GANException(324832, "getaddrinfo failed: " + std::to_string(status));
	}


	int socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
	if (socketfd == -1) {
		throw GANException(235153 , "Creating socket failed");
	}


	return socketfd;
}


std::string LibSocket::GetMessage(const size_t RECV_PART, struct timeval tv, const int socketfd) const {
	std::string got_message = "";
	char * incoming_data_buffer = new char[RECV_PART + 1];

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socketfd, &fds);

	while (1) {
		memset(incoming_data_buffer, 0, RECV_PART + 1);

		int select_result = select(socketfd + 1, &fds, NULL, NULL, &tv);
		if (select_result < 0) {
			throw GANException(954625, "select failed");
		}

		if (!select_result) {
			break;
		}

		ssize_t bytes_recieved = recv(socketfd, incoming_data_buffer, RECV_PART, 0);
		if (bytes_recieved == 0) {
		       	break;
		}

		got_message += std::string(incoming_data_buffer);
	}
	delete [] incoming_data_buffer;
	return got_message;
}


void LibSocket::SendMessage(const int socketfd, const std::string& send_message) const {
	send(socketfd, send_message.c_str(), send_message.size(), 0);

}

/*    BaseClient      */

int BaseClient::Connect()  {
	int socketfd = Start();

	int status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1) {
		throw GANException(247528, "connect failed");
	}

	return socketfd;
}

std::string BaseClient::CreateJsonForDaemon(const std::string& query) const {
	return query;
}

std::string BaseClient::CreateAnswerFromJson(const std::string& json) {
	return json;
}




bool BaseClient::GetQuery(std::string* query)  {
	std::cout << "gan> ";
	if (!std::getline(std::cin, *query)) {
		std::cout << "\n";
		return false;
	}
	return !std::cout.eof();
}


bool BaseClient::Conversation(std::string* answer, const size_t RECV_PART, struct timeval tv) {
	std::string query;
	if (!GetQuery(&query)) {
		return false;
	}
	int socketfd;
	try {
		socketfd = Connect();
		query = CreateJsonForDaemon(query);
		SendMessage(socketfd, query);
		shutdown(socketfd, 1);

		std::string got_message = GetMessage(RECV_PART, tv, socketfd);
		*answer = CreateAnswerFromJson(got_message);

	} catch (std::exception& e) {
		*answer =  e.what();
	}

	freeaddrinfo(host_info_list);
	close(socketfd);
	return true;
}

void BaseClient::Callback(const std::string& answer) const {
	std::cout << answer << "\n";
}

void BaseClient::Process() {
	const size_t RECV_PART = 10000;

	double timeout = 900;
	struct timeval tv;
	tv.tv_sec = int(timeout);
	tv.tv_usec = int((timeout - tv.tv_sec) * 1e6);

	Prepare();

	while (1) {
		std::string answer;
		if (!Conversation(&answer, RECV_PART, tv)) {
			return;
		}
		Callback(answer);
	}

}


BaseClient::BaseClient(const std::string& ip, const std::string& port)
: LibSocket(ip, port)
{}



/*	NginxClient	*/

void NginxClient::AddQuery(const std::string& new_query) {
	query = new_query;
	is_used = false;
}

void NginxClient::Callback(const std::string& answer) const {}

bool NginxClient::GetQuery(std::string* new_query) {
	if (is_used) {
		return false;
	}
	*new_query =  query;
	is_used = true;
	return true;
}

NginxClient::NginxClient(const std::string& ip, const std::string& port)
: BaseClient(ip, port)
, query()
, is_used(false)
{}




/*	TerminalClient	*/


std::string TerminalClient::CreateAnswerFromJson(const std::string& json) {
	Json::Value answer;
	Json::Reader reader;
	bool is_parsing_successful = reader.parse(json, answer);
	if (!is_parsing_successful) {
		throw GANException(256285, "Incorrect answer from server: " + json + ".");
	}

	std::string string_ans = "Ok";
	if (answer["status"].asInt() == 0) {
		string_ans = std::string("Not Ok");
	}
	if (!answer["head"].isNull()) {
		string_ans += "\n";
		Json::Value head = answer["head"];
		for (size_t i = 0; i < head.size(); ++i) {
			string_ans += head[i].asString() + "\t";
		}
	}
	if (!answer["table"].isNull()) {
		Json::Value table = answer["table"];
		Json::Value head = answer["head"];
		for (size_t i = 0; i < table.size(); ++i) {
			string_ans += "\n";
			Json::Value row = table[i];
			for (size_t j = 0; j < head.size(); ++j) {
				string_ans += row[head[j].asString()].asString() + "\t";
			}
		}
	}
	if (!answer["error"].isNull()) {
		string_ans += std::string("\n") + answer["error"].asString();
	}
	return string_ans + "\0";

}

std::string TerminalClient::CreateJsonForDaemon(const std::string& query) const	{
	Json::Value json_query;
	boost::smatch match;

	json_query["ignore"] = (boost::regex_match(query, boost::regex(".*ignore.*"))) ? 1 : 0;

	if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "empty_query"}
			})
		);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+(ignore\\s+){0,1}graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "graph"},
				{"graph", match[2]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+(ignore\\s+){0,1}graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "delete"},
				{"object", "graph"},
				{"graph", match[2]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+(ignore\\s+){0,1}block\\s+(\\w+):(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s*")

		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "block"},
				{"block", match[2]},
				{"block_type", match[3]},
				{"graph", match[4]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+(ignore\\s+){0,1}block\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "delete"},
				{"object", "block"},
				{"block", match[2]},
				{"graph", match[3]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*create\\s+(ignore\\s+){0,1}edge\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
		)
	)  {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "create"},
				{"object", "edge"},
				{"edge", match[2]},
				{"graph", match[3]},
				{"from", match[4]},
				{"to", match[5]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*delete\\s+(ignore\\s+){0,1}edge\\s+(\\w+)\\s+in\\s+graph\\s+(\\w+)\\s+from\\s+(\\w+)\\s+to\\s+(\\w+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "delete"},
				{"object", "edge"},
				{"edge", match[2]},
				{"graph", match[3]},
				{"from", match[4]},
				{"to", match[5]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*deploy\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "deploy"},
				{"object", "graph"},
				{"graph", match[1]},
			})
		);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*insert\\s+point\\s+('.+':\\d+:\\-{0,1}\\d*.{0,1}\\d*\\s*,{0,1}\\s*)+\\s+into(\\s+block\\s+(\\w+)\\s+of){0,1}\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		std::vector<std::string> points = Split(match[1], ',');
		Json::Value jpoints;

		for (size_t i = 0; i < points.size(); ++i) {
			boost::smatch point;
			if (
				boost::regex_match(
					points[i],
					point,
					boost::regex("\\s*'(.+)':(\\d+):(\\-{0,1}\\d*.{0,1}\\d*)\\s*")
				)
			) {

				Json::Value jpoint;
				jpoint["series"] = std::string(point[1]);
				jpoint["time"] = std::stoll(point[2]);
				jpoint["value"] = std::stod(point[3]);
				jpoints.append(jpoint);
			}
		}

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "insert"},
				{"graph", match[4]},
			})
		);
		json_query["points"] = jpoints;
		if (match[3] != std::string("")) {
			json_query["block"] = std::string(match[3]);
		}
 	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*modify\\s+param\\s+(\\w+)\\s+to\\s+(.+)\\s+in\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "modify"},
				{"object", "param"},
				{"name", match[1]},
				{"value", match[2]},
				{"block", match[3]},
				{"graph", match[4]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+graphs\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "graphs"},
			})
		);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+blocks\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "blocks"},
				{"graph", match[1]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+params\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "params"},
				{"block", match[1]},
				{"graph", match[2]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+edges\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "edges"},
				{"graph", match[1]}
			})
		);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*is\\s+graph\\s+(\\w+)\\s+deployed\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "is_deployed"},
				{"object", "graph"},
				{"graph", match[1]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+possible\\s+edges\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "possible_edges"},
				{"block", match[1]},
				{"graph", match[2]}
			})
		);
	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+block\\s+type\\s+of\\s+block\\s+(\\w+)\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "block_type"},
				{"block", match[1]},
				{"graph", match[2]}
			})
		);
	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+blocks\\s+types\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "types"},
			})
		);

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*show\\s+cycle\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "show"},
				{"object", "cycle"},
				{"graph", match[1]}
			})
		);

	} else if  (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*save\\s+graph\\s+(\\w+)\\s+to\\s+file\\s+(\\S+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "save"},
				{"object", "graph"},
				{"graph", match[1]},
				{"file", match[2]}
			})
		);


	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*load\\s+(ignore\\s+){0,1}\\s*(replace\\s+){0,1}\\s*graph\\s+(\\w+)\\s+from\\s+file\\s+(\\S+)\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "load"},
				{"object", "graph"},
				{"graph", match[3]},
				{"file", match[4]}
			})
		);
		json_query["replace"] = (match[2] == "") ? 0 : 1;

	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*convert\\s+config\\s+(\\S+)\\s+to\\s+queries\\s*")
		)
	) {

		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "convert"},
				{"file", match[1]}
			})
		);
	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*enable\\s+debug\\s+mode\\s+on\\s+graph\\s+(\\w+)")
		)
	) {
		json_query["type"] = "debug";
		json_query["object"] = "mode";
		json_query["enable"] = true;
		json_query["graph"] = std::string(match[1]);

	} else  if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*disable\\s+debug\\s+mode\\s+on\\s+graph\\s+(\\w+)")
		)
	) {
		json_query["type"] = "debug";
		json_query["object"] = "mode";
		json_query["enable"] = false;
		json_query["graph"] = std::string(match[1]);
	}  else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*get\\s+debug\\s+info\\s+of\\s+graph\\s+(\\w+)\\s*")
		)
	) {
		json_query = CreateJson(
			std::map<std::string, std::string>({
				{"type", "debug"},
				{"object", "info"},
				{"graph", match[1]}
			})
		);



	} else if (
		boost::regex_match(
			query,
			match,
			boost::regex("\\s*help\\s*")
		)
	) {
		json_query["type"] = "help";
	} else {
		throw GANException(529352, "Incorrect query");
	}

	if (
		boost::regex_match(
			query,
			boost::regex(".*ignore.*")
		)
	) {
		json_query["ignore"] =  1;
	} else {
		json_query["ignore"] = 0;
	}

	Json::FastWriter fastWriter;
	return fastWriter.write(json_query);
}


TerminalClient::TerminalClient(const std::string& ip, const std::string& port)
: BaseClient(ip, port)
{}

/*    DaemonBase      */

int DaemonBase::Connect() {
	int socketfd = Start();


	int status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1) {
		throw GANException(649265, "bind failed");
	}

	listen(socketfd, 10);

	return socketfd;

}


Json::Value DaemonBase::JsonRespond(const Json::Value& query) {
	return Json::Value();
}


DaemonBase::DaemonBase(const std::string& ip, const std::string& port)
: LibSocket(ip, port)
{
	Daemon();
}


DaemonBase::DaemonBase(const std::string& ip, const std::string& port, const int )
: LibSocket(ip, port)
{}


void DaemonBase::Daemon() {
	const size_t RECV_PART = 10000;

	double timeout = 900;
	struct timeval tv;
	tv.tv_sec = int(timeout);
	tv.tv_usec = int((timeout - tv.tv_sec) * 1e6);

	Prepare();

	int socketfd = Connect();

	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	while (1) {

		int client_socketfd = accept(socketfd, (struct sockaddr *) &cli_addr, &clilen);
		if (client_socketfd < 0) {
			throw GANException(539154, "Accepting client socket failed");
		}


		std::string query = GetMessage(RECV_PART, tv, client_socketfd);

		shutdown(client_socketfd, 0);

		if (query == "shutdown") {
			SendMessage(client_socketfd, "Ok");
			close(client_socketfd);
			break;
		}



		Json::Value answer;
		Json::Reader reader;
		try {
			Json::Value json_query;
			logger << "get query from client: " + query;
			bool is_parsing_successful = reader.parse(query, json_query);

			Json::FastWriter fastWriter;
			std::string j = fastWriter.write(json_query);
			if (!is_parsing_successful) {
				throw GANException(135167, "Incorrect query.");
			}

			answer = JsonRespond(json_query);
		} catch (std::exception& e) {
			answer["status"] = 0;
			answer["error"] = e.what();
		}
		Json::StyledWriter styledWriter;
		std::string str_answer = styledWriter.write(answer);
		logger << "answer = " + str_answer;

		SendMessage(client_socketfd, str_answer);

		close(client_socketfd);
	}
}


/*    EchoDaemon     */


Json::Value EchoDaemon::JsonRespond(const Json::Value& query) {
	return query;
}


EchoDaemon::EchoDaemon(const std::string& ip, const std::string& port)
: DaemonBase(ip, port, 0)
{
	Daemon();
}


