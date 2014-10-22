all:
	mkdir -p bin
	g++ -o bin/mr main.cpp ../../src/platform.cpp -lpthread -lrt -g
	ln -s bin/mr ./run_mr

data: FORCE
	mkdir -p bin
	g++ -o bin/datagen ../../datagen/random4byte.c
	./bin/datagen
	mv random.dat bin/file00.dat
	./bin/datagen
	mv random.dat bin/file01.dat
	./bin/datagen
	mv random.dat bin/file02.dat
	./bin/datagen
	mv random.dat bin/file03.dat
	./bin/datagen
	mv random.dat bin/file04.dat
	./bin/datagen
	mv random.dat bin/file05.dat
	./bin/datagen
	mv random.dat bin/file06.dat
	./bin/datagen
	mv random.dat bin/file07.dat

clean: FORCE
	rm -rf bin
	rm ./run_mr

FORCE:
