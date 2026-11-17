build:
	g++ -o builder -std=c++26 -freflection main.cpp remote_sink.cpp -lcore

install:
	mkdir -p ${out}/bin
	cp builder ${out}/bin/
