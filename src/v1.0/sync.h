#ifndef SYNC_H
#define SYNC_H

#include "utils.h"

class Sync {
public:
	void mux_init(pthread_mutex_t&);
	void cndvar_init(pthread_cond_t&);
	void mlock(pthread_mutex_t&);
	void munlock(pthread_mutex_t&);
	void cndwait(pthread_cond_t&, pthread_mutex_t&);
	void cndsignal(pthread_cond_t&);
};

extern Sync g_sync;

#endif /* SYNC_H */