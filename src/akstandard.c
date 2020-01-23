/*
This file is part of the aklib c library
Copyright 2016-2019 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "akstandard.h"

/*************************************************
Various random functions
*************************************************/


void ERROR(char *text, int errornum) {
  fprintf(stderr,"%s\n",text);
  exit(errornum);
}
void ERRORs(char *format, char *text, int errornum) {
  fprintf(stderr,format,text);
  exit(errornum);
}

// Concatenates up to 4 strings
static char *strconcat(char *s0, char *s1, char *s2, char *s3 ) {
  int i;
  int l[4], L;
  char *r, *t, *s[4];
  s[0]=s0; s[1]=s1; s[2]=s2; s[3]=s3;
  for (i=0,L=0; i<4; ++i) { if (s[i]) L+=l[i]=strlen(s[i]); else l[i]=0; }
  t = r = (char*)malloc((L+1)*sizeof(char));
  for (i=0,L=0; i<4; ++i) if (s[i]) while (*(s[i])) *(t++) = *(s[i]++);
  *t='\0';
  return r;
}

char *strconcat2(char *s0, char *s1) {return strconcat(s0,s1,NULL,NULL); }
char *strconcat3(char *s0, char *s1, char *s2) {return strconcat(s0,s1,s2,NULL); }
char *strconcat4(char *s0, char *s1, char *s2, char *s3) {return strconcat(s0,s1,s2,s3); }

/*
  Opens file nm.ext for reading (if ext==NULL it uses nm)
*/
FILE *open_file_read_ifexists(char *nm, char *ext) {
  FILE *fp;
  char *fn=nm;
  if (ext) fn = strconcat(nm,".",ext,NULL);
  fp = fopen(fn,"r");
  if (ext) free(fn);
  return fp;
}

/*
  Opens file nm.ext for reading (if ext==NULL it uses nm)
  If file does not exist, msg is printed first if given, then exit.
*/
FILE *open_file_read(char *nm, char *ext, char *msg) {
  FILE *fp;
  fp = open_file_read_ifexists(nm, ext);
  if (!fp) {
    if (msg) fprintf(stderr,"%s: ",msg);
    ERRORs("Couldn't open file %s for reading\n",nm,1);
  }
  return fp;
}

FILE *open_file_write(char *nm, char *ext, char *msg) {
  FILE *fp;
  char *fn=nm;

  if (ext) fn = strconcat(nm,".",ext,NULL);

  fp = fopen(fn,"w");
  if (!fp) {
    if (msg) fprintf(stderr,"%s: ",msg);
    ERRORs("Couldn't open file %s for writing\n",fn,1);
  }
  if (ext) free(fn);
  return fp;
}



/*************************************************
Look through a list of directories to find the dir for file named nm
Open file and return pointer
*************************************************/

int file_exists(char *filename) {
  struct stat buffer;   
  return ( stat (filename, &buffer) == 0 );
}

FILE *find_dir_open_file(List *paths, char *nm) {
  const int BSIZE=1024;
  int l;
  char buffer[BSIZE+3];
  char *path;

  resetList(paths);
  while ( (path=(char*)iterateList(paths)) ) {
    l = strlen(path);
    strncpy(buffer,path,BSIZE);
    if (buffer[l-1]!='/') buffer[(l++)-1]='/';
    strncpy(buffer+l,nm,BSIZE-l);
    if (file_exists(buffer)) {
      return open_file_read(buffer, NULL, "");
    }
  }

  fprintf(stderr,"File %s was not found in these locations:\n",nm);
  resetList(paths);
  while ( (path=(char*)iterateList(paths)) ) fprintf(stderr,"     %s\n",path);
  exit(2);
}


/*************************************************
Binary read and write of arrays
*************************************************/

void fwriteArray(void *a, int size, int len, FILE *fp) {
  int nbytes = len*size;
  fwrite((void*)(&nbytes),sizeof(int),1,fp);
  fwrite(a,1,nbytes,fp);
}

/* Number of items of size read is returned in *len
   Add nterm*size terminating bytes of 0 (normally =0 or =1 for strings)
*/
void *freadArray(int size, int *len, int nterm, FILE *fp) {
  char *a;
  int i, nbytes;
  int n=nterm*size;
  fread(&nbytes,sizeof(int),1,fp);
  if (len) {
    *len = nbytes/size;
    if ( *len*size != nbytes ) ERROR("freadArray: nbytes not divisable by size",1);
  }
  a = (char*)malloc(nbytes+n);
  fread((void*)a,1,nbytes,fp);
  for (i=0; i<n; ++i) a[nbytes+i]=0;
  return (void*)a;
}


/* Assumes that length of string is <256 - otherwise truncate */
void fwriteShortString(char *str, FILE *fp) {
  uchar ul;
  int l;
  l = strlen(str);
  if (l>255) l=255;
  ul = l;
  fwrite(&ul,sizeof(uchar),1,fp);
  fwrite(str,sizeof(char),l,fp);
}

char *freadShortString(FILE *fp) {
  uchar ul;
  int l;
  char *str;
  fread(&ul,sizeof(uchar),1,fp);
  l = ul;
  str = malloc((l+1)*sizeof(char));
  fread(str,sizeof(char),l,fp);
  str[l]='\0';
  return str;
}


/*************************************************
Reversing strings of chars
*************************************************/


// Works when source = destination (rev)
void reverseString(char *source, char *rev, long l) {
  long i=0;
  char a;

  // If length is uneven, middle letter is not touched
  --l;
  while (i<l) {
    a = source[i];
    rev[i++] = source[l];
    rev[l--] = a;
  }
  if (i==l) rev[i] = source[i];
}


// Use buffers to reverse seq from start1 into start2 and vice versa
static void reverse_chunks(char *buf1, char *buf2, int n, char *start1, char *start2) {
  int i,j;
  memcpy(buf1,start1,n);
  memcpy(buf2,start2,n);
  for (i=0, j=n; i<n; ++i) start1[i]=buf2[--j];
  for (i=0, j=n; i<n; ++i) start2[i]=buf1[--j];
}


// Use a buffer to speed up things
static void reverse_string_in_buffer(char *x, long l) {
  const int half_size=(1<<15);
  char *buffer=malloc(2*half_size);
  long i, k, i1, i2, nchunks, lhalf;

  lhalf=l/2;
  // Number of complete chunks:
  nchunks = lhalf/half_size;

  i1=0;
  i2=l-half_size;
  for (i=0; i<nchunks; ++i) {
    reverse_chunks(buffer,buffer+half_size,half_size,x+i1,x+i2);
    i1+=half_size;
    i2-=half_size;
  }

  // Do remaining sequence in the middle
  k=lhalf-i1;
  if (k>0) reverse_chunks(buffer,buffer+half_size,(int)k,x+i1,x+i2+half_size-k);
    
  free(buffer);
}



void reverseStringInplace(char *source, long l) {
  if ( l < (1<<16) ) reverseString(source, source, l);
  else reverse_string_in_buffer(source, l);
}


/*************************************************

Simple singly linked list implemented using stack jargon
Usage example:
  List *list = allocList();
  pushList(list,(void*)some_pointer);
  some_pointer = (pointer_type*)popList(list);
  freeList(list);

Keeps a pointer to first and last element, so you can push and
append

*************************************************/

static inline listEntry *alloc_listEntry(void *item, listEntry *next) {
  listEntry *entry=(listEntry*)malloc(sizeof(listEntry));
  entry->item = item;
  entry->next = next;
  return entry;
}

// Add element to beginning of linked list
void pushList(List *s, void *item) {
  s->entry=alloc_listEntry(item,s->entry);
  if (s->n==0) s->bottom=s->entry;
  s->n += 1;
}

// Append to the end of the linked list
void appendList(List *s, void *item) {
  if (s->n==0) s->bottom=s->entry=alloc_listEntry(item,NULL);
  else {
    s->bottom->next=alloc_listEntry(item,NULL);
    s->bottom=s->bottom->next;
  }
  s->n += 1;
}

// Get the first element of the list (last added with push, first with append)
void *popList(List *s) {
  listEntry *entry = s->entry;
  void *item;
  if (entry==NULL) return NULL;
  s->entry = entry->next;
  item = entry->item;
  free(entry);
  s->n -= 1;
  if (s->n==0) s->bottom=NULL;
  return item;
}

void reverseList(List *s) {
  listEntry *curr, *next;

  // First becomes last
  curr = s->bottom = s->entry;
  s->entry=NULL;

  // Now push on to the list from the start
  while (curr) {
    next = curr->next;
    curr->next = s->entry;
    s->entry = curr;
    curr = next;
  }
}

void *iterateList(List *s) {
  if (!s->curr) s->curr=s->entry;
  else s->curr=s->curr->next;
  if (s->curr) return s->curr->item;
  return NULL;
}

List *allocList() {
  List *r = (List *)malloc(sizeof(List));
  r->n=0;
  r->entry = r->bottom = r->curr = NULL;
  return r;
}

// You must free content of list before calling this
void freeList(List *s) {
  while (s->entry) popList(s);
  free(s);
}


/*
  Split a string on character c and return List with pointers to start
  of each string
  REPLACES c with 0 in original string!

  return NULL if result is empty.
  If skipEmpty!=0 it skips empty strings: If c=':', these string "a:bc:def"
  would give same result as ":a:::bc::def::"
  Otherwise the second string would include a lot of empty strings in the list

  NOTE: Tokens surrounded by "" are not interpreted, but the
        "'s are removed
 */
List *splitString(char *s, char c, int skipEmpty) {
  int stop=0, pling=0;
  List *r;
  char *ptr;

  if (!s) return NULL;
  if (strlen(s)<1) return NULL;

  r = allocList();
  ptr=s;
  while (!stop) {
    if (*ptr=='\0') stop=1;
    if (*ptr=='\"') {
      if (!pling) {pling=1; s+=1;}
      else { pling=0; *ptr='\0'; }
    }
    if ( (!pling && *ptr==c) || *ptr=='\0') {
      *ptr='\0';
      if ((int)(ptr-s)>0 || !skipEmpty) appendList(r,(void*)s);
      s=ptr+1;
    }
    ++ptr;
  }
  if (ListSize(r)==0) { freeList(r); r=NULL; }
  return r;
}


/*************************************************

Infinite strings

*************************************************/

static inline void alloc_current_iString(iString *is) {
  is->cptr = (char*)malloc(is->chunkSize*sizeof(char));
  is->last = is->cptr + is->chunkSize-1; 
  appendList(is->strings,(void*)(is->cptr));
}

iString *alloc_iString(int chunksize) {
  iString *is = (iString *)malloc(sizeof(iString));
  is->len=0;
  is->lastread=-10;
  is->chunkSize=chunksize;
  is->strings = allocList();
  alloc_current_iString(is);
  return is;
}


// This function resets an iString for re-use
void reuse_iString(iString *is) {
  while (ListSize(is->strings)) free(popList(is->strings));
  alloc_current_iString(is);
  is->len=0;
  is->lastread=-10;
}


void reset_iString(iString *is) {
  is->cptr=NULL;
  resetList(is->strings);
  is->cptr=(char *)peekList(is->strings);
}


void push_iString(iString *is) {
  *(is->cptr) = '\0';
  alloc_current_iString(is);
}

static inline void set_char_iString(iString *is, int c) {
  if ( is->len>0 && ++(is->cptr) >= is->last ) push_iString(is);
  *(is->cptr) = c;
  is->len += 1;
}

void append_char_iString(iString *is, int c) { set_char_iString(is,c); }


/*
  Read into an infinite string until end of line or until stopchar is
  reached. EOL, newline, or stopchar is NOT included

  iString must be allocated

  if include!=NULL, only include a char c if include[c]>0

  Continues reading from ->cptr is!=NULL

  ->lastread is set to last char read (EOF, stopchar or newline)

  Returns 0 on EOF

  Can be used like this to read until it meets '@':

  iString *istr=alloc_iString(100);
  while ( read_line_iString(fp, istr, '@', NULL) != 0 );

 */
int read_line_iString(FILE *fp, iString *is, int stopchar, char *include) {
  int c;

  if (include) {
    while ( (c=fgetc(fp)) ) {
      if ( c==stopchar || c=='\n' || c==EOF ) break;
      if (include[c]>0) set_char_iString(is, c);
    }
  }
  else {
    while ( (c=fgetc(fp)) ) {
      if ( c==stopchar || c=='\n' || c==EOF ) break;
      set_char_iString(is, c);
    }
  }

  // *(is->cptr) = '\0';
  // Terminator is lot counted in length
  // set_char_iString(is, '\0');
  // is->len -=1;

  is->lastread = c;

  if (c==EOF) return 0;
  else return 1;
}


/*
  Read until stopchar (across newlines) otherwise as above
*/
int read_iString(FILE *fp, iString *is, int stopchar, char *include) {
  int c;

  if (include) {
    while ( (c=fgetc(fp)) ) {
      if ( c==stopchar || c==EOF ) break;
      if (include[c]>0) set_char_iString(is, c);
    }
  }
  else {
    while ( (c=fgetc(fp)) ) {
      if ( c==stopchar || c==EOF ) break;
      set_char_iString(is, c);
    }
  }

  // *(is->cptr) = '\0';
  // Terminator is lot counted in length
  set_char_iString(is, '\0');
  is->len -=1;
  is->lastread = c;

  if (c==EOF) return 0;
  else return 1;
  
}

/*
  Read until stopchar is first char on a line (across newlines)
*/
int read_iString_until_startline(FILE *fp, iString *is, int stopchar, char *include) {
  int c;

  is->lastread = c = fgetc(fp);
  while ( c!=stopchar && c!=EOF ) {
    if (include[c]>0) set_char_iString(is, c);
    // Read rest of line
    if (c!='\n') c = read_line_iString(fp,is,0,include);
    if (!c) c = EOF;
    else is->lastread = c = fgetc(fp);
  }

  //*(is->cptr) = '\0';
  // Terminator is lot counted in length
  set_char_iString(is, '\0');
  is->len -=1;

  if (c==EOF) return 0;
  else return 1;
  
}



void free_iString(iString *is) {
  void *s;
  if (is->strings) {
    while ( (s = popList(is->strings)) ) { free(s); }
    freeList(is->strings);
  }
  free(is);
}



/* If *len>0 convert the first *len chars to a string of length
   len, else convert the whole iString
   If term!=0 append '\0'
 */
char *convert_iString_nondestruct(iString *is, int *len, int term) {
  char *str, *ss;
  int lleft, l, LEN=-1;

  reset_iString(is);
  if (!len) { len=&LEN; }
  if (*len<=0) *len = is->len;

  l = is->chunkSize-1;

  ss = popList(is->strings);
  if (term) {
    str = (char *)realloc(ss, (*len+1) * sizeof(char) );
    str[*len] = '\0';
  }
  else str = (char *)realloc(ss, *len * sizeof(char) );

  while (l<*len) {
    ss = popList(is->strings);
    lleft = *len-l;
    if (lleft >= is->chunkSize) lleft = is->chunkSize-1;
    strncpy(str+l,ss,lleft);
    l += lleft;
    free(ss);
  }

  return str;
}

// As above, but frees the iString
char *convert_iString(iString *is, int *len, int term) {
  char *str = convert_iString_nondestruct(is,len,term);
  free_iString(is);
  return str;
}


/*************************************************

Double-linked list (functions to be added...)

*************************************************/

static llistEntry *alloc_llistEntry(void *item, llistEntry *prev, llistEntry *next) {
  llistEntry *entry=(llistEntry*)malloc(sizeof(llistEntry));
  entry->item=item;
  entry->next=next;
  entry->prev=prev;
  return entry;
}

void appendLList(LList *ll, void *item) {
  if (ll->n) {
    ll->last->next=alloc_llistEntry(item,ll->last,NULL);
    ll->last = ll->last->next;
  }
  else {
    ll->first = ll->last = alloc_llistEntry(item,NULL,NULL);
  }
  ll->n += 1;
}
void pushLList(LList *ll, void *item) {
  if (ll->n) {
    ll->first->prev=alloc_llistEntry(item,NULL,ll->first);
    ll->first = ll->first->prev;
  }
  else {
    ll->first = ll->last = alloc_llistEntry(item,NULL,NULL);
  }
  ll->n += 1;
}

void *chopLList(LList *ll) {
  llistEntry *entry;
  void *item;
  if (ll->n==0) return NULL;
  entry=ll->last->prev;
  item = ll->last->item;
  free(ll->last);
  if (entry) { entry->next=NULL; ll->last=entry; }
  else ll->first = ll->last = entry;
  ll->n -= 1;
  return item;
}
void *popLList(LList *ll) {
  llistEntry *entry;
  void *item;
  if (ll->n==0) return NULL;
  entry=ll->first->next;
  item = ll->first->item;
  free(ll->first);
  if (entry) { entry->prev=NULL; ll->first=entry; }
  else ll->first = ll->last = entry;
  ll->n -= 1;
  return item;
}

// Iterate through linked list (from start). Use resetLList before calling
// Returns NULL at end (and starts over on next call).
// Do not insert or delete items while iterating with this function
void *iterateLList(LList *ll) {
  if (!ll->n) return NULL;
  if (ll->curr) ll->curr=ll->curr->next;
  else ll->curr=ll->first;
  if (ll->curr) return ll->curr->item;
  return NULL;
}

LList *allocLList() {
  LList *ll = (LList*)malloc(sizeof(LList));
  ll->n=0;
  ll->first = ll->last = ll->curr = NULL;
  return ll;
}

void freeLList(LList *ll) {
  while (ll->n) popLList(ll);
  free(ll);
}
