CC = g++
LOADLIBES = -lpthread
CXXFLAGS = -Wall -O2 -g

SRC_C = client.cpp
SRC_S = server.cpp
SRC = $(SRC_C) $(SRC_S)
BUILD = $(SRC:.cpp=)
OBJS = $(SRC:.cpp=.o)
AUX = $(SRC:.cpp=.h)

all: $(OBJS) $(SRC) client server;

client: client.cpp
	$(CC) $(LOADLIBES) $(SRC_C) -o client

server: server.cpp
	$(CC) $(LOADLIBES) $(SRC_S) -o server

clean:
	rm -f $(OBJS) $(AUX) $(BUILD)