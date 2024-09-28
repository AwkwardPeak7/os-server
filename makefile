build: clean
	@mkdir build
	gcc -O3 server.c -o build/server -lcjson -lpthread -D_GNU_SOURCE

run:
	build/server

clean:
	rm -rf build

all: build
