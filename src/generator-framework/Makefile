build:
	g++ -shared -o libgenerator-framework.so -std=c++26 -freflection framework.cpp task.cpp remote_sink.cpp -lcore

install:
	mkdir -p ${out}/include
	mkdir -p ${out}/lib
	cp *.h ${out}/include
	cp libgenerator-framework.so ${out}/lib
