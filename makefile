build: all

map: utils/map/map.c utils/map/map.h
	@mkdir -p build/map
	gcc -O3 utils/map/map.c -o build/map/map.o -c

filesystem: utils/filesystem/filesystem.c utils/filesystem/filesystem.h
	@mkdir -p build/filesystem
	gcc -O3 utils/filesystem/filesystem.c -o build/filesystem/filesystem.o -c

config: utils/config/config.c utils/config/config.h
	@mkdir -p build/config
	gcc -O3 utils/config/config.c -o build/config/config.o -c

transfer: transfer/transfer.c transfer/transfer.h
	@mkdir -p build/transfer
	gcc -O3 transfer/transfer.c -o build/transfer/transfer.o -c

arena: arena/arena.c arena/arena.h
	@mkdir -p build/arena
	gcc -O3 arena/arena.c -o build/arena/arena.o -c

server: server.c
	@mkdir -p build
	gcc -O3 server.c -o build/server.o -D_GNU_SOURCE -c

all: map filesystem config transfer server arena
	gcc -O3 build/map/map.o build/filesystem/filesystem.o build/config/config.o build/transfer/transfer.o build/arena/arena.o build/server.o -o build/server -lpthread -lcjson

run:
	build/server

clean:
	rm -rf build
