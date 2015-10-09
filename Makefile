all: mysql-test client test-server gan-server testing
gan-exception.o: gan-exception.cpp
	g++ -c -g -std=c++0x -Wall gan-exception.cpp

logger.o: logger.cpp
	g++ -c -g  -std=c++0x -Wall logger.cpp

mysql.o: mysql.cpp
	g++ -c -g -std=c++0x -Wall  mysql.cpp

mysql-test.o: mysql-test.cpp
	g++ -c -g -Wall -std=c++0x mysql-test.cpp

mysql-test: mysql-test.o mysql.o gan-exception.o logger.o base64.o
	g++ -g -Wall -lmysqlcppconn -std=c++0x -Wall gan-exception.o mysql-test.o mysql.o logger.o base64.o -o mysql-test

daemons.o: daemons.cpp
	g++ -g -std=c++0x -c -Wall daemons.cpp

test-server.o: test-server.cpp
	g++ -g -c -std=c++0x -Wall test-server.cpp

test-server: test-server.o daemons.o gan-exception.o logger.o
	g++ -g  -std=c++0x -Wall test-server.o daemons.o  gan-exception.o logger.o -o test-server

client.o: client.cpp
	g++ -g -std=c++0x -c -Wall client.cpp

client: client.o daemons.o gan-exception.o logger.o
	g++  -std=c++0x -g -Wall client.o daemons.o gan-exception.o logger.o -o client


graph.o: graph.cpp
	g++ -lboost_regex -std=c++0x -g -Wall -I/usr/include/jsoncpp  -ljsoncpp -c graph.cpp

gan-server.o: gan-server.cpp
	g++ -lboost_regex -std=c++0x -Wall -I/usr/include/jsoncpp  -ljsoncpp -g -c gan-server.cpp

gan-server: gan-exception.o graph.o gan-server.o daemons.o mysql.o logger.o base64.o execute.o
	g++ -lboost_regex -lmysqlcppconn -std=c++0x -Wall -I/usr/include/jsoncpp  -ljsoncpp  -lyaml-cpp -g mysql.o gan-exception.o daemons.o graph.o gan-server.o logger.o base64.o execute.o -o gan-server

base64.o: base64.cpp
	g++ -g -std=c++0x -c -Wall base64.cpp

testing.o: testing.cpp
	g++ -g -c -std=c++0x -lyaml-cpp  -Wall -I/usr/include/jsoncpp  -ljsoncpp testing.cpp

testing: testing.o gan-exception.o graph.o  daemons.o mysql.o logger.o base64.o execute.o
	g++ -lboost_regex -lmysqlcppconn -lyaml-cpp -std=c++0x  -I/usr/include/jsoncpp  -ljsoncpp -g -Wall testing.o mysql.o gan-exception.o execute.o  daemons.o graph.o  logger.o base64.o  -o test

execute.o: execute.cpp
	g++ -g -c -std=c++0x -Wall execute.cpp

clean:
	rm -rf *.o mysql-test client test-server gan-server test
