#include <string>
#include <iostream>

#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include "gan-exception.h"
#include "daemons.h"

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

/*    Client      */

int Client::Connect()  {
	int socketfd = Start();

	int status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1) {
		throw GANException(247528, "connect failed");
	}

	return socketfd;

}



Client::Client(const std::string& ip, const std::string& port)
: LibSocket(ip, port)
{
	const size_t RECV_PART = 10000;

	double timeout = 900;
	struct timeval tv;
	tv.tv_sec = int(timeout);
	tv.tv_usec = int((timeout - tv.tv_sec) * 1e6);

	Prepare();

	while (1) {

		std::cout << "gan> ";
		std::string query;
		if (!std::getline(std::cin, query)) {
			std::cout << "\n";
			break;
		}

		int socketfd = Connect();

		SendMessage(socketfd, query);
		shutdown(socketfd, 1);

		std::string got_message = GetMessage(RECV_PART, tv, socketfd);


		std::cout << got_message << "\n";
		freeaddrinfo(host_info_list);
		close(socketfd);
	}
}


/*    DaemonBase      */

int DaemonBase::Connect() {
	std::string ip = "127.0.0.1";
	std::string port = "8081";

	int socketfd = Start();


	int status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1) {
		throw GANException(649265, "bind failed");
	}

	listen(socketfd, 10);

	return socketfd;

}


std::string DaemonBase::Respond(const std::string& query) {
	return std::string("\0");
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
			break;
		}



		std::string message;

		try {
			message = Respond(query);
		} catch (std::exception& e) {
			message = e.what();
		}

		SendMessage(client_socketfd, message);

		close(client_socketfd);
	}
}


/*    EchoDaemon     */


std::string EchoDaemon::Respond(const std::string& query) {
	return query;
}


EchoDaemon::EchoDaemon(const std::string& ip, const std::string& port)
: DaemonBase(ip, port, 0)
{
	Daemon();
}


