#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>

#include <iostream>

#include "fcgi_config.h"
#include "fcgiapp.h"

#include <json/json.h>

#include "daemons.h"
#define THREAD_COUNT 8
#define SOCKET_PATH "127.0.0.1:9000"

//хранит дескриптор открытого сокета
static int socketId;

static void *doit(void *a)
{
	int rc;
	FCGX_Request request;

	if(FCGX_InitRequest(&request, socketId, 0) != 0)
	{
		//ошибка при инициализации структуры запроса
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

	for(;;)
	{
		static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

		//попробовать получить новый запрос
		printf("Try to accept new request\n");
		pthread_mutex_lock(&accept_mutex);
		rc = FCGX_Accept_r(&request);
		pthread_mutex_unlock(&accept_mutex);

		if(rc < 0)
		{
			//ошибка при получении запроса
			printf("Can not accept new request\n");
			break;
		}
		printf("request is accepted\n");

		//получить значение переменной
		char* query_string = FCGX_GetParam("QUERY_STRING", request.envp);

		client.AddQuery(std::string(query_string));
		std::string answer;
		client.Conversation(&answer, RECV_PART, tv);

		//вывести все HTTP-заголовки (каждый заголовок с новой строки)
		FCGX_PutS("Content-type: text/html\r\n", request.out);
		//между заголовками и телом ответа нужно вывести пустую строку
		FCGX_PutS("\r\n", request.out);

	 	FCGX_PutS("<html>\r\n", request.out);
		FCGX_PutS("<head>\r\n", request.out);
		FCGX_PutS(std::string("<title>" + answer  +  "</title>\r\n").c_str(), request.out);
		FCGX_PutS("<body>\r\n", request.out);
		FCGX_PutS(std::string(answer  +"\r\n").c_str(), request.out);
		FCGX_PutS("</body>\r\n", request.out);
		FCGX_PutS("</html>\r\n", request.out);


		//закрыть текущее соединение
		FCGX_Finish_r(&request);

		//завершающие действия - запись статистики, логгирование ошибок и т.п.
	}

	return NULL;
}

int main(void)
{
	int i;
	pthread_t id[THREAD_COUNT];

	//инициализация библилиотеки
	FCGX_Init();
	printf("Lib is inited\n");

	//открываем новый сокет
	socketId = FCGX_OpenSocket(SOCKET_PATH, 20);
	if(socketId < 0)
	{
		//ошибка при открытии сокета
		return 1;
	}
	printf("Socket is opened\n");

	//создаём рабочие потоки
	for(i = 0; i < THREAD_COUNT; i++)
	{
		pthread_create(&id[i], NULL, doit, NULL);
	}

	//ждем завершения рабочих потоков
	for(i = 0; i < THREAD_COUNT; i++)
	{
		pthread_join(id[i], NULL);
	}

	return 0;
}
