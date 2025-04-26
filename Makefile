# Makefile for Stock Trading System C++ project

CXX             := g++
CXXFLAGS        := -std=c++17 -Wall -Wextra -g

TARGETS := serverM serverA serverP serverQ client

all: $(TARGETS)

serverM: serverM.cpp helper.cpp helper.h serverM.h
	$(CXX) $(CXXFLAGS) serverM.cpp helper.cpp -o serverM 

serverA: serverA.cpp helper.cpp helper.h serverA.h
	$(CXX) $(CXXFLAGS) serverA.cpp helper.cpp -o serverA

serverP: serverP.cpp helper.cpp helper.h serverP.h
	$(CXX) $(CXXFLAGS) serverP.cpp helper.cpp -o serverP

serverQ: serverQ.cpp helper.cpp helper.h serverQ.h
	$(CXX) $(CXXFLAGS) serverQ.cpp helper.cpp -o serverQ

client: client.cpp helper.cpp helper.h client.h
	$(CXX) $(CXXFLAGS) client.cpp helper.cpp -o client

clean:
	rm -f $(TARGETS)

.PHONY: all clean
