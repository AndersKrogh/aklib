/*
This file is part of the aklib c library
Copyright 2016-2019 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/


/*
  Make a simple hash.
  You need to be able to estimate a reasonable size in advance.
  No resizing or anything like that.
  Very simple collision handling.

  You can store multiple items with same key - functions can handle this
  If you don't want this feature, use lookup function to check if key is
  already present before inserting new item

  A default hash function and comparison function is implemented, but you
  can also specify your own.
*/

// #define TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "simpleHash.h"


/********************************************************************

  A simple hash function

  Key is any type with a length (in bytes) that you give the function

  If key is longer than 16 bytes, a set of 16 bytes evenly distributed along the string
  are selected for the hash function to use. (for 32 bit machines it is probably 8 bytes)
  Default hash function is a simple random number generator. You can give another hash function

 ********************************************************************/

/* Simple random number generator from Num. recipies
   Returns a random number: 0<=number<max
   Seed can be negative
*/
static long _randomNumber_(long seed) {
  long k;
  const long c1=127773; /* IQ */
  const long c2=16807; /* IA */
  const long c3=2836; /* IR */
  const long max = (~(unsigned long)0)>>1;

  if (seed<0) seed += max;
  k = seed/c1;
  seed = c2*(seed-k*c1)-c3*k;
  if (seed<0) seed += max;
  // return seed % mod;
  return seed;
}



/* Take a sequence of len bytes and make a hash value (quick and dirty)
   If key is longer than 16 bytes, takes bytes more or less evenly spread

   If len=0, assumes that key is a string and uses strlen!!
*/
int simpleHash_value(const void *w, int len, const int hash_size) {
  const int N = sizeof(long);
  int N2 = N+N;
  long value[2] = {0,0};
  char *s = (char *)value, *ww=(char*)w;
  long retval;
  int i;

  if (len<=0) len=strlen((char*)w);

  /* Just insert chars in the number */
  if (len<=N2) for (i=0; i<len; ++i) s[i]=ww[i];
  /* Else, pick chars evenly spread from end */
  else for (i=0; i<N2; ++i)  s[i]=ww[len - ((len*i)/N2) -1];

  retval = _randomNumber_(value[0]);

  // If word longer than 16, divide number by 2 and add new random number
  if (len>N) retval = (retval>>1) + ( _randomNumber_(value[1])>>1 );

  return (int) (retval%hash_size);
}



/************************************************************

Making the hash

************************************************************/

/* Default keyCompare function. Wrapper for memcmp
   If l1<=0 it assumes that keys are strings and uses strcmp!!
*/
static int _keyCompare_(const void *s1, int l1, const void *s2, int l2) {
  if (l1<=0) return strcmp((char*)s1,(char*)s2);
  if (l1!=l2) return 1;
  return memcmp(s1,s2,l1);
}

// Initialize iterator
void simpleHash_init(simpleHash *hash) {
  hash->p=hash->beforep=NULL;
  hash->hashval=0;
}


/* Allocate hash
   If hfunc=NULL the simpleHash_value is used
*/
simpleHash *simpleHash_alloc(int hsize, int (*hfunc)(const void*, const int, const int) ) {
  simpleHash *ret = (simpleHash *)malloc(sizeof(simpleHash));

  ret->hsize=hsize;
  ret->nkeys=0;
  ret->collisions=0;
  ret->dkey=NULL;

  ret->tab = (hash_struct **)calloc(hsize,sizeof(hash_struct *));

  if (hfunc) ret->hfunc=hfunc;
  else ret->hfunc=simpleHash_value;
  ret->keyCompare = _keyCompare_;

  simpleHash_init(ret);

  ret->n0 = ret->n1 = -1;
  ret->z0 = ret->z1 = ret->zc = -1.;

  return ret;
}

// Change the keyCompare function
void simpleHash_keyCompare(simpleHash *h, int (*comp)(const void*, const int, const void*, const int) ) {
  h->keyCompare = comp;
}


/* Insert key and value into hash
   NOTE: Key is NOT duplicated
*/
int simpleHash_insert(void *key, int n, void *val, simpleHash *sh) {

  sh->hashval = sh->hfunc(key,n,sh->hsize);
  // fprintf(stderr,"h = %d hsize=%d\n",h,sh->hsize);
  sh->p = (hash_struct *)malloc(sizeof(hash_struct));
  sh->p->val=val;
  sh->p->key=key;
  sh->p->n = n;
  if (sh->tab[sh->hashval]) sh->collisions += 1;
  sh->p->next = sh->tab[sh->hashval];
  sh->tab[sh->hashval] = sh->p;
  sh->nkeys += 1;

  sh->beforep=NULL;

  return sh->hashval;
}


/* keys or values are NOT freed
   Manually delete content first
*/
void simpleHash_free(simpleHash *hash) {
  int i;
  hash_struct *p;
  if (hash->nkeys>0) {
    for (i=0; i<hash->hsize; ++i) {
      while (hash->tab[i]) {
	p=hash->tab[i]->next;
	free(hash->tab[i]);
	hash->tab[i]=p;
      }
    }
  }
  free(hash->tab);
  free(hash);
}



/* Looking up a hash entry, key x, length n
   returns NULL if unsuccessful
 */
static void *simpleHash_lookup_orig(void *x, int n, simpleHash *hash) {
  int h = hash->hfunc(x,n,hash->hsize);
  hash_struct *p;
  p = hash->tab[h];
  while (p) {
    if ( hash->keyCompare(x,n,p->key,p->n)==0 ) return p->val;
    p = p->next;
  }
  return NULL;
}



/* Looking up a hash entry. If there are more with the same key,
   this function return a new one on each call, if the key (x) is
   NULL.
   Returns NULL if unsuccessful (or list is exhausted)

   Adding entries between calls with x=NULL will mess it up!
 */
void *simpleHash_lookup(void *x, int n, simpleHash *hash) {

  if (x) {
    hash->hashval = hash->hfunc(x,n,hash->hsize);
    hash->p = hash->tab[hash->hashval];
    hash->beforep=NULL;
  }
  else {
    if (hash->p) {
      x = hash->p->key;
      n = hash->p->n;
      hash->beforep=hash->p;
      hash->p = hash->p->next;
    }
    else {
      hash->beforep=NULL;
      return NULL;
    }
  }
  while (hash->p) {
    if ( hash->keyCompare(x,n,hash->p->key,hash->p->n)==0 ) {
      return hash->p->val;
    }
    hash->beforep=hash->p;
    hash->p = hash->p->next;
  }
  hash->beforep=NULL;
  return NULL;
}




/* Delete last entry in hash
   To delete an entry, first use lookup on the key and then call this func to delete it
   Note that neither the key nor content are deleted
   Returns value

   Saves key in ->dkey
*/
void *simpleHash_delete(simpleHash *hash) {
  void *val=NULL;
  hash_struct *p;

  if (hash->p) {
    // fprintf(stderr,"Deleting %d %s\n",hash->hashval,(char*)hash->p->key);
    hash->nkeys -=1;
    p = hash->p;
    val = p->val;
    hash->dkey=p->key;
    hash->p = NULL;
    if (p->next || hash->beforep) hash->collisions -= 1;
    if (hash->beforep) hash->beforep->next=p->next;
    else hash->tab[hash->hashval]=p->next;
    free(p);
  }

  return val;
}



/* Iterate through all entries in hash
   Call simpleHash_init to initialize iterator

   Resturns value
   Returns NULL when hash is exhausted

   State is stored in hash (hashval, p, and beforep)
   When p=NULL, pos is used to set p
*/
void *simpleHash_next(simpleHash *hash) {

  // On first call it is initialized to hashval=0 and beforep=p=NULL

  // p is null either on first call or if last entry was deleted
  if (hash->p) {
    // fprintf(stderr,"p exists, choose next hval=%d\n",hash->hashval);
    hash->beforep = hash->p;
    hash->p = hash->p->next;
  }
  else {
    // fprintf(stderr,"p is NULL, choose value at hval=%d\n",hash->hashval);
    if (hash->beforep) hash->p=hash->beforep->next;  // after deletion
    else if ( hash->hashval < hash->hsize ) hash->p = hash->tab[hash->hashval];
  }

  // Next p will be first in linked list if p=NULL
  if (hash->p==NULL) hash->beforep=NULL;

  // Forward to first non-empty position
  while (hash->p==NULL && ++hash->hashval < hash->hsize ) hash->p = hash->tab[hash->hashval];

  // if p is still NULL, end is reached
  if ( hash->p == NULL ) {
    simpleHash_init(hash);
    return NULL;
  }

  return hash->p->val;
}


/* Print all keys - assuming they are strings */
void simpleHash_print_string_keys(simpleHash *hash, FILE *fp) {
  void *r;
  simpleHash_init(hash);
  while ((r=simpleHash_next(hash))) fprintf(fp,"%s ",(char*)(hash->p->key));
  fprintf(fp,"\n");
}


/*
  Calculate actual number of free slots (n0), slots with only one item (n1),
  the expected number of the same (z0, z1), and the expected number of
  collisions: zc = nkeys-(hsize-z0)

  z0 and z1 are from first two terms of binomial (n choose k)p^k(1-p)^(n-k)
  where p is 1/hsize and n=nkeys
*/
void simpleHash_diagnose(simpleHash *hash) {
  int i;
  double q;
  hash->n0=hash->n1=0;

  for (i=0; i<hash->hsize; ++i) {
    if (hash->tab[i]==NULL) ++hash->n0;
    else if (hash->tab[i]->next==NULL) ++hash->n1;
  }
  q = log(1.-1./hash->hsize);
  hash->z0 = hash->hsize * exp(hash->nkeys*q);
  hash->z1 = hash->hsize * exp((hash->nkeys-1)*q)*hash->nkeys/hash->hsize;
  hash->zc = hash->nkeys - (hash->hsize - hash->z0);
}


/* Print diagnostic values of Hash. Mostly for debugging
*/
void simpleHash_printStats(simpleHash *hash, FILE *fp) {

  simpleHash_diagnose(hash);

  fprintf(fp,"\nHash size: %d\nNumber of keys: %d\n", hash->hsize, hash->nkeys);
  fprintf(fp,"Free slots (actual, estimated): %d, %f\n", hash->n0, hash->z0);
  fprintf(fp,"Occupied by one (actual, estimated): %d, %f\n", hash->n1, hash->z1);
  fprintf(fp,"Slots with collisions (actual, estimated): %d, %f\n", hash->hsize-hash->n0-hash->n1,
	  hash->hsize - hash->z0 - hash->z1);
  fprintf(fp,"Collisions (actual, estimated): %d, %f\n", hash->collisions, hash->zc);
}

