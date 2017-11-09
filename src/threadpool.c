#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#include "threadpool.h"
#include "queue.h"

/**
 * Initializes a threadpool
 *
 * @param tp - pointer to the threadpool to initialize
 * @param pool_size - number of worker-threads in the pool
 * @param num_funcs - size of function-pointer array funcs
 * @param funcs - functions threadpool can operate upon
 *
 * @return - returns status of function. TODO: Actually do something with the return value
 **/
int threadpool_init(threadpool *tp, unsigned int pool_size, unsigned int num_funcs, p_func *funcs) {
	tp->num_threads = pool_size;
	tp->num_funcs = num_funcs;
	tp->funcs = funcs;
	sem_init(&(tp->sem), 0, pool_size);

	// TODO: Consider if there is a better data structure for tp->idle.
	// Order is trivial, so prioritize speed. Perhaps a stack might be a little more efficient
	// as it only requires tracking the number of elements in the strcture
	
	thread_details **temp;
	temp = malloc(sizeof(thread_details *) * pool_size);
	queue_init(&(tp->idle), temp, sizeof(thread_details *), pool_size);
	queue_d_init(&(tp->pending), sizeof(job_t), 4); //start with an initial size of 4 and see how that goes

	thread_details *td = malloc(sizeof(thread_details) * pool_size);
	job_loop_args *jla = malloc(sizeof(job_loop_args) * pool_size);
	tp->td = td;
	tp->jla = jla;

	for(unsigned int i = 0; i < pool_size; i++) {
		thread_details *ptd = &td[i];
		queue_enqueue(&(tp->idle), &ptd);
		td[i].job.func = NULL;
		td[i].job.args = NULL;
		td[i].sem = malloc(sizeof(sem_t)); //can this be moved outside the loop?
		sem_init(td[i].sem, 0, 0);
		jla[i].tp = tp;
		jla[i].td = &td[i];
		pthread_create(&(td[i].thread), NULL, &threadpool_job_loop, &jla[i]);
	}
	return 0;
}

/**
 * Free memory allocated on the heap from threadpool_init()
 *
 * @param tp = threadpool to be destroyed
 **/
void threadpool_destroy(threadpool *tp) {
	free(tp->idle.data);
	free(tp->pending.data);
	free(tp->jla);
	free(tp->td);
	// what about the thread details and job_loop_args?
}

/**
 * Worker threads will spawn here to do jobs as assigned.
 *
 * @param args - type struct job_loop_args
 **/
void *threadpool_job_loop(void *args) { //should I just give it job_loop_args type arg instead of the generic void* ?

	volatile const thread_details *td = ((job_loop_args *)args)->td;
	threadpool *tp = ((job_loop_args *)args)->tp;

	// TODO: consider conditions that should result in the death of a thread
	// Maybe even turn this into a pointer to a volatile arg that can be changed from outside the function
	bool should_exist = true;

	while (should_exist) {
		int err;
		do {
			err = sem_wait(td->sem);
		} while (err && errno == EINTR);
		td->job.func(td->job.args);

		if (queue_enqueue(&tp->idle, &td)) {
			perror("Thread &d failed to go into idle queue");
			should_exist = false;
		}
		sem_post(&(tp->sem));
	}
	return NULL;
}

/**
 * Add a job to the queue to be executed
 *
 * @param tp - pointer to the threadpool
 * @param func_index = The index of the function to be executed
 * @param func_args - arguments to be passed to the function
 **/
void threadpool_enqueue_job(threadpool *tp, unsigned int func_index, void *func_args) {
	job_t j;
	j.args = func_args;
	j.func = tp->funcs[func_index];
	queue_d_enqueue(&(tp->pending), &j);
}

/**
 * Assign pending job to an available thread
 *
 * @param tp - pointer to the threadpool
 **/
int threadpool_assign_job(threadpool *tp) {

	//wait for a thread to become available before assigning a job
	int err;
	do {
		err = sem_wait(&(tp->sem));
	} while (err && errno == EINTR);

	job_t j;
	thread_details *ptd;

	if(queue_d_dequeue(&(tp->pending), &j)) {
		return -1;
	}

	if(queue_dequeue(&(tp->idle), &ptd)) {
		return -1;
	}
	ptd->job = j;

	sem_post(ptd->sem);
	return 0;
}
