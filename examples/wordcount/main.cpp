#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>

#include "../../src/platform.h"

// Type definitions /////////////////////////////////////////

class Input4Byte : public InputPair {
public:
	unsigned int data1;
};

class InternalKVPair : public Pair {
	public:
	bool done;
	unsigned int data;
};

class TestContext : public Context {
public:
	TestContext(unsigned int key) : Context(key) {
		count = 0;
	};
	unsigned int count;
};

// End type definitions ///////////////////////////////////

// Start input parser implementation //////////////////////
class NullInputParser : public InputParser {
public:

	private:
	FILE* infile;
	int readBufferCount;

	Input4Byte* readBuffer;
	int curPairIndex;
	int loadedPairCount;

	uint64_t curKey;

	bool readFinished;

	public:
	NullInputParser(const char* path) : InputParser() {
		readBufferCount = 8192;
		curPairIndex = 0;
		curKey = 0;
		readBuffer = (Input4Byte*)malloc(sizeof(Input4Byte)*readBufferCount);
		infile = fopen(path, "r");
		if ( !infile ) {
			fprintf(stderr, "ERROR: failed to open file %s to read\n", path );
		}
		readFinished = false;
		loadedPairCount = 0;
	}
	void addPair(Input4Byte p) {
		readBuffer[loadedPairCount] = p;
		loadedPairCount++;
	}
	void parse(char* buffer) {
		unsigned int* uib = (unsigned int*)buffer;
		Input4Byte p;
		p.done = false;
		for ( int i = 0; i < 8192/sizeof(unsigned int); i++ ) {
			p.key = i;
			p.data1 = uib[i];
			addPair(p);
			//printf( "mrpair: %x\n", uib[i] );
		}
	}

	virtual Input4Byte *getPair() {
		if ( curPairIndex +1 >= loadedPairCount ) {
			char rbuffer[8192];
			loadedPairCount = 0;
			if ( feof(infile) ) {
				readFinished = true;
				printf( "Finished reading file\n" );
			} else {
				int res = fread(rbuffer, 1, sizeof(char)*8192, infile);
				if ( res == 0 ) {
					printf( "Finished reading file\n" );
					readFinished = true;
				} else if ( res != 8192 ) {
					fprintf(stderr, "File read only %d!!\n", res );
				} else {
					parse(rbuffer);
				}
			}
			curPairIndex = 0;
		}

		Input4Byte *p;
		if ( readFinished == false ) {
			p = &readBuffer[curPairIndex];
			curPairIndex ++;
		} else {
			p = new Input4Byte();
			p->done = true;
		}
		return p;
	}
};

// Start MapReduce implementation
class TestMR : public MapReduceApp {
	private:
	
	public:
	TestMR(NullInputParser* ni, int reducerCount) : MapReduceApp(ni, 1024, reducerCount) {};
	virtual void map (InputPair* p);
	virtual unsigned int keyHash(uint64_t key);
	virtual void reduce (Pair* p, Context* c);
	virtual void finalize (Context* ctx);
	virtual Context *createNewContext(uint64_t key_) {
		TestContext* nc = new TestContext(key_);
		return nc;
	}
};

unsigned int TestMR::keyHash(uint64_t key) {
	return (unsigned int) key;
}

void TestMR::map(InputPair* ip) {
	Input4Byte* p = (Input4Byte*) ip;
	InternalKVPair * np = new InternalKVPair();
	np->key = p->data1;
	np->data = 1;
	mapOutput(np);
}

void TestMR::reduce(Pair* ip, Context* ic) {
	InternalKVPair* p = (InternalKVPair*)ip;
	TestContext* c = (TestContext*)ic;

	c->count++;
}
void TestMR::finalize(Context* ictx) {
	TestContext* ctx = (TestContext*)ictx;
	
	if ( ctx->count > 4 ) {
		printf( "mr: key %lx value %d\n", ctx->key, ctx->count );
	}
}

double timespec_diff_sec( timespec start, timespec end ) {
	double t = end.tv_sec - start.tv_sec;
	t += ((double)(end.tv_nsec - start.tv_nsec)/1000000000L);
	return t;
}

int main() {
	int threadCount = 8;
	init_mrplatform(threadCount);
	printf( "Initialized MapReduce Platform\n" ); fflush(stdout);

	// Initialize components
	for ( int tidx = 0; tidx < threadCount; tidx++ ) {
		char filename[128];
		sprintf( filename, "bin/file%02d.dat", tidx );
		NullInputParser* ni = new NullInputParser(filename);
		TestMR* worker = new TestMR(ni, threadCount);
		
		add_mrworker(worker);
	}
	printf( "Added MapReduce Worker threads\n" );

	timespec start, end;
	
	clock_gettime(CLOCK_REALTIME, & start);

	spawn_threads();
	printf( "Waiting for MapReduce to finish...\n" );

	join_threads();
	clock_gettime(CLOCK_REALTIME, & end);
	printf( "MapReduce finished! %f\n", timespec_diff_sec(start, end) ); fflush(stdout);

	
	// Running golden implementation
	char rbuffer[8192];
	std::map<uint64_t, int> goldenMap;
	clock_gettime(CLOCK_REALTIME, & start);
	for ( int tidx = 0; tidx < threadCount; tidx++ ) {
		char filename[128];
		sprintf( filename, "bin/file%02d.dat", tidx );
		FILE* infile = fopen(filename, "r");
		while(!feof(infile)) {
			int rres = fread(rbuffer, 1, sizeof(char)*8192, infile);
			if ( rres != 8192 ) {
				//fprintf(stderr, "File read only %d!!\n", rres );
				continue;
			}
			
			unsigned int* uib = (unsigned int*)rbuffer;
			for ( int i = 0; i < 8192/sizeof(unsigned int); i++ ) {
				//p.data1 = uib[i];
				
				std::map<uint64_t, int>::iterator iter = goldenMap.find(uib[i]);
				if ( iter == goldenMap.end() ) { 
					goldenMap[uib[i]] = 1;
				}else {
					goldenMap[uib[i]]++;
				}
			}
		}
	}
	clock_gettime(CLOCK_REALTIME, & end);
	printf( "Golden finished! %f\n", timespec_diff_sec(start, end) ); fflush(stdout);
	
	std::map<uint64_t, int>::iterator iter;
	for ( iter = goldenMap.begin(); iter != goldenMap.end(); ++iter ) {
		if ( iter->second > 4 )
		printf( "golden: key %lx value %d\n", iter->first, iter->second );
	}

	return 0;
}
