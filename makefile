build: clean
	@mkdir build
	gcc -O3 server.c -o build/server -lcjson

run:
	@build/server

clean:
	rm -rf build
