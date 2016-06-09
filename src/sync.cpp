#include "sync.h"

Sync g_sync;

void Sync::mux_init(pthread_mutex_t &mux) {
	pthread_mutex_init(&mux, NULL);
}

void Sync::cndvar_init(pthread_cond_t &cndvar) {
	pthread_cond_init(&cndvar, NULL);
}

void Sync::mlock(pthread_mutex_t &mux) {
	int status;

	status = pthread_mutex_lock(&mux);
	g_utils.handle_type1_error(status, "Lock error: sync_mlock");
}

void Sync::munlock(pthread_mutex_t &mux) {
	int status;

	status = pthread_mutex_unlock(&mux);
	g_utils.handle_type1_error(status, "Unlock error: sync_munlock");
}

void Sync::cndwait(pthread_cond_t &cndvar, pthread_mutex_t &mux) {
	int status;

	status = pthread_cond_wait(&cndvar, &mux);
	g_utils.handle_type1_error(status, "Condition wait error: sync_cndwait");
}

void Sync::cndsignal(pthread_cond_t &cndvar) {
	int status;

	status = pthread_cond_signal(&cndvar);
	g_utils.handle_type1_error(status, "Condition signal error: sync_cndsignal");
}