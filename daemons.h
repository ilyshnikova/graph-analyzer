#ifndef DAEMONS
#define DAEMONS

#include <string>

#include <exception>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <iostream>

class LibSocket {
public:

	std::string ip;
	std::string port;
	struct addrinfo host_info;
	struct addrinfo * host_info_list;

	LibSocket(const std::string& ip, const std::string& port);

	class SocketExceptions : public std::exception {
	private:
		std::string reason;

	public:
		SocketExceptions(const std::string& reason);

		const char * what() const throw();

		~SocketExceptions() throw();
	};

	void Prepare();

	int Start();

	std::string GetMessage(const size_t RECV_PART, struct timeval tv, const int socketfd) const;

	void SendMessage(const int socketfd, const std::string& send_message) const;



};

class Client : public LibSocket {
private:
	int Connect() ;

public:
	Client(const std::string& ip, const std::string& port);
};


class DaemonBase : public LibSocket {
private:
	int Connect() ;


	virtual std::string Respond(const std::string& query) const;

public:

	DaemonBase(const std::string& ip, const std::string& port);

	DaemonBase(const std::string& ip, const std::string& port, const int option);

	void Daemon();
};


class EchoDaemon : public DaemonBase {
private:
	std::string Respond(const std::string& query) const;

public:
	EchoDaemon(const std::string& ip, const std::string& port);

};

#endif

