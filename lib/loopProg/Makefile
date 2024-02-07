CXX := clang++
CPP := g++
CC  := gcc

#CXXFLAGS := -Wall -g -O2 -D DIGITHIN
#CXXFLAGS := -Wall -g -O2 -Wno-unused-variable -Wno-unused-function
CXXFLAGS := -Wall -g -O2 -Werror
LDFLAGS := -lm -lrt -lpthread

all:	loopRead.run

loopRead.run : src/main_loopRead.cpp
		$(CPP) $(CXXFLAGS) -o bin/loopRead.run src/main_loopRead.cpp $(LDFLAGS)
			
clean:
	rm -f bin/loopRead.run bin/*.o 
