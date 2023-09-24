all: client server;

client: client.cpp
	g++ -lpthread client.cpp -o client

server: server.cpp
	g++ -lpthread server.cpp -o server

clean:
	rm -rf client server client.o server.o
