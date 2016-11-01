#ifndef UTILS_H
#define UTILS_H
#define DEBUG 0
/* (C++) chrono: high_resolution_clock, microseconds */
#include <chrono>

/* (C++) cout, endl */
#include <iostream> 

/* (C) INT_MAX */
#include <limits.h>

/* (C) pthread_create, pthread_kill */
#include <pthread.h>

/* (C++) STL: queue */
#include <queue>

/* (C++) default_random_engine, exponential_distribution<T> */
#include <random>

/* (C) signal */
#include <signal.h>

/* (C) memset, memmove */
#include <stdio.h>

/* (C) strlen */
#include <string.h>

/* (C++) STL: string */
#include <string>

/* (C++) stringstream */
#include <sstream>

/* (C++) STL: thread */
#include <thread>

/* (C++) STL: unordered map */
#include <unordered_map>

/* (C++) STL: vector */
#include <vector>

/* datastore connection */
#include "KVStore.h"


using namespace std;
using namespace kvstore;


typedef std::chrono::high_resolution_clock CLOCK;
typedef std::chrono::microseconds MICROSECONDS;


#define MME "10.129.26.104"
//#define MME "10.129.28.33" //for other parallel replicas

#define SGW "10.129.28.191"
//#define SGW "10.129.28.224" //for other parallel replicas

#define PGW "10.129.28.147"
//#define PGW "10.129.26.101" //for other parallel replicas

#define MMELB "10.129.26.199"
#define SGWLB "10.129.26.200"
#define PGWLB "10.129.26.201"
#define INIT_VAL 100000000   // to set with different init value for other mme replicas e.g. next replica could start with 700000000

#define TRACE(x) if (DEBUG) { x }

const int MAX_UE_COUNT = 10000;


class Utils {
public:
	void handle_type1_error(int, string);
	void handle_type2_error(int, string);
	char* allocate_str_mem(int);
	uint8_t* allocate_uint8_mem(int);
	void time_check(time_t, double, bool&);
	int max_ele(vector<int> inp);
};

extern Utils g_utils;

#endif /* UTILS_H */
