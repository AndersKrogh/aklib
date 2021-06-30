/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/


/*

  Functions for translating words (k-mers) in a sequence to numbers

  kmerSpecs holds info about the kmers, such as k, alphabet and the
  arrays used for translating kmers to numbers

 */




typedef struct {
  int alen;       // Alphabet length
  int wlen;       // Wordlen (k)
  int max_kmer;   // wlen^alen
  char *alphabet; // NULL or array of length alen+1
  char *reverseAlphabet;
  int **letterNumbers; // Used for calculating word number from word
} kmerSpecs;

static inline int kmerNumber(kmerSpecs *h, char *s) {
  int n, w = 0;
  for (n=0; n<h->wlen; ++n) w += h->letterNumbers[n][ (int)s[n] ];
  return w;
}

/*
  Returns the number of the next kmer in a sequence
  Assumes that n contains the number of the current kmer and that seq
  contains at least k+1 letters.
  The caller must make certain that these terms are met.
  Here is an example:
  int n=kmerNumber(h,seq);
  for (i=0; i<seqlen-h->wlen; ++i) n = kmerNextINsequence(h,seq+i,n);
 */
static inline int kmerNextINsequence(kmerSpecs *h, char *s, int n) {
  n -= h->letterNumbers[0][ (int)s[0] ];
  n *= h->alen;
  return n + h->letterNumbers[h->wlen-1][ (int)s[h->wlen] ];
}



/*
  Returns the number of the previous kmer in a sequence (reverse of the above)
  BUT s must point to the new position in sequence.
  Assumes that n contains the number of the current kmer

  Here is an example:
  int n=kmerNumber(h,seq+seqlen-h->wlen);
  for (i=seqlen-h->wlen-1; i>=0; --i) n = kmerPreviousINsequence(h,seq+i,n);
 */
static inline int kmerPreviousINsequence(kmerSpecs *h, char *s, int n) {
  n /= h->alen;
  return n + h->letterNumbers[0][ (int)s[0] ];
}


/*
  Replace letter i in word number n from old to new and return the number
  of the mutated word
 */
static inline int kmerReplaceLetter(kmerSpecs *h, int i, char old, char new, int n) {
  return n + h->letterNumbers[i][(int)new] - h->letterNumbers[i][(int)old];
}



kmerSpecs *alloc_kmerSpecs(int alen, int wlen, char *alphabet);
void free_kmerSpecs(kmerSpecs *h);
char *number2kmer(kmerSpecs *h, int n, char *w);
int nextKmer(kmerSpecs *h, char *s);
int nextKmerRev(kmerSpecs *h, char *s);

