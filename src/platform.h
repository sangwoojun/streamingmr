#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>

class InputPair {
public:
	bool done;
	uint64_t key;
};
class InputParser {
	public:
	InputParser() { }
	virtual InputPair *getPair() { }
	//bool done;
};

class Pair {
public:
	bool done;
	uint64_t key;
};

class Context {
public:
	Context (uint64_t key_) {key = key_;}
	uint64_t key;
};

class MapReduceApp
{
	protected:

	int intermediateBufferThresh;
	pthread_mutex_t mutex;


	public:
	int reducerCount;
	void lock() { pthread_mutex_lock(&mutex); }
	void unlock() { pthread_mutex_unlock(&mutex); }

	std::list<Pair*> intermediatePairs;
	std::list<Pair*> reduceInputPairs;
	InputParser* inputParser;
	bool readDone;
	MapReduceApp(InputParser* parser, int interBufferSize, int reducerCount_) {
		inputParser = parser;
		intermediateBufferThresh = interBufferSize;
		readDone = false;
		reducerCount = reducerCount_;

		pthread_mutex_init(&mutex, NULL);
	};
	virtual void map (InputPair* p) {};
	virtual void mapOutput(Pair *p);
	virtual unsigned int getKeyHash(uint64_t key) {return (unsigned int) key;};

	virtual void reduce(Pair* p, Context* c) {};
	virtual void finalize(Context* ctx) {};
	
	bool isIntermediateBufferReady() {
		if ( intermediatePairs.size() >= intermediateBufferThresh ) return true;
		if ( readDone ) return true;

		return false;
	}

	std::map<uint64_t, Context*> contextMap;
	bool ctxExists(uint64_t key) {
		std::map<uint64_t, Context*>::const_iterator it = contextMap.find(key);
		if ( it == contextMap.end() ) return false;
		return true;
	}
	virtual Context *createNewContext(uint64_t key_) {};
};

void init_mrplatform(int threadCount);
void add_mrworker(MapReduceApp* app);
void spawn_threads();
void join_threads();

#endif
