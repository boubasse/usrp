CXX := clang++
CPP := g++
CC  := gcc

#CXXFLAGS := -Wall -g -O2 -D DIGITHIN
#CXXFLAGS := -Wall -g -O2 -Wno-unused-variable -Wno-unused-function
CXXFLAGS := -Wall -g -O2 -Wextra
LDFLAGS := -lm -lrt -lpthread -luhd


all:	usrptx.run usrprx.run

usrptx.run : src/main_usrp_tx_c.c
		$(CC) $(CXXFLAGS) -o bin/usrptx.run src/main_usrp_tx_c.c $(LDFLAGS)

usrprx.run : src/main_usrp_rx_c.c
		$(CC) $(CXXFLAGS) -o bin/usrprx.run src/main_usrp_rx_c.c $(LDFLAGS)
			

clean:
	rm -f bin/usrptx.run bin/usrprx.run bin/*.o 
