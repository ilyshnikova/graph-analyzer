all: main client server

mysql.o: mysql.cpp
	g++ -c -std=c++0x mysql.cpp

daemons.o: daemons.cpp
	g++ -g -std=c++0x -c daemons.cpp

main.o: main.cpp
	g++ -c -std=c++0x main.cpp

server.o: server.cpp
	g++ -g -c -std=c++0x server.cpp

server: server.o daemons.o
	g++  -std=c++0x server.o daemons.o -o server

client.o: client.cpp
	g++ -g -std=c++0x -c client.cpp

client: client.o daemons.o
	g++  -std=c++0x -g client.o daemons.o -o client


main: main.o mysql.o
	g++  -lmysqlcppconn -std=c++0x main.o mysql.o daemons.o -o main

clean:
	rm -rf *.o main client server
