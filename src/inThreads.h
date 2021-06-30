/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#ifndef INTHREAD_H
#define INTHREAD_H

#include <pthread.h>
#include <time.h>

#ifndef AKLIB_H
#include "akstandard.h"
#endif

/* 
   Main structure holding information about all threads and jobs
   This struct contains info shared by all workers
*/
typedef struct {
  int nthreads;
  int finished;
  int no_more_jobs;
  int sleep;         // Number of milliseconds to sleep
  List *inqueue;
  List *outqueue;
  List *running_jobs;
  // This function takes the thread number and a queue item and does the work
  int (*wfunc)(int, void*);
  // Mutexes - lock for accessing this structure
  pthread_mutex_t lock;
  pthread_t *worker;
} inThreads;


inThreads *init_inThreads(int nthreads, int (*wfunc)(int, void*) );
void new_job_inThreads(inThreads *threads, void* job);
void finished_jobqueue_inThreads(inThreads *threads);
void start_inThreads(inThreads *threads);
void *next_output_inThreads(inThreads *threads);
int done_inThreads(inThreads *threads);
int all_done_inThreads(inThreads *threads);
int jobs_waiting_inThreads(inThreads *threads);
int jobs_outqueue_inThreads(inThreads *threads);
void cleanup_inThreads(inThreads *threads);
void millisleep(int millisecs);
void print_status_inThreads(inThreads *threads, FILE *fp);

#endif
