/*
This file is part of the aklib c library
Copyright 2016-2019 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "kmers.h"

/*
   letterNumbers[i][c] = c*alen^(wlen-i-1)
   if alphabet is given, the table is made for character strings
*/
static void make_letterNumbers(kmerSpecs *h) { // int alen, int wlen, char *alphabet) {
  int c, power=1, i=h->wlen, arraylen;
  int **letter_numbers = (int **)malloc(h->wlen*sizeof(int *));
  char *alphabet = h->alphabet;

  if (alphabet) arraylen = 128;
  else arraylen=h->alen;
  
  while ( i-- >0 ) {
    letter_numbers[i] = (int *)calloc(arraylen,sizeof(int));
    if (alphabet) {
      for (c=0; c<arraylen; ++c) letter_numbers[i][c]=-1;
      for (c=0; c<h->alen; ++c) letter_numbers[i][(int)alphabet[(int)(c)]] = c*power;
    }
    else for ( c=0; c<h->alen; ++c) letter_numbers[i][c] = c*power;
    power *= h->alen;
  }
  h->letterNumbers = letter_numbers;
  h->max_kmer=power;
}


kmerSpecs *alloc_kmerSpecs(int alen, int wlen, char *alphabet) {
  int i;
  kmerSpecs *r = (kmerSpecs*)malloc(sizeof(kmerSpecs));
  r->alen = alen;
  r->wlen = wlen;
  if (alphabet) {
    r->alphabet = strndup(alphabet, alen+1);
    r->reverseAlphabet=(char*)malloc(128);
    for (i=0; i<alen; ++i) r->reverseAlphabet[(int)alphabet[i]]=i;    
  }
  else r->alphabet = NULL;
  // r->powers = make_powers(alen,wlen,alphabet);
  make_letterNumbers(r);
  return r;
}


void free_kmerSpecs(kmerSpecs *h) {
  int i;
  if (h->alphabet) {
    free(h->alphabet);
    free(h->reverseAlphabet);
  }
  for (i=0; i<h->wlen; ++i) free(h->letterNumbers[i]);
  free(h->letterNumbers);
}



/*
   If *w is given it must point to enough allocated space.
   If alphabet is given, word is converted and terminated by 0
*/
char *number2kmer(kmerSpecs *h, int n, char *w) {
  int l=h->wlen;
  if (!w) { w = malloc(h->wlen+1); w[l]=0; }
  while ( l-- > 0 ) { w[l] = n%h->alen; n /= h->alen; }
  if (h->alphabet) {
    for (l=0; l<h->wlen; ++l) w[l]=h->alphabet[(int)w[l]];
    w[l]='\0';
  }
  return w;
}

/*
  Iterate through words (k-mers)
  e.g. for alphabet ABC and k=4: AAAA, AAAB, AAAC, AABA, AABB, AABC, AACA, ....
  Returns 0 after the last k-mer
  May be started with
     w = number2kmer(h, 0, NULL)
  which returns the 0th word

  Example:
  h = alloc_kmerSpecs(alen, wlen, alphabet);
  char *w = number2kmer(h, 0, NULL);
  do { this_and_that(w); } while (nextKmer(h,w));
 */
int nextKmer(kmerSpecs *h, char *s) {
  int i, k=h->wlen;
  if (h->alphabet) {
    while (k-->0) {
      i = 1 + h->reverseAlphabet[(int)s[k]];
      if ( i>=h->alen ) s[k]=h->alphabet[0];
      else {
	s[k]=h->alphabet[i];
	break;
      }
    }
    if ( k==-1 && s[0]==h->alphabet[0]) return 0;
  }
  else {
    while (k-->0) { if ( ++s[k]>=h->alen ) s[k]=0; else break; }
    if ( k==-1 && s[0]==0 ) return 0;
  }
  return 1;
}



/*
  Iterate through words in reverse order: AAAA, BAAA, CAAA, ABAA, BBAA, etc
  returns the next word. May be started with
     w = number2kmer(h, 0, NULL)
  which returns the 0th word
 */
int nextKmerRev(kmerSpecs *h, char *s) {
  int i, k;
  if (h->alphabet) {
    for (k=0; k<h->wlen; ++k) {
      i = 1 + h->reverseAlphabet[s[k]];
      if ( i>=h->alen ) s[k]=h->alphabet[0];
      else {
	s[k]=h->alphabet[i];
	break;
      }
    }
    if ( k==h->wlen && s[h->wlen-1]==h->alphabet[0]) return 0;
  }
  else {
    for (k=0; k<h->wlen; ++k) { if ( ++s[k]>=h->alen ) s[k]=0; else break; }
    if ( k==h->wlen && s[h->wlen-1]==0) return 0;
  }
  return 1;
}
