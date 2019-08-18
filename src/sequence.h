/*
This file is part of the aklib c library
Copyright 2016-2019 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/



typedef struct __SEQstruct__ {
  char *id;
  char *descr;     // Description (stuff following id). Allocated with id.
  uchar flag;     // flag for allocation status and reverse complement
  long len;
  long pos;        // Index of sequence in long allocation
  char *s;         // Sequence
  char *q;         // Pointer to secondary sequence - e.g. qual scores (if any)
  char *lab;       // Pointer to labels (if any)
  int sort_order;

  struct __SEQstruct__ *next;  /* For a single linked list used when reading */
} Sequence;


typedef struct {
  int len;           // Alphabet length
  ushort flag;       // Flags. See values later
  char *a;           // Alphabet sequence (0 terminated, first char is terminator, last may be wildcard)
  char *trans;       // Translate char c to int i: trans[c]=i (this points to one of the two below)
  char *sensitivetrans; // Translate char c to int i case sensitive
  char *insenstrans; // Translate char c to int i case insensitive
  char *comp;        // DNA complement alphabet comp[a]=t, etc.
  char *compTrans;   // Table to complement a sequence which is already turned into numbers
  char *gCode;       // String for genetic code
} AlphabetStruct;


// Structure containing stuff for reading the single line format
typedef struct {
  int separator;
  int id_field;
  int seq_field;
  int q_field;
  int lab_field;
  int max_field;
  AlphabetStruct *seq_alph;
  AlphabetStruct *q_alph;
  AlphabetStruct *lab_alph;
  iString *desc;
  iString **strings;
} singleLineStruct;


// Bits for the seq->flag
static const uchar seq_flag_rev=1;
static const uchar seq_flag_comp=2;

// Flags to track if id, descr, seq and lab are allocated (and should be freed)
static const uchar seq_flag_id=3;
static const uchar seq_flag_descr=4;
static const uchar seq_flag_seq=5;
static const uchar seq_flag_lab=6;
static const uchar seq_flag_q=7;

// Alphabet flags
#define AS_wildcard 1
#define AS_term 2
#define AS_protein 3
#define AS_DNA 4
#define AS_RNA 5
#define AS_IUPAC 6       // If set, DNA is also set
#define AS_revcomp 7
#define AS_stopcodon 8
#define AS_casesens 9
#define AS_variants 10

#define UONE ((ushort)1)

static inline int AlphabetStruct_test_flag(AlphabetStruct *alph, int bit) { return (int)( (alph->flag) & (UONE<<(bit-1)) ); }

static inline int letter2number(char c, AlphabetStruct *a) { return a->trans[(int)c];}
static inline char number2letter(int i, AlphabetStruct *a) { return a->a[i];}

/* Change index i to reverse (complement) sequence
   len = total sequence length
   mlen = match length (set to zero or 1 if not relevant)
 */
static inline long reverse_coordinate_base0(long len, long i, long mlen) {
  if (mlen==0) mlen=1;
  return len-i-mlen;
}
static inline long reverse_coordinate_base1(long len, long i, long mlen) {
  if (mlen==0) mlen=1;
  return len-i+2-mlen;
}


/* FUNCTION PROTOTYPES BEGIN  ( by funcprototypes.pl ) */
Sequence *alloc_Sequence();
void free_Sequence(Sequence *ss);
void case_sensitive_alphabet(AlphabetStruct *astruct);
void case_insensitive_alphabet(AlphabetStruct *astruct);
AlphabetStruct *alloc_AlphabetStruct(char *a, int caseSens, int revcomp, char term, int wildcard);
void write_AlphabetStruct(AlphabetStruct *alph, FILE *fp);
AlphabetStruct *read_AlphabetStruct(FILE *fp);
AlphabetStruct *bio_AlphabetStruct(char *a);
void free_AlphabetStruct(AlphabetStruct *astruct);
void print_AlphabetStruct(AlphabetStruct *a, FILE *fp);
void translate2numbers(char *s, const long slen, AlphabetStruct *astruct);
Sequence *make_Sequence(char *letters, char *id, AlphabetStruct *alphabet);
int ReadSequenceFileHeader(FILE *fp, int type);
Sequence *readFasta(FILE *fp, AlphabetStruct *alph, int read_size, int save_descr, char *eof);
Sequence *readFastq(FILE *fp, AlphabetStruct *seq_alph, AlphabetStruct *qual_alph, int read_size, int save_descr, char *eof);
singleLineStruct *make_singleLineStruct(int separator, int id_field, int seq_field, int lab_field, int q_field,
					AlphabetStruct *seq_alph, AlphabetStruct *lab_alph, AlphabetStruct *q_alph,
					char *format);
void free_singleLineStruct(singleLineStruct *s);
Sequence *readSingleLineFormat(FILE *fp, singleLineStruct *sls, int read_size, char *eof);
void reverseSequence(Sequence *seq);
void revcompSequence(Sequence *seq, AlphabetStruct *astruct);
void printSeqRaw(FILE *file, char *s, int seqlen, char *alphabet, int from, int printlen);
void printSeqRawReverse(FILE *file, char *s, int seqlen, char *alphabet, int from, int printlen);
void printSeqOneLine(FILE *file, Sequence *seq, char *alphabet);
void printFasta(FILE *file, Sequence *seq, char *alphabet, int linelen);
void makeGeneticCode(AlphabetStruct *alph, AlphabetStruct *prot_alph);
char *translateDNA(Sequence *seq, AlphabetStruct *alph);
/* FUNCTION PROTOTYPES END */


