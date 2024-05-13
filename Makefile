#Brandon Skeens
#826416091

CXX=g++

CXXFLAGS=-std=c++11 -Wall -g3 -c

OBJS = pagingwithpr.o log_helpers.o vaddr_tracereader.o PageTable.o

PROGRAM = pagingwithpr


$(PROGRAM) : $(OBJS) 
	$(CXX) -o $(PROGRAM) $^

pagingwithpr.o : pagingwithpr.cpp pagingwithpr.h vaddr_tracereader.h log_helpers.h PageTable.h
	$(CXX) $(CXXFLAGS) pagingwithpr.cpp

log_helpers.o : log_helpers.cpp log_helpers.h
	$(CXX) $(CXXFLAGS) log_helpers.cpp
	
vaddr_tracereader.o : vaddr_tracereader.cpp vaddr_tracereader.h
	$(CXX) $(CXXFLAGS) vaddr_tracereader.cpp

PageTable.o : PageTable.cpp PageTable.h
	$(CXX) $(CXXFLAGS) PageTable.cpp

clean :
	rm -f *.o $(PROGRAM)

