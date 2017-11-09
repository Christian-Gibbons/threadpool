#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>
#include <semaphore.h>

#include "queue.h"

typedef int (*p_func)(void *);

typedef struct {
	void *args;
	p_func func;
} job_t;

typedef struct {
	job_t job;
	sem_t *sem;
	pthread_t thread;
} thread_details;

typedef struct job_loop_args job_loop_args;

typedef struct {
	unsigned int num_threads;
	unsigned int num_funcs;
	p_func *funcs;
	sem_t sem;
	queue idle;
	queue pending;
	thread_details *td;
	struct job_loop_args *jla;
} threadpool;

struct job_loop_args{
	threadpool *tp;
	thread_details *td;
};

int threadpool_init(threadpool *tp, unsigned int pool_size, unsigned int num_funcs, p_func *funcs);
static void *threadpool_job_loop(void *args);
void threadpool_enqueue_job(threadpool *tp, unsigned int func_index, void *func_args);
int threadpool_assign_job(threadpool *tp);

void threadpool_destroy(threadpool *tp);

#endif
