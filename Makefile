all: main client server graph

mysql.o: mysql.cpp
	g++ -c -std=c++0x mysql.cpp

main.o: main.cpp
	g++ -c -std=c++0x main.cpp

main: main.o mysql.o
	g++  -lmysqlcppconn -std=c++0x main.o mysql.o -o main


daemons.o: daemons.cpp
	g++ -g -std=c++0x -c daemons.cpp

server.o: server.cpp
	g++ -g -c -std=c++0x server.cpp

server: server.o daemons.o
	g++  -std=c++0x server.o daemons.o -o server

client.o: client.cpp
	g++ -g -std=c++0x -c client.cpp

client: client.o daemons.o
	g++  -std=c++0x -g client.o daemons.o -o client


graph.o: graph.cpp
	g++ -lboost_regex -std=c++0x -g -c graph.cpp

graph_test.o: graph_test.cpp
	g++ -lboost_regex -std=c++0x -g -c graph_test.cpp

graph: graph.o graph_test.o daemons.o mysql.o
	g++ -lboost_regex -lmysqlcppconn -std=c++0x -g mysql.o  daemons.o graph.o graph_test.o -o graph


clean:
	rm -rf *.o main client server graph
