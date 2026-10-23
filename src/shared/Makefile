build:
	g++ -O2 -shared -o libshared.so logger.cpp message.cpp

install:
	mkdir -p ${out}/include
	mkdir -p ${out}/lib
	cp *.h ${out}/include
	cp libshared.so ${out}/lib
