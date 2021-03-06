all: client gan-server nginx-server send-email
BASEP = -Wall -std=c++0x -gdwarf-3

gan-exception.o: gan-exception.cpp
	g++ $(BASEP) -c gan-exception.cpp

logger.o: logger.cpp
	g++ $(BASEP) -c logger.cpp

mysql.o: mysql.cpp
	g++ $(BASEP) -c mysql.cpp

mysql-test.o: mysql-test.cpp
	g++ $(BASEP) -c mysql-test.cpp

mysql-test: mysql-test.o mysql.o gan-exception.o logger.o base64.o
	g++ $(BASEP) -lmysqlcppconn gan-exception.o mysql-test.o mysql.o logger.o base64.o -o mysql-test

daemons.o: daemons.cpp
	g++ $(BASEP) -c -lboost_regex -I/usr/include/jsoncpp -ljsoncpp daemons.cpp

test-server.o: test-server.cpp
	g++ $(BASEP) -c -lboost_regex -I/usr/include/jsoncpp -ljsoncpp test-server.cpp

test-server: test-server.o daemons.o gan-exception.o logger.o graph.o mysql.o base64.o execute.o
	g++ $(BASEP) -lboost_regex -lyaml-cpp -lmysqlcppconn -I/usr/include/jsoncpp -ljsoncpp test-server.o daemons.o execute.o gan-exception.o logger.o graph.o mysql.o base64.o -o test-server

client.o: client.cpp
	g++ $(BASEP) -c -lboost_regex -I/usr/include/jsoncpp -ljsoncpp client.cpp

client: client.o daemons.o gan-exception.o logger.o graph.o mysql.o base64.o execute.o
	g++ $(BASEP) -lmysqlcppconn -lyaml-cpp -I/usr/include/jsoncpp -lboost_regex -ljsoncpp client.o daemons.o gan-exception.o mysql.o execute.o base64.o logger.o graph.o -o client

graph.o: graph.cpp
	g++ $(BASEP) -c -lboost_regex -I/usr/include/jsoncpp -ljsoncpp graph.cpp

gan-server.o: gan-server.cpp
	g++ $(BASEP) -c -lboost_regex -I/usr/include/jsoncpp -ljsoncpp gan-server.cpp

gan-server: gan-exception.o graph.o gan-server.o daemons.o mysql.o logger.o base64.o execute.o
	g++ $(BASEP) -lboost_regex -lmysqlcppconn -I/usr/include/jsoncpp -ljsoncpp -lyaml-cpp mysql.o gan-exception.o daemons.o graph.o gan-server.o logger.o base64.o execute.o -o gan-server

base64.o: base64.cpp
	g++ $(BASEP) -c base64.cpp

testing.o: testing.cpp
	g++ $(BASEP) -c -lyaml-cpp -I/usr/include/jsoncpp -ljsoncpp testing.cpp

testing: testing.o gan-exception.o graph.o daemons.o mysql.o logger.o base64.o execute.o
	g++ $(BASEP) -lboost_regex -lmysqlcppconn -lyaml-cpp -I/usr/include/jsoncpp -ljsoncpp testing.o mysql.o gan-exception.o execute.o daemons.o graph.o logger.o base64.o -o test

execute.o: execute.cpp
	g++ $(BASEP) -c execute.cpp

nginx-server.o: nginx-server.cpp
	g++ $(BASEP) -c -lfcgi -lpthread -I/usr/include/jsoncpp -lcgicc -lcurl -ljsoncpp -lboost_regex nginx-server.cpp

nginx-server: nginx-server.o daemons.o gan-exception.o logger.o graph.o mysql.o base64.o execute.o
	g++ $(BASEP) -lfcgi -lpthread -lmysqlcppconn -lyaml-cpp -lcgicc  -lcurl -I/usr/include/jsoncpp -lboost_regex -ljsoncpp  daemons.o gan-exception.o logger.o graph.o mysql.o base64.o execute.o nginx-server.o -o nginx-server

send-email: send_email.cpp
	g++ $(BASEP) send_email.cpp -o send_email

clean:
	rm -rf *.o mysql-test client test-server gan-server test nginx-server
