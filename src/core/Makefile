build:
	g++ -O2 -std=c++26 -freflection -shared -o libcore.so logger.cpp message.cpp

install:
	mkdir -p ${out}/include
	mkdir -p ${out}/lib
	cp *.h ${out}/include
	cp libcore.so ${out}/lib
