COMMON_OBJS := $(patsubst %.cpp,%.o,$(wildcard common/*.cpp))
CLIENT_OBJS := $(patsubst %.cpp,%.o,$(wildcard client/*.cpp))
SERVER_OBJS := $(patsubst %.cpp,%.o,$(wildcard server/*.cpp))
INC := $(wildcard common/*.h) $(wildcard client/*.h) $(wildcard server/*.h)

all: game_client game_server standalone

game_client: $(COMMON_OBJS) $(CLIENT_OBJS) game_client.o Makefile
	g++ -g -std=c++11 -o $@ $(COMMON_OBJS) $(CLIENT_OBJS) game_client.o

game_server: $(COMMON_OBJS) $(SERVER_OBJS) game_server.o Makefile
	g++ -g -std=c++11 -o $@ $(COMMON_OBJS) $(SERVER_OBJS) game_server.o

standalone: $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) standalone.o
	g++ -g -std=c++11 -o $@ $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) standalone.o

$(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) game_client.o game_server.o standalone.o: %.o: %.cpp $(INC) Makefile
	g++ -g -std=c++11 -o $@ -Icommon -Iclient -Iserver -c $<

clean:
	rm -f game_client game_server standalone
	rm -f common/*.o
	rm -f client/*.o
	rm -f server/*.o
	rm -f *.o
