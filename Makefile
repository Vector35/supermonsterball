COMMON_OBJS := $(patsubst %.cpp,%.o,$(wildcard common/*.cpp))
CLIENT_OBJS := $(patsubst %.cpp,%.o,$(wildcard client/*.cpp))
SERVER_OBJS := $(patsubst %.cpp,%.o,$(wildcard server/*.cpp))
INC := $(wildcard common/*.h) $(wildcard client/*.h) $(wildcard server/*.h)

CFLAGS := -I/usr/local/include -g -O3 -DOSATOMIC_DEPRECATED
LDFLAGS := -L/usr/local/lib

all: game_client game_server standalone

common/request.pb.cc: common/request.proto Makefile
	protoc -I=common --cpp_out=common common/request.proto

common/request.pb.o: common/request.pb.cc common/request.proto
	g++ $(CFLAGS) -std=c++11 -o $@ -Icommon -Iclient -Iserver -c $<

server/sqlite3.o: server/sqlite3.c
	gcc $(CFLAGS) -o $@ -Iserver -c $<

game_client: $(COMMON_OBJS) $(CLIENT_OBJS) game_client.o common/request.pb.o Makefile
	g++ $(CFLAGS) $(LDFLAGS) -std=c++11 -o $@ $(COMMON_OBJS) $(CLIENT_OBJS) game_client.o common/request.pb.o -lprotobuf -lssl -lcrypto

game_server: $(COMMON_OBJS) $(SERVER_OBJS) server/sqlite3.o game_server.o common/request.pb.o Makefile
	g++ $(CFLAGS) $(LDFLAGS) -std=c++11 -o $@ $(COMMON_OBJS) $(SERVER_OBJS) server/sqlite3.o game_server.o common/request.pb.o -lprotobuf -lssl -lcrypto

standalone: $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) server/sqlite3.o standalone.o
	g++ $(CFLAGS) $(LDFLAGS) -std=c++11 -o $@ $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) server/sqlite3.o standalone.o common/request.pb.o -lprotobuf -lssl -lcrypto

$(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS) game_client.o game_server.o standalone.o: %.o: %.cpp $(INC) Makefile common/request.pb.o
	g++ $(CFLAGS) -std=c++11 -o $@ -Icommon -Iclient -Iserver -c $<

clean:
	rm -f game_client game_server standalone
	rm -f common/*.o
	rm -f client/*.o
	rm -f server/*.o
	rm -f *.o
