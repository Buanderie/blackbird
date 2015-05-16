# Blackbird Bitcoin Arbitrage Makefile

CC = g++
CFLAGS = -c -g -Wall -O2 
# CFLAGS = -c -g -Wall -O2 -fno-stack-protector

EXEC = blackbird
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: EXEC

EXEC: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) -lcrypto -ljansson -lcurl

%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clearscreen:
	clear

clean:
	rm -rf core $(OBJECTS)
