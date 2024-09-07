build: clean
	@mkdir build
	gcc -O3 -lcjson server.c -o build/server

run:
	@build/server

clean:
	rm -rf build
