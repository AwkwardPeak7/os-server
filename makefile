build:
	@mkdir build
	gcc -O3 server.c -o build/server

run: build
	@build/server

clean:
	rm -rf build
