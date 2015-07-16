all: client

daemons.o: daemons.cpp
	g++ -g -std=c++0x -c daemons.cpp
client.o: client.cpp
	g++ -g -std=c++0x -c client.cpp -g -std=c++0x

client: client.o daemons.o
	g++  -std=c++0x -g client.o daemons.o -o client

clean:
	rm -rf *.o client
