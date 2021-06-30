/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#include <stdio.h>

typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;

/* Bits
   Note that bits are numbered from 1 to 8 in these functions, so toggling
   first bit in z is achieved by toggleBit(z,1)
*/
#define toggleBit(flag, bitn) ( (flag) ^= (1<<(bitn-1)) )
#define setBit(flag, bitn)    ( (flag) |= (1<<(bitn-1)) )
#define clearBit(flag, bitn)  ( (flag) &= ~(1<<(bitn-1)) )
#define checkBit(flag, bitn)  ( ((flag)>>(bitn-1)) & 1 )
//static inline int checkBit(uchar flag, int bitn) { return (flag>>(bitn-1)) & 1; }

typedef struct _listItem_ { void *item; struct _listItem_ *next; } listEntry;
typedef struct { int n; listEntry *entry; listEntry *bottom; listEntry *curr; } List;


/* Various random functions */
void ERROR(char *text, int errornum);
void ERRORs(char *format, char *text, int errornum);
char *strconcat2(char *s0, char *s1);
char *strconcat3(char *s0, char *s1, char *s2);
char *strconcat4(char *s0, char *s1, char *s2, char *s3);
FILE *open_file_read_ifexists(char *nm, char *ext);
FILE *open_file_read(char *nm, char *ext, char *msg);
FILE *open_file_write(char *nm, char *ext, char *msg);
int file_exists(char *filename);
FILE *find_dir_open_file(List *paths, char *nm);

#define MINIMUM(a,b) ((a)<(b)?(a):(b))
#define MAXIMUM(a,b) ((a)>(b)?(a):(b))

void fwriteArray(void *a, int size, int len, FILE *fp);
void *freadArray(int size, int *len, int nterm, FILE *fp);
void fwriteShortString(char *str, FILE *fp);
char *freadShortString(FILE *fp);

void reverseString(char *source, char *rev, long l);
void reverseStringInplace(char *source, long l);


/* single linked list that acts like a stack
   You can only remove the head (using pop)
   but you can append to the tail (unlike a stack)
*/

void pushList(List *s, void *item);
void appendList(List *s, void *item);
// void insert_after_List(List *s, void *item);
// void delete_after_List(List *s, void *item);
void *popList(List *s);
void reverseList(List *s);
static inline int ListSize(List *s) { return s->n; }
static inline void *peekList(List *s) { if (s->entry) return s->entry->item; return NULL; }
static inline void resetList(List *s) { s->curr=NULL; }
void *iterateList(List *s);
List *allocList();
void freeList(List *s);

List *splitString(char *s, char c, int skipEmpty);

/* Infinite string (mostly for reading strings of unkown length) */
typedef struct {
  int lastread;    // last  character read
  int len;
  int chunkSize;
  List *strings;
  //  char *current;
  char *cptr;
  char *last;
} iString;

iString *alloc_iString(int chunksize);
int read_line_iString(FILE *fp, iString *is, int stopchar, char *include);
int read_iString(FILE *fp, iString *is, int stopchar, char *include);
int read_iString_until_startline(FILE *fp, iString *is, int stopchar, char *include);
char *convert_iString(iString *is, int *len, int term);
char *convert_iString_nondestruct(iString *is, int *len, int term);
void free_iString(iString *is);
void reuse_iString(iString *is);
void reset_iString(iString *is);
void push_iString(iString *is);
void append_char_iString(iString *is, int c);
static inline int length_iString(iString *is) { return is->len; }
static inline int last_read_iString(iString *is) { return is->lastread; }
static inline char *iterate_iString(iString *is) {
  if ( *(++(is->cptr))=='\0') is->cptr = (char*)iterateList(is->strings);
  return is->cptr;
}


/* Double linked list */

typedef struct _llistEntry_ {
  void *item;
  struct _llistEntry_ *next;
  struct _llistEntry_ *prev;
} llistEntry;

typedef struct {
  int n;
  llistEntry *first;
  llistEntry *last;
  llistEntry *curr;
} LList;

void appendLList(LList *ll, void *item);
void pushLList(LList *ll, void *item);
void *chopLList(LList *ll);
void *popLList(LList *ll);
static inline int LListSize(LList *ll) { return ll->n; }
static inline void resetLList(LList *ll) { ll->curr=NULL; }
void *iterateLList(LList *ll);
LList *allocLList();
void freeLList(LList *ll);

