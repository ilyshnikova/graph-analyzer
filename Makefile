all: mysql-test client test-server gan-server

mysql.o: mysql.cpp
	g++ -c -std=c++0x mysql.cpp

mysql-test.o: mysql-test.cpp
	g++ -c -std=c++0x mysql-test.cpp

mysql-test: mysql-test.o mysql.o
	g++  -lmysqlcppconn -std=c++0x mysql-test.o mysql.o -o mysql-test


daemons.o: daemons.cpp
	g++ -g -std=c++0x -c daemons.cpp

test-server.o: test-server.cpp
	g++ -g -c -std=c++0x test-server.cpp

test-server: test-server.o daemons.o
	g++  -std=c++0x test-server.o daemons.o -o test-server

client.o: client.cpp
	g++ -g -std=c++0x -c client.cpp

client: client.o daemons.o
	g++  -std=c++0x -g client.o daemons.o -o client


graph.o: graph.cpp
	g++ -lboost_regex -std=c++0x -g -c graph.cpp

gan-server.o: gan-server.cpp
	g++ -lboost_regex -std=c++0x -g -c gan-server.cpp

gan-server: graph.o gan-server.o daemons.o mysql.o
	g++ -lboost_regex -lmysqlcppconn -std=c++0x -g mysql.o  daemons.o graph.o gan-server.o -o gan-server


clean:
	rm -rf *.o mysql-test client test-server gan-server
