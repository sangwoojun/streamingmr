#include "platform.h"

void MapReduceApp::mapOutput(Pair *pair) {
	this->lock();
	intermediatePairs.push_front(pair);
	this->unlock();
}


MapReduceApp **pMrApps;
void *mr_worker_thread( void *ptr ) {
	int idx = *(int*)(ptr);
	free( ptr );
	MapReduceApp *m = pMrApps[idx];
	printf( "Spawned thread %d\n", idx ); fflush(stdout);


	while ( !m->readDone ) {
		while ( !m->isIntermediateBufferReady() ) {
			InputPair *p = m->inputParser->getPair();

			if ( p->done ) {
				m->readDone = true;
			} else {
				m->map(p);
			}
		}
		
		//printf( "Thread %d finished a shuffle phase!\n", idx );

		while ( !m->intermediatePairs.empty() ) {
			m->lock();
			Pair* tp = m->intermediatePairs.back();
			m->intermediatePairs.pop_back();
			m->unlock();

			unsigned int key = m->getKeyHash(tp->key);
			int reducerid = key%(m->reducerCount);
			MapReduceApp *tm = pMrApps[reducerid];

			tm->lock();
			tm->reduceInputPairs.push_front(tp);
			tm->unlock();
		}

		//printf( "Thread %d finished a shuffle phase!\n", idx );

		while ( !m->reduceInputPairs.empty() ) {
			m->lock();
			Pair* tp = m->reduceInputPairs.back();
			m->reduceInputPairs.pop_back();
			m->unlock();
			
			Context *ctx;
			if ( !m->ctxExists(tp->key) ) {
				ctx = m->createNewContext(tp->key);
				m->contextMap[tp->key] = ctx;
			} else {
				ctx = m->contextMap[tp->key];
			}

			m->lock();
			m->reduce(tp, ctx);
			m->unlock();

			//m->contextMap[tp.key] = ctx; //ctx is a pointer, so this doesn't matter
		}
		//printf( "Thread %d finished a reduce phase!\n", idx );
	}
	std::map<uint64_t, Context*>::iterator iter;
	for ( iter = m->contextMap.begin(); iter != m->contextMap.end(); ++iter ) {
		m->finalize(iter->second);
	}
}

int totalThreadCount = 0;
int curThreadCount = 0;
pthread_t *aThreadId;
void init_mrplatform(int threadCount) {
	totalThreadCount = threadCount;
	pMrApps = (MapReduceApp**)malloc(sizeof(MapReduceApp*)*threadCount);
	aThreadId = (pthread_t*)malloc(sizeof(pthread_t)*threadCount);
}

void add_mrworker(MapReduceApp* app) {
	//for ( int tidx = 0; tidx < threadCount; tidx++ ) {

		//char filename[128];
		//sprintf( filename, "./file%02d.dat", tidx );
		//NullInputParser* ni = new NullInputParser(filename);
		int tidx = curThreadCount;
		curThreadCount++;

		pMrApps[tidx] = app;//new TestMR(ni, totalThreadCount);

}
void spawn_threads() {
	for ( int tidx = 0; tidx < totalThreadCount; tidx++ ) {
		pthread_t threadid;
		int* intval = (int*)malloc(sizeof(int));
		intval[0] = tidx;
	
		int tid = pthread_create( &threadid, NULL, mr_worker_thread, intval );
		aThreadId[tid] = threadid;
	}
}
void join_threads() {
	for ( int tidx = 0; tidx < totalThreadCount; tidx++ ) {
		pthread_join( aThreadId[tidx], NULL );
	}
}

