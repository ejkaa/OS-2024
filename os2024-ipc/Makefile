CXX = g++
CXXFLAGS = -std=c++11
BFILES = zadanie proc_serv2 proc_d proc_t proc_p1 proc_p2
OFILES = proc_pr.err proc_pr.out proc_serv1.err proc_serv1.out proc_t.err proc_t.out proc_s.err proc_s.out serv2.txt

all: $(BFILES)

zadanie: zadanie.cpp
	$(CXX) $(CXXFLAGS) zadanie.cpp -o zadanie

proc_serv2: proc_serv2.cpp
	$(CXX) $(CXXFLAGS) proc_serv2.cpp -o proc_serv2

proc_d: proc_d.cpp
	$(CXX) $(CXXFLAGS) proc_d.cpp -o proc_d

proc_t: proc_t.cpp
	$(CXX) $(CXXFLAGS) proc_t.cpp -o proc_t

proc_p1: proc_p1.cpp
	$(CXX) $(CXXFLAGS) proc_p1.cpp -o proc_p1

proc_p2: proc_p2.cpp
	$(CXX) $(CXXFLAGS) proc_p2.cpp -o proc_p2

clean:
	rm -f $(BFILES) $(OFILES)

.PHONY: all clean
