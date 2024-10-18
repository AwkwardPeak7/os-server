queue: utils/queue/queue.c utils/queue/queue.h
	@mkdir -p build/queue
	gcc -O3 utils/queue/queue.c -o build/queue/queue.o -c

map: utils/map/map.c utils/map/map.h
	@mkdir -p build/map
	gcc -O3 utils/map/map.c -o build/map/map.o -c

filesystem: utils/filesystem/filesystem.c utils/filesystem/filesystem.h
	@mkdir -p build/filesystem
	gcc -O3 utils/filesystem/filesystem.c -o build/filesystem/filesystem.o -c

server: server.c
	@mkdir -p build
	gcc -O3 server.c -o build/server.o -D_GNU_SOURCE -c

all: queue map filesystem server
	gcc build/queue/queue.o build/map/map.o build/filesystem/filesystem.o build/server.o -o build/server.elf -lpthread -lcjson

build: all

run:
	build/server.elf

clean:
	rm -rf build
