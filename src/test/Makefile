CXXFLAGS = -std=c++11 -I..

all: test_graph
	./test_graph

test_graph: test_graph.cc ../graph.h
	$(CXX) $(CXXFLAGS) test_graph.cc -o test_graph
