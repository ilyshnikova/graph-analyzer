all: mysql-test client test-server gan-server

gan-exception.o: gan-exception.cpp
	g++ -c -g -std=c++0x gan-exception.cpp

logger.o: logger.cpp
	g++ -c -g  -std=c++0x logger.cpp

mysql.o: mysql.cpp
	g++ -c -g -std=c++0x mysql.cpp

mysql-test.o: mysql-test.cpp
	g++ -c -g -Wall -std=c++0x mysql-test.cpp

mysql-test: mysql-test.o mysql.o gan-exception.o logger.o base64.o
	g++ -g -Wall -lmysqlcppconn -std=c++0x gan-exception.o mysql-test.o mysql.o logger.o base64.o -o mysql-test

daemons.o: daemons.cpp
	g++ -g -std=c++0x -c daemons.cpp

test-server.o: test-server.cpp
	g++ -g -c -std=c++0x test-server.cpp

test-server: test-server.o daemons.o gan-exception.o logger.o
	g++ -g  -std=c++0x test-server.o daemons.o  gan-exception.o logger.o -o test-server

client.o: client.cpp
	g++ -g -std=c++0x -c client.cpp

client: client.o daemons.o gan-exception.o logger.o
	g++  -std=c++0x -g client.o daemons.o gan-exception.o logger.o -o client


graph.o: graph.cpp
	g++ -lboost_regex -std=c++0x -g -c graph.cpp

gan-server.o: gan-server.cpp
	g++ -lboost_regex -std=c++0x -g -c gan-server.cpp

gan-server: gan-exception.o graph.o gan-server.o daemons.o mysql.o logger.o base64.o
	g++ -lboost_regex -lmysqlcppconn -std=c++0x -g mysql.o gan-exception.o daemons.o graph.o gan-server.o logger.o base64.o -o gan-server

base64.o: base64.cpp
	g++ -g -std=c++0x -c base64.cpp

clean:
	rm -rf *.o mysql-test client test-server gan-server
