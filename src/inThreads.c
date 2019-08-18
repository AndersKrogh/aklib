/*
This file is part of the aklib c library
Copyright 2016-2019 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/


/*
  Tasks are queued for each type of processing (normally just one)
  A task is encapsulated in a struct including the result

  A number of threads are set aside for each process type and they pick
  tasks from the queue and move them from the queue to the end of another
  queue (to preserve order)

  When a task is done it is put last in another queue from which
  the master is taking it for finishing

  Simple example code for running inThreads:

  // Making 10 threads running "function"
  inThreads *threads = init_inThreads(10,function);

  // Put 100 jobs in the queue
  for (i=0; i<100; ++i) {
    job=something;
    new_job_inThreads(threads,(void*)job);
  }
  finished_jobqueue_inThreads(threads);

  // Start running. They stop when the job queue is empty
  start_inThreads(threads);

  // Checking for output
  while ( 1 ) {
    if ( (job = (jobType*)next_output_inThreads(threads)) ) {
      // Do what you have to do with job
    }
    else {
      // If all jobs are done, break
      if ( done_inThreads(threads) ) break;
      // Else wait for a while and check if there is a finished job
      else millisleep(10); // Sleep for 10 milliseconds
    }
  }
  cleanup_inThreads(threads);


*/

#include <stdlib.h>

#include "inThreads.h"


typedef struct {
  void *job;
  int done;
} JobStruct;


/* This struct contains info for individual workers (points to sharedStruct) */
typedef struct {
  int threadnumber;
  // Pointer to shared info (queues & mutex)
  inThreads *shared;
} workerStruct;



JobStruct *newJob(void *job) {
  JobStruct *js = (JobStruct*)malloc(sizeof(JobStruct));
  js->job=job;
  js->done=0;
  return js;
}


void new_job_inThreads(inThreads *threads, void* job) {
  pthread_mutex_lock(&(threads->lock));
  appendList(threads->inqueue,(void*)newJob(job));
  pthread_mutex_unlock(&(threads->lock));
}

void finished_jobqueue_inThreads(inThreads *threads) {
  pthread_mutex_lock(&(threads->lock));
  threads->no_more_jobs=1;
  pthread_mutex_unlock(&(threads->lock));
}

int jobs_waiting_inThreads(inThreads *threads) {
  int i;
  pthread_mutex_lock(&(threads->lock));
  i = ListSize(threads->inqueue);
  pthread_mutex_unlock(&(threads->lock));
  return i;
}

int jobs_outqueue_inThreads(inThreads *threads) {
  int i;
  pthread_mutex_lock(&(threads->lock));
  i = ListSize(threads->outqueue);
  pthread_mutex_unlock(&(threads->lock));
  return i;
}


void *next_output_inThreads(inThreads *threads) {
  void *x;
  pthread_mutex_lock(&(threads->lock));
  JobStruct *js = (JobStruct *)popList(threads->outqueue);
  pthread_mutex_unlock(&(threads->lock));
  if (js) {
    x = js->job;
    free(js);
    return x;
  }
  else return NULL;
}

// Have all jobs finished running?
int done_inThreads(inThreads *threads) {
  int i=0;
  pthread_mutex_lock(&(threads->lock));  
  i = threads->finished;
  pthread_mutex_unlock(&(threads->lock));
  return i;
}

// Are we completely done (all queues empty)
int all_done_inThreads(inThreads *threads) {
  int i=0;
  pthread_mutex_lock(&(threads->lock));
  if (threads->no_more_jobs && threads->finished && ListSize(threads->inqueue)==0
      && ListSize(threads->outqueue)==0) i=1;
  pthread_mutex_unlock(&(threads->lock));
  return i;
}

// Assumes that queues are empty and everything has finished!!
static void free_inThreads(inThreads *threads) {
  freeList(threads->inqueue);
  freeList(threads->outqueue);
  freeList(threads->running_jobs);
  if (threads->worker) free(threads->worker);
  free(threads);
}


// Calling function must lock mutex
static JobStruct *next_in_line(inThreads *threads) {
  return (JobStruct *)popList(threads->inqueue);;
}


inThreads *init_inThreads(int nthreads, int (*wfunc)(int, void*) ) {
  inThreads *s = (inThreads*)malloc(sizeof(inThreads));
  s->nthreads=nthreads;
  s->wfunc = wfunc;
  s->finished = 0;           // All threads are done
  s->no_more_jobs=0;         // If calling prog has finished adding new jobs
  s->sleep = 10;             // Sleep time in milliseconds
  s->inqueue = allocList();
  s->outqueue = allocList();
  s->running_jobs = allocList();
  pthread_mutex_init(&(s->lock), NULL);
  return s;
}


// Move jobs done from running to output
// Calling routine must lock mutex
static void flushJobs(inThreads *t) {
  JobStruct *x;
  // pthread_mutex_lock(&(t->lock));
  while ( 1 ) {
    x = (JobStruct*)peekList(t->running_jobs);
    if (x && x->done) {
      x = (JobStruct*)popList(t->running_jobs);
      appendList(t->outqueue,x);
    }
    else break;
  }
  // pthread_mutex_unlock(&(t->lock));
}

void millisleep(int millisecs) {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = millisecs*1000;
  nanosleep(&ts, NULL);
}


// Main worker function
void *workerFunc(void *xx) {
  workerStruct *w = (workerStruct*)xx;
  JobStruct *js;
  int keep_running = 1;

  while (keep_running) {
    // Pull a job from the inqueue (mutex on inqueue + running_jobs)
    pthread_mutex_lock(&(w->shared->lock));
    js = next_in_line(w->shared);
    if (js) {
      // Put it in the running_jobs queue
      appendList(w->shared->running_jobs,js);
      pthread_mutex_unlock(&(w->shared->lock));
      // Start function on the job
      w->shared->wfunc(w->threadnumber,js->job);
      // Mark job as done
      pthread_mutex_lock(&(w->shared->lock));
      js->done=1;
      // Flush the running jobs queue (moving jobs done to the outqueue)
      flushJobs(w->shared);
      pthread_mutex_unlock(&(w->shared->lock));
    }
    else {
      if ( w->shared->no_more_jobs || w->shared->finished ) {
	keep_running=0;
	pthread_mutex_unlock(&(w->shared->lock));
      }
      else {
	pthread_mutex_unlock(&(w->shared->lock));
	millisleep(w->shared->sleep);
      }
    }
  }

  pthread_mutex_lock(&(w->shared->lock));
  if (ListSize(w->shared->running_jobs)==0) w->shared->finished = 1;
  pthread_mutex_unlock(&(w->shared->lock));

  return NULL;
}


void start_inThreads(inThreads *threads) {
  int k;
  workerStruct *w;

  threads->worker = (pthread_t *)malloc(threads->nthreads*sizeof(pthread_t));

  for (k=0; k<threads->nthreads; ++k) {
    w = (workerStruct *)malloc(sizeof(workerStruct));
    w->shared = threads;
    w->threadnumber = k;
    pthread_create(&(threads->worker[k]), NULL, workerFunc, (void*)w);
  }

}

// This function is called when everything is done and the outqueue
// (and other queues) is empty
void cleanup_inThreads(inThreads *threads) {
  int k;

  /* Join workers */
  for (k=0; k<threads->nthreads; ++k) pthread_join(threads->worker[k], NULL);

  free_inThreads(threads);

}


void print_status_inThreads(inThreads *threads, FILE *fp) {
  pthread_mutex_lock(&(threads->lock));
  fprintf(fp,"nthreads %d\n",threads->nthreads);
  fprintf(fp,"inqueue %d\n",ListSize(threads->inqueue));
  fprintf(fp,"outqueue %d\n",ListSize(threads->outqueue));
  fprintf(fp,"running_jobs %d\n",ListSize(threads->running_jobs));
  fprintf(fp,"finished %d\n",threads->finished);
  fprintf(fp,"no_more_jobs %d\n",threads->no_more_jobs);
  fprintf(fp,"sleep %d\n",threads->sleep);
  pthread_mutex_unlock(&(threads->lock));
}
