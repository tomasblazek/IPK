CC=g++
FLAGS= -std=c++11 -pedantic -Wall -Wextra

all:ftrest ftrestd

ftrest: client.cpp
	$(CC) $(FLAGS) client.cpp -o ftrest

ftrestd: server.cpp
	$(CC) $(FLAGS) server.cpp -o ftrestd
