all: server

daemons.o: daemons.cpp
	g++ -g -c daemons.cpp -std=c++0x

server.o: server.cpp
	g++ -g -c server.cpp -std=c++0x

server: server.o daemons.o
	g++  -std=c++0x server.o daemons.o -o server

clean:
	rm -rf *.o server
