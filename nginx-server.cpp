#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <iostream>
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <json/json.h>
#include <string>
#include <cgicc/Cgicc.h>
#include "daemons.h"
#include "mysql.h"
#define THREAD_COUNT 1
#define SOCKET_PATH "127.0.0.1:9000"

static int socketId;


std::string ParseURLQuery(const std::string& queries) {
	std::vector<std::string> json_queries;
	std::vector<std::string> parts = Split(queries, '&');
	for (size_t i = 0; i < parts.size(); ++i) {
		std::vector<std::string> query = Split(parts[i], '=');
		if (query[0] == "json") {
			return cgicc::form_urldecode(query[1]);
		}
	}
	return "{}";
}



static void *doit(void *a) {
	int rc;
	FCGX_Request request;

	if(FCGX_InitRequest(&request, socketId, 0) != 0) {
		printf("Can not init request\n");
		return NULL;
	}
	printf("Request is inited\n");


	const size_t RECV_PART = 10000;
	double timeout = 900;
	struct timeval tv;
	tv.tv_sec = int(timeout);
	tv.tv_usec = int((timeout - tv.tv_sec) * 1e6);

	NginxClient client("127.0.0.1", "8081");

	for(;;) {
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

		printf("Try to accept new request\n");
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if(rc < 0) {
			printf("Can not accept new request\n");
			break;
		}
		printf("request is accepted\n");

		char* query_string = FCGX_GetParam("QUERY_STRING", request.envp);
		std::string s = query_string;
		std::cout << "Got query " << std::string(query_string) << std::endl;
		std::string json_query = ParseURLQuery(std::string(query_string));

		client.AddQuery(json_query);
		std::string answer;
		client.Conversation(&answer, RECV_PART, tv);
		FCGX_PutS("Content-type: application/json\r\n\r\n", request.out);
		FCGX_PutS(answer.c_str(), request.out);
		FCGX_PutS("\r\n", request.out);

		FCGX_Finish_r(&request);

	}

	return NULL;
}

int main(void) {
	int i;
	pthread_t id[THREAD_COUNT];

	FCGX_Init();
	printf("Lib is inited\n");

	socketId = FCGX_OpenSocket(SOCKET_PATH, 20);
	if(socketId < 0) {
		return 1;
	}
	printf("Socket is opened\n");

	for(i = 0; i < THREAD_COUNT; i++) {
		pthread_create(&id[i], NULL, doit, NULL);
	}

	for(i = 0; i < THREAD_COUNT; i++) {
		pthread_join(id[i], NULL);
	}

	return 0;
}
