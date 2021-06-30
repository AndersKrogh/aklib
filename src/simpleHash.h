/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#ifndef SIMPLEHASH_h
#define SIMPLEHASH_h

typedef struct _HASH_STRUCT {
  void *val;
  void *key;
  int n;       // Key len
  struct _HASH_STRUCT *next;
} hash_struct;

typedef struct {
  int hsize;
  int nkeys;
  int collisions;
  int hashval;             // Hash value of last lookup
  int (*hfunc)(const void*, int, const int);
  int (*keyCompare)(const void*, int, const void*, int);
  hash_struct **tab;
  hash_struct *p;          // Points to the latest element
  hash_struct *beforep;    // element pointing to last item, if there is one before
  void *dkey;              // Last key that was deleted
  // Diagnose terms
  int n0;
  int n1;
  double z0;
  double z1;
  double zc;
} simpleHash;

static inline void *simpleHash_del_key(simpleHash *h) { return h->dkey; }
static inline void *simpleHash_get_key(simpleHash *h) { if (h->p) return h->p->key; else return NULL; }
static inline void *simpleHash_get_val(simpleHash *h) { if (h->p) return h->p->val; else return NULL; }


/* FUNCTION PROTOTYPES BEGIN  ( by funcprototypes.pl ) */
int simpleHash_value(const void *w, int len, const int hash_size);
void simpleHash_init(simpleHash *hash);
simpleHash *simpleHash_alloc(int hsize, int (*hfunc)(const void*, const int, const int) );
void simpleHash_keyCompare(simpleHash *h, int (*comp)(const void*, const int, const void*, const int) );
int simpleHash_insert(void *key, int n, void *val, simpleHash *sh);
void simpleHash_free(simpleHash *hash);
void *simpleHash_lookup(void *x, int n, simpleHash *hash);
void *simpleHash_delete(simpleHash *hash);
void *simpleHash_next(simpleHash *hash);
void simpleHash_print_string_keys(simpleHash *hash, FILE *fp);
void simpleHash_diagnose(simpleHash *hash);
void simpleHash_printStats(simpleHash *hash, FILE *fp);
/* FUNCTION PROTOTYPES END */


// macroes for simple string hashes
#define stringHash_init simpleHash_init
#define stringHash_free simpleHash_free
#define stringHash_delete simpleHash_delete
static inline simpleHash *stringHash_alloc(int hsize) { return simpleHash_alloc(hsize, NULL); }
static inline int stringHash_insert(char *key, void *val, simpleHash *sh) {
  return simpleHash_insert((void *)key, 0, val, sh);
}
static inline void *stringHash_lookup(char *x, simpleHash *hash) {
  return simpleHash_lookup((void *)x, 0, hash);
}


#endif
