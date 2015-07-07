all: main

mysql.o: mysql.cpp
	g++ -c mysql.cpp -std=c++0x

main.o: main.cpp
	g++ -c main.cpp -std=c++0x

main: main.o mysql.o
	g++  -lmysqlcppconn -std=c++0x main.o mysql.o -o main

clean:
	rm -rf *.o main
