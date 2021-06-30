/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "akstandard.h"
#include "sequence.h"


Sequence *alloc_Sequence() {
  Sequence *ss=(Sequence *)malloc(sizeof(Sequence));
  ss->len = 0;
  ss->flag = 0;
  ss->pos = 0;
  ss->id = NULL;
  ss->descr = NULL;
  ss->s = NULL;
  ss->lab=NULL;
  ss->q=NULL;
  ss->sort_order = 0;
  ss->next=NULL;
  return ss;
}

void free_Sequence(Sequence *ss) {
  if (ss) {
    if (ss->id && checkBit(ss->flag,seq_flag_id) ) free(ss->id);
    if (ss->descr && checkBit(ss->flag,seq_flag_descr) ) free(ss->descr);
    if (ss->s && checkBit(ss->flag,seq_flag_seq) ) free(ss->s);
    if (ss->lab && checkBit(ss->flag,seq_flag_lab) ) free(ss->lab);
    if (ss->q && checkBit(ss->flag,seq_flag_q) ) free(ss->q);
    free(ss);
  }
}



/* Makes a translation table from an alphabet to a translation, so
        table[alphabet[i]] = translation[i]

   If translation==NULL, the letter alphabet[i] is translated to i.

   Letters not in alphabet are translated to dummy.
   If dummy==0, dummy is set to the translation of the last char in alphabet
   (assumed to be a "wildcard" character)

   Translation for non-characters (!ischar) is -1, but you can have any printable
   char in the alphabet, in which case it is translated normally.

   casesens !=0, means case sensitive, otherwise case insentitive.

   Returns an array (char *table) of length 128

   The length of the translation table (which may contain zeros)
   HAS to be as long (or longer) than alphabet.

   If 0 is not in the alphabet, it is translated to 0
*/
static char *translation_table(char *alphabet, char *translation, char dummy, int casesens) {
  int i, l, freetrans=0;
  char *table = (char*)malloc(128*sizeof(char));

  l = strlen(alphabet);

  if (translation==0) {
    translation = (char *)malloc(l*sizeof(char));
    for (i=0; i<l; ++i) translation[i]=i;
    freetrans=1;
  }
  if (dummy==0) dummy = translation[l-1];

  table[0]=0;
  for (i=1; i<128; ++i) { table[i]=-1; if (isprint((char)i)) table[i]=dummy; }

  if (!casesens) {
    for (i=0; i<l; ++i) {
      if (table[toupper(alphabet[i])]==dummy) { // If translation already set, don't change
	table[toupper(alphabet[i])]=translation[i];
	table[tolower(alphabet[i])]=translation[i];
      }
    }
  }
  else {
    for (i=0; i<l; ++i) table[(int)alphabet[i]] = translation[i];
  }

  if (freetrans) free(translation);

  return table;
}


/*
  Returns a string containing the reverse complement of an alphabet
  Uses IUPAC characters

R [AG]   Y [CT]
S [GC]   S
W [AT]   W
K [GT]   M [AC]
B [CGT]  V [ACG]
D [AGT]  H [ACT]
N [ACGT] N

 */
static char *dnaComplement(char *alphabet, int RNA) {
  int l=strlen(alphabet);
  char *comp=(char *)malloc((l+1)*sizeof(char));
  int i;
  char zz[128];
  int T='T', t='t';

  // Check for RNA alphabet
  if (RNA) { T='U'; t='u'; }
  
  for (i=0;i<128;++i) zz[i]=i;

  zz['a'] = t; zz['A'] = T;
  zz['c'] = 'g'; zz['C'] = 'G';
  zz['g'] = 'c'; zz['G'] = 'C';
  zz[t] = 'a'; zz[T] = 'A';

  zz['r'] = 'y'; zz['R'] = 'Y';
  zz['y'] = 'r'; zz['Y'] = 'R';
  zz['s'] = 's'; zz['S'] = 'S';
  zz['w'] = 'w'; zz['W'] = 'W';
  zz['k'] = 'm'; zz['K'] = 'M';
  zz['m'] = 'k'; zz['M'] = 'K';
  zz['b'] = 'v'; zz['B'] = 'V';
  zz['d'] = 'h'; zz['D'] = 'H';
  zz['h'] = 'd'; zz['H'] = 'D';
  zz['v'] = 'b'; zz['V'] = 'B';
  zz['n'] = 'n'; zz['N'] = 'N';

  for (i=0;i<l;++i) { comp[i] = zz[(int)alphabet[i]]; }
 
  comp[i]=0;
  return comp;
}


/*
  Order of bits:
 */

static void AlphabetStruct_set_flag(AlphabetStruct *alph, int bit) { alph->flag |= UONE<<(bit-1); }
static void AlphabetStruct_clear_flag(AlphabetStruct *alph, int bit) { alph->flag &= ~( UONE<<(bit-1) ); }



void case_sensitive_alphabet(AlphabetStruct *astruct) {
  astruct->trans = astruct->sensitivetrans;
  AlphabetStruct_set_flag(astruct,AS_casesens);
}


void case_insensitive_alphabet(AlphabetStruct *astruct) {
  astruct->trans = astruct->insenstrans;
  AlphabetStruct_clear_flag(astruct,AS_casesens);
}


/*
  Based on flags AS_casesens and AS_revcomp this function makes the appropriate
  tanslation table and complement table
 */
static inline void set_AlphabetStruct(AlphabetStruct *astruct) {
  int i, RNA=0;
  int wildcard=-1;

  if (AlphabetStruct_test_flag(astruct,AS_wildcard)) wildcard = astruct->len-1;
  if (AlphabetStruct_test_flag(astruct,AS_RNA)) RNA=1;

  astruct->sensitivetrans = translation_table(astruct->a, NULL, wildcard, 1);
  astruct->insenstrans = translation_table(astruct->a, NULL, wildcard, 0);
  if (AlphabetStruct_test_flag(astruct,AS_casesens)) astruct->trans = astruct->sensitivetrans;
  else astruct->trans = astruct->insenstrans;

  if (AlphabetStruct_test_flag(astruct,AS_revcomp)) {
    astruct->comp = dnaComplement(astruct->a,RNA);
    astruct->compTrans = (char*)malloc( astruct->len*sizeof(char) );
    for (i=0;i<astruct->len;++i) astruct->compTrans[i] = astruct->trans[(int)astruct->comp[i]];
  }
}


/*
  If terminal symbol is given, it is put first in alphabet (number 0)
  Otherwise the first char in alphabet has number 0

  if (a==NULL) it returns a blank struct
 */
static AlphabetStruct *alloc_AlphabetStruct_raw(char *a, char term) {
  AlphabetStruct *astruct = (AlphabetStruct *)malloc(sizeof(AlphabetStruct));

  astruct->len = 0;      
  astruct->flag = 0;
  astruct->a = NULL;
  astruct->trans = NULL;
  astruct->sensitivetrans = NULL;
  astruct->insenstrans = NULL;
  astruct->comp = NULL;
  astruct->compTrans = NULL;
  astruct->gCode = NULL;

  if (a==NULL) return astruct;

  /* The alphabet string is constructed with the terminator first */
  if (term) {
    astruct->a = (char*)malloc((strlen(a)+2)*sizeof(char));
    *(astruct->a)=term;
    strcpy(astruct->a+1,a);
    AlphabetStruct_set_flag(astruct,AS_term);
  }
  else astruct->a = strdup(a);
  astruct->len = strlen(astruct->a);

  return astruct;
}


/*
  If terminal symbol is given, it is put first in alphabet (number 0)
  Otherwise the first char in alphabet has number 0

  if (a==NULL) it returns a blank struct
 */
AlphabetStruct *alloc_AlphabetStruct(char *a, int caseSens, int revcomp, char term, int wildcard) {
  //AlphabetStruct *astruct = (AlphabetStruct *)malloc(sizeof(AlphabetStruct));
  AlphabetStruct *astruct = alloc_AlphabetStruct_raw(a, term);

  if (a==NULL) return astruct;

  if (caseSens) AlphabetStruct_set_flag(astruct,AS_casesens);
  if (revcomp) AlphabetStruct_set_flag(astruct,AS_revcomp);
  if (wildcard) AlphabetStruct_set_flag(astruct,AS_wildcard);
  
  set_AlphabetStruct(astruct);

  return astruct;
}


void write_AlphabetStruct(AlphabetStruct *alph, FILE *fp) {
  fwrite(&(alph->len),sizeof(int),1,fp);
  fwrite(alph->a,sizeof(char),alph->len+1,fp);
  fwrite(&(alph->flag),sizeof(ushort),1,fp);
}



AlphabetStruct *read_AlphabetStruct(FILE *fp) {
  AlphabetStruct *alph = alloc_AlphabetStruct(NULL, 0, 0, 0, 0);
  fread(&(alph->len),sizeof(int),1,fp);
  alph->a = (char*)calloc(alph->len+1,sizeof(char));
  fread(alph->a,sizeof(char),alph->len+1,fp);
  fread(&(alph->flag),sizeof(ushort),1,fp);
  set_AlphabetStruct(alph);
  return alph;
}


// Only low-case those that have a lower-case version
static void change_case_string(char *s) {
  char *r=s;
  while (*s) {
    if (isupper(*s)) *(r++) = tolower(*s);
    else if (islower(*s)) *(r++) = toupper(*s);
    ++s;
  }
  *r='\0';
}

/*
     You can specify an alphabet like this:
         <string>(/qualifier)*
     where <string> is
         * One of the words protein, DNA, UIPAC, or RNA
         * a string of chars in the alphabet
         - in the string a range can be like "[A-Z]"

     The qualifiers are one of
         "casesens": Alphabet is case sensitive
         "wildcard": Add 'X' to protein alphabet or 'N' to DNA/RNA alphabet
         "stopcodon": If add '$' to alphabet and with translation of standard stop codons to '$'
         "variants": Add '|' to alphabet for encoding variants (used with DNA or IUPAC)
         - the qualifiers can be shortened (e.g. :w or :stop)
     examples:
         "DNA/w/variants" gives alphabet ACGT|N
         "[A-C]/c" gives alphabet ABCabc

     IUPAC is special, because /casesens will only make lower-case ACGTN
 */
static char *interpret_alphabet_specs(char *a, ushort *flags) {
  char *str, orig[256], newa[256];
  int i, k, l, n;

  // Split alphabet and qualifiers
  strcpy(orig, a);
  n = strlen(orig);

  // Do qualifiers
  str=orig+n;
  while (str>orig) {
    if (*(str-1)=='/') {
      l = strlen(str);
      if (l==0) break;
      if (strncmp(str,"casesens",l)==0) {
	setBit((*flags), AS_casesens);
      }
      else if (strncmp(str,"wildcard",l)==0) {
	setBit((*flags), AS_wildcard);
      }
      else if (strncmp(str,"stopcodon",l)==0) {
	setBit((*flags), AS_stopcodon);
      }
      else if (strncmp(str,"variants",l)==0) {
	setBit((*flags), AS_variants);
      }
      else break;
      *(str-1) = '\0';
    }
    --str;
  }
  // Alphabet is now left as a string in orig
  str = newa;
  n = strlen(orig);
  for (i=0; i<n; ++i) {
    if ( i+4<n && orig[i]=='[' && orig[i+2]=='-' && orig[i+4]==']' ) {
      l = 1+orig[i+3]-orig[i+1];
      if (l>0 && l<=128) {
	for (k=0; k<l; ++k) *(str++) = orig[i+1]+k;
	i+=4;
      }
    }
    else *(str++) = orig[i];
  }
  *str = '\0';
  return strdup(newa);
}



/*
  This function checks if *a is equal to protein, DNA, IUPAC, or RNA, in which
  case an alphabet is made accordingly. revcomp=1 is assumed if DNA or RNA.

  Wildcard is included as last char (N for DNA/RNA, X for protein) if AS_wildcard
  bit is set.

  If AS_stopcodon bit is set and it is protein alphabet, '$' is added as stopcodon
  character.

  If AS_variants bit is set and it is DNA/IUPAC alphabet, '|' is added as stopcodon
  character.

  If caseSens is on, the bio alhphabet will be made in BOTH cases. So a nucl alphabet
  will have 8 (or 10) letters and a protein alphabet will have 40 (42) letters.

  The function ALWAYS adds a terminating symbol as first char (* as default)

  Otherwise it is like a call to alloc_AlphabetStruct

  Note that *a is unused and not freed.
 */
//AlphabetStruct *bio_AlphabetStruct(char *a, int caseSens, char term, int wild, int stopcodon) {
AlphabetStruct *bio_AlphabetStruct(char *a) {
  char str[128], lcstr[128];
  int n=0, nucl=0, prot=0;
  int revComp=0;
  AlphabetStruct *alph;
  int caseSens, wild;
  int wild_lett='N';
  char term ='*';  // This is added when the alloc_AlphabetStruct is called
  ushort flags=0;

  // Interpret qualifiers and ranges
  a=interpret_alphabet_specs(a, &flags);
  caseSens = checkBit(flags,AS_casesens);
  wild = checkBit(flags,AS_wildcard);

  if (strcmp(a,"DNA")==0) { nucl='T'; setBit(flags, AS_DNA); }
  else if (strcmp(a,"IUPAC")==0) { nucl='T'; setBit(flags, AS_DNA); setBit(flags, AS_IUPAC); }
  else if (strcmp(a,"RNA")==0) { nucl='U'; setBit(flags, AS_RNA); }
  else nucl=0;

  if (nucl) {
    revComp=1;
    setBit(flags,AS_revcomp);
    strcpy(str,"ACGT");
    str[3]=nucl;         // Changes T to U if RNA
    n=4;
    if (checkBit(flags,AS_IUPAC)) {
      strcpy(str+n,"RYSWKMBDHVN");
      n+=11;
      wild = 0;
      clearBit(flags, AS_wildcard);
    }
  }
  else if (strcmp(a,"protein")==0) {
    setBit(flags, AS_protein);
    prot=1;
    strcpy(str,"ACDEFGHIKLMNPQRSTVWY");
    n=20;
    if (wild) wild_lett='X';
  }
  else {
    strcpy(str,a);
    n = strlen(str);
  }
  if (caseSens) {
    if (checkBit(flags,AS_IUPAC)) strcpy(lcstr,"acgtn");
    else {
      strcpy(lcstr,str);
      change_case_string(lcstr);
    }
    strcpy(str+n,lcstr);
    n = strlen(str);
    if (wild) str[n++] = tolower(wild_lett);
    str[n]=0;
  }


  if (prot && checkBit(flags,AS_stopcodon)) str[n++] = '$';
  if (nucl && checkBit(flags,AS_variants))  str[n++] = '|';
  if (wild) str[n++]=wild_lett;      // We always have standard wildcard at end
  str[n]=0;

  /*
  if (prot || nucl) {
    alph = alloc_AlphabetStruct(str,caseSens,revComp,term,wild);
  }
  else alph = alloc_AlphabetStruct(str,caseSens,revComp,term,wild);
  */

  alph = alloc_AlphabetStruct_raw(str,term);

  // Set flags
  alph->flag |= flags;
  set_AlphabetStruct(alph);

  free(a);

  return alph;
}





void free_AlphabetStruct(AlphabetStruct *astruct) {
  if (astruct) {
    if (astruct->a) free(astruct->a);
    if (astruct->sensitivetrans) free(astruct->sensitivetrans);
    if (astruct->insenstrans) free(astruct->insenstrans);
    if (astruct->comp) free(astruct->comp);
    if (astruct->compTrans) free(astruct->compTrans);
    if (astruct->gCode) free(astruct->gCode);
    free(astruct);
  }
}


void print_AlphabetStruct(AlphabetStruct *a, FILE *fp) {
  fprintf(fp,"Alphabet: %s   ",a->a);
  if (a->comp) fprintf(fp,"complement: %s  ",a->comp);
  fprintf(fp,"length: %d",a->len);
  if (AlphabetStruct_test_flag(a,AS_casesens)) fprintf(fp," casesens");
  if (AlphabetStruct_test_flag(a,AS_wildcard)) fprintf(fp," wildcard");
  if (AlphabetStruct_test_flag(a,AS_term)) fprintf(fp," term");
  if (AlphabetStruct_test_flag(a,AS_protein)) fprintf(fp," protein");
  if (AlphabetStruct_test_flag(a,AS_DNA)) fprintf(fp," DNA");
  if (AlphabetStruct_test_flag(a,AS_RNA)) fprintf(fp," RNA");
  if (AlphabetStruct_test_flag(a,AS_revcomp)) fprintf(fp," revcomp");
  if (AlphabetStruct_test_flag(a,AS_stopcodon)) fprintf(fp," stopcodon");
  fprintf(fp,"\n");
}


/*
  translate a sequence (s) to numbers
 */
void translate2numbers(char *s, const long slen, AlphabetStruct *astruct) {
  long k;
  for (k=0;k<slen;++k) s[k]=astruct->trans[(int)s[k]];
}


/*
  NOTE THAT THIS FUNCTION reuses the memory of letters & id!!
*/
Sequence *make_Sequence(char *letters, char *id, AlphabetStruct *alphabet) {
  Sequence *seq = alloc_Sequence();
  seq->len = strlen(letters);
  seq->s = letters;
  seq->id = id;
  translate2numbers(seq->s,seq->len,alphabet);
  return seq;
}


// Skip until stopchar
static int skip_until_char(FILE *fp, int stopchar) {
  int c;
  while ( ( c=fgetc(fp)) ) if ( c==EOF || c==stopchar ) break;
  return c;
}


/* Assume that \n> or \n@ has been read and read to end of line
   ignore anything after space if save_descr==0
   returns 1 on success
*/
static int read___ID(FILE *fp, Sequence *seq, int save_descr) {
  const int id_read_size=256;
  iString *is;
  int c, i, lastc;

  is = alloc_iString(id_read_size);

  if (save_descr) {
    // Read whole line
    c = read_line_iString(fp,is,0,NULL);
    if ( c && is->len ) {
      seq->id = convert_iString(is,NULL,1);   // This frees iString
      // Find first space:
      for (i=0; i<is->len; ++i) if ( isspace(seq->id[i]) ) break;
      if (i<is->len) {
	seq->id[i++]='\0';
	for ( ; i<is->len; ++i) if ( !isspace(seq->id[i]) ) break;
	if (i<is->len) seq->descr = seq->id+i;
      }
      toggleBit(seq->flag,seq_flag_id);
    }
    else free_iString(is);
  }
  else {
    // Read ID (until space)
    c = read_line_iString(fp,is,' ',NULL); // Returns 0 on EOF
    if (c && is->len) {
      lastc = is->lastread;
      seq->id = convert_iString(is,NULL,1);
      toggleBit(seq->flag,seq_flag_id);
      if ( lastc != '\n' && lastc!=EOF) lastc = skip_until_char(fp,'\n'); // Skip to EOL
      if (lastc==EOF) c=0;
    }
    else free_iString(is);
  }

  return c;
}




/*

  Autodetect file type and returns type
  Except when type!=0, then the type is forced

  It determinines filetype by these rules:

  - Skips likes in the beginning of the file that starts with one of
    these chars: '#', ' ', '\t', and '\n'
  - If the starting char of the line reached is
      '>'   then fasta is assumed
      '@'   then fastq is assumed
      any other: single line format is assumed

  If it is single line format, the first char on the line is pushed back
  on the input stream

  returns 0 on EOF

  type is either '>' or '@' or 's' for fasta, fastq or single

 */
int ReadSequenceFileHeader(FILE *fp, int type) {
  int c;

  // Skip until a line that starts with something different from '#', ' ', '\t', and '\n'
  c = fgetc(fp); // c is first char on a line
  while ( c=='#' || c=='\n' || c==' ' || c=='\t' ) {
    if (c != '\n') c = skip_until_char(fp,'\n');  // Skip to EOL
    if (c==EOF) break;
    c = fgetc(fp);
  }
  if (c==EOF) return 0;

  if ( c!= '>' && c!= '@' ) ungetc(c,fp);

  if (type) {
    if ( type != 'l' && type != c ) {
      fprintf(stderr,"ReadSequenceFileHeader: Found letter %c in beginning of file, where %c ",c,type);
      ERROR("was expected",1);
    }
  }
  else {
    type = 's';
    if ( c=='>' || c=='@' ) type = c;
  }

  return type;
}



/* When reading an iString it takes an array of letters to include
   This array includes all letters A-Z,a-z
 */
static char *make_readInclude() {
  int i;
  static int done=0;
  static char z[128];
  if (!done) {
    for (i=0; i<128; ++i) z[i]=0;
    for (i='A'; i<'Z'; ++i) z[i]=1;
    for (i='a'; i<'z'; ++i) z[i]=1;
    done =1;
  }
  return z;
}



/*
  When this function is called, fp must be at the first position of the id (after '\n>')
  use function ReadSequenceFileHeader to reach that point

  It returns NULL on EOF. It uses eof as memory - if it is 1, it assumes that the EOF
  was reached at the last call. Therefore it must be 0 on first call

 */
Sequence *readFasta(FILE *fp, AlphabetStruct *alph, int read_size, int save_descr, char *eof) {
  int c;
  iString *is;
  Sequence *seq;
  static char *readInclude = NULL;

  if (!readInclude) readInclude = make_readInclude();
 
  // Last call reached EOF
  if (*eof) return NULL;

  // Read  ID
  seq = alloc_Sequence();
  c = read___ID(fp, seq, save_descr);

  if (c==0) { free_Sequence(seq); *eof=1; return NULL; }

  // Read sequence until next '>'
  is = alloc_iString(read_size);
  c = read_iString_until_startline(fp,is,'>',readInclude);
  //c = read_iString_until_startline(fp,is,'>',alph->trans);
  if (c==0) *eof=1;

  seq->len = is->len;
  if (is->len) {
    seq->s = convert_iString(is,NULL,0);
    translate2numbers((char *)seq->s, seq->len, alph);
    toggleBit(seq->flag,seq_flag_seq);
  }
  else free_iString(is); // In this case a sequence of length NULL is returned
  
  return seq;
}


/*
  When this function is called, fp must be at the first position of the id (after '\n@')
 */
Sequence *readFastq(FILE *fp, AlphabetStruct *seq_alph, AlphabetStruct *qual_alph, int read_size, int save_descr, char *eof) {
  int c;
  int i, n;
  iString *is;
  Sequence *seq;
  static char *readInclude = NULL;

  if (!readInclude) readInclude = make_readInclude();

  // Last call reached EOF
  if (*eof) return NULL;

  seq = alloc_Sequence();
  c = read___ID(fp, seq, save_descr);
  if (c==0) { free(seq); *eof=1; return NULL; }

  // Read sequence until next '\n+'
  is = alloc_iString(read_size);
  c = read_iString_until_startline(fp,is,'+',readInclude);
  //c = read_iString_until_startline(fp,is,'+',seq_alph->trans);
  if (c==0) ERROR("File ended in the middle og fastq entry",1);

  // Skip until end of line
  c = skip_until_char(fp,'\n');
  if (c==EOF) ERROR("File ended in the middle og fastq entry",1);

  seq->len = is->len;
  if (is->len) {
    toggleBit(seq->flag,seq_flag_seq);
    seq->s = convert_iString(is,NULL,0);   // Frees the iString
    translate2numbers((char *)seq->s, seq->len, seq_alph);
    // If a sequence was read, now read qual scores
    n=0;
    if ( qual_alph ) {
      seq->q = (char*)malloc(seq->len*sizeof(char));
      while ( n<seq->len) {
	c=fgetc(fp);
	if ( c==EOF ) break;
	i = qual_alph->trans[c];
	if (i>0) seq->q[n++] = i;
      }
      toggleBit(seq->flag,seq_flag_q);
    }
    else {
      while ( n<seq->len) {
	c=fgetc(fp);
	if ( c==EOF ) break;
	++n;
      }
    }

    if (c==EOF) {
      fprintf(stderr,"For sequence %s\n",seq->id);
      ERROR("EOF reached before quality sequence was complete",1);
    }

  }
  else free_iString(is);

  // Skip until next entry
  c = ' ';
  while ( c!='@' && c!=EOF ) {
    if (c!='\n') c = skip_until_char(fp,'\n');
    if (c!=EOF) c = fgetc(fp);
  }

  if (c==EOF) *eof=1;

  return seq;
}



/*
  You can specify the single line format with a string like this:
     'i1s5l6q7S '
  After i, s, q, and l follows an integer telling the field number for
  id, sequence, secondary seq, and labels, respectively (counting from 1).
  After S comes a single char giving the separator. 't' means tab.
  They can come in random order. Terms not given use the argument to the func instead

  Use 0 for fields if not used

 */
singleLineStruct *make_singleLineStruct(int separator, int id_field, int seq_field, int lab_field, int q_field,
					AlphabetStruct *seq_alph, AlphabetStruct *lab_alph, AlphabetStruct *q_alph,
					char *format) {
  int i;
  singleLineStruct *r = (singleLineStruct *)malloc(sizeof(singleLineStruct));
  char *b, *e, c;

  r->separator = separator;
  r->id_field = --id_field;
  r->seq_field = --seq_field;
  r->lab_field = --lab_field;
  r->q_field = --q_field;
  r->seq_alph = seq_alph;
  r->lab_alph = lab_alph;
  r->q_alph = q_alph;


  // Interpret format string:
  if (format) {
    b = format;
    while(*b) {
      // Separator
      if (*b=='S') {
	r->separator = *(b+1);
	fprintf(stderr,"separator >>%c<<\n",r->separator);
	if (r->separator=='t') r->separator='\t';
	b+=2;
      }
      // Field number
      else {
	e = b+1;
	while (*e && isdigit(*e)) ++e;
	c=*e; *e='\0';
	switch (*b) {
	case 'i':
	  r->id_field = atoi(b+1)-1; fprintf(stderr,"id_field %d\n",r->id_field); break;
	case 's':
	  r->seq_field = atoi(b+1)-1; fprintf(stderr,"seq_field %d\n",r->seq_field); break;
	case 'q':
	  r->q_field = atoi(b+1)-1; fprintf(stderr,"q_field %d\n",r->q_field); break;
	case 'l':
	  r->lab_field = atoi(b+1)-1; fprintf(stderr,"lab_field %d\n",r->lab_field); break;
	default:
	  fprintf(stderr,"Don't understand single line format string %s - ",format);
	  ERROR("dying",1);
	}
	b=e;
	*b=c;
      }
    }
  }

  // Find max of id_field, seq_field and lab_field (field numbers decreased by 1)
  r->max_field = r->lab_field;
  if (r->max_field < r->id_field) r->max_field = r->id_field;
  if (r->max_field < r->seq_field) r->max_field = r->seq_field;
  if (r->max_field < r->q_field) r->max_field = r->q_field;

  r->desc = alloc_iString(256);
  r->strings = (iString**)malloc((r->max_field+1)*sizeof(iString*));
  for (i=0; i<=r->max_field; ++i) r->strings[i]=r->desc;

  return r;
}


void free_singleLineStruct(singleLineStruct *s) {
  free(s->strings);
  free_iString(s->desc);
  free(s);
}


/*
  Single line format has everything on one line separatet by separator.
  The seq, labels, and id have the specified fields
  All other info is stored in description with separators in between

  Fields are counted from 1

  Labels:
    if lab_field>0, labels are read
    if label_alph==NULL they are discarded (ie not included in ->lab or ->desc)
  Same for seq2

 */
Sequence *readSingleLineFormat(FILE *fp, singleLineStruct *sls, int read_size, char *eof) {
  int i;
  iString *curr;
  int lastchar, error=0;
  Sequence *seq;

  sls->strings[sls->id_field] = alloc_iString(256);
  sls->strings[sls->seq_field] = alloc_iString(read_size);
  if (sls->lab_field>=0) sls->strings[sls->lab_field] = alloc_iString(read_size);
  if (sls->q_field>=0) sls->strings[sls->q_field] = alloc_iString(read_size);

  // Read the line
  lastchar=0;
  i=0;
  while (lastchar!= EOF && lastchar!='\n') {
    if (i>sls->max_field) curr=sls->desc;
    else curr=sls->strings[i];
    if (curr->len>0) append_char_iString(curr, sls->separator);
    read_line_iString(fp,curr,sls->separator,NULL);
    lastchar = curr->lastread;
    /* This is a hack to ignore blank lines */
    if ( i==0 && lastchar=='\n' && curr->len==0) {
      reset_iString(curr);
      if (lastchar!=EOF) lastchar=0;
    }
    else ++i;
  }

  if (lastchar==EOF) {
    *eof=1;
    free_iString(sls->strings[sls->id_field]);
    free_iString(sls->strings[sls->seq_field]);
    if (sls->lab_field>=0) free_iString(sls->strings[sls->lab_field]);
    if (sls->q_field>=0) free_iString(sls->strings[sls->q_field]);
    return NULL;
  }

  // Checks;
  if (i<sls->max_field) { error++; fprintf(stderr,"readSingleLineFormat: Not all fields present on line."); }
  if (sls->strings[sls->id_field]->len == 0) { error++; fprintf(stderr,"readSingleLineFormat: ID empty."); }
  if (sls->strings[sls->seq_field]->len == 0) { error++; fprintf(stderr,"readSingleLineFormat: No sequence read."); }
  if (sls->lab_field>=0 && sls->strings[sls->seq_field]->len != sls->strings[sls->lab_field]->len) {
    error++; fprintf(stderr,"readSingleLineFormat: Label has length different from sequence.");
  }
  if (sls->q_field>=0 && sls->strings[sls->seq_field]->len != sls->strings[sls->q_field]->len) {
    error++; fprintf(stderr,"readSingleLineFormat: Label has length different from sequence.");
  }
  if (error) ERROR(" Dying",1);

  // Make sequence
  seq = alloc_Sequence();
  seq->id = convert_iString(sls->strings[sls->id_field],NULL,1);
  if (sls->desc->len>0) seq->descr = convert_iString_nondestruct(sls->desc,NULL,1);
  reuse_iString(sls->desc);

  seq->len = sls->strings[sls->seq_field]->len;
  seq->s = convert_iString(sls->strings[sls->seq_field],NULL,0);
  translate2numbers((char *)seq->s, seq->len, sls->seq_alph);
  toggleBit(seq->flag,seq_flag_seq);

  if (sls->lab_field>=0) {
    // Discard labels if no label alphabet is given
    if (sls->lab_alph) {
      seq->lab = convert_iString(sls->strings[sls->lab_field],NULL,0);
      translate2numbers((char *)seq->lab, seq->len, sls->lab_alph);
      toggleBit(seq->flag,seq_flag_lab);
    }
    else free_iString(sls->strings[sls->lab_field]);
  }
  if (sls->q_field>=0) {
    // Discard q if no q alphabet is given
    if (sls->q_alph) {
      seq->q = convert_iString(sls->strings[sls->q_field],NULL,0);
      translate2numbers((char *)seq->q, seq->len, sls->q_alph);
      toggleBit(seq->flag,seq_flag_q);
    }
    else free_iString(sls->strings[sls->q_field]);
  }

  return seq;
}




/* Complement a sequence using the trans table
*/
static inline void complement(char *s, const char *trans, const int len) {
  int i;
  for (i=0; i<len; ++i) s[i] = trans[ (int)s[i] ];
}



/*
  Reverse sequence - do NOT complement
*/
void reverseSequence(Sequence *seq) {
  toggleBit(seq->flag,seq_flag_rev);
  reverseStringInplace(seq->s,seq->len);
  if (seq->lab) reverseStringInplace(seq->lab,seq->len);
  if (seq->q) reverseStringInplace(seq->q,seq->len);
}


void revcompSequence(Sequence *seq, AlphabetStruct *astruct) {
  reverseSequence(seq);
  complement(seq->s,astruct->compTrans,seq->len);
  toggleBit(seq->flag,seq_flag_comp);
}



/*
  This is discontinued
  If inplace==0, a new seq is allocated
*/
static Sequence *revcomp(Sequence *s, AlphabetStruct *astruct, int inplace) {
  Sequence *r = s;

  if (!inplace) { // Allocates a new sequence, but point to id and description
    r = (Sequence*)malloc(sizeof(Sequence));
    memcpy(r,s,sizeof(Sequence));
    r->s = (char*)malloc(r->len*sizeof(char));
    if (s->lab) r->lab = (char*)malloc(r->len*sizeof(char));
    if (s->q) r->q = (char*)malloc(r->len*sizeof(char));
  }

  toggleBit(r->flag,seq_flag_rev);
  toggleBit(r->flag,seq_flag_comp);

  reverseString(s->s, r->s, s->len);
  complement(r->s, astruct->compTrans, r->len);

  if (s->lab) reverseString(s->lab, r->lab, s->len);
  if (s->q) reverseString(s->q, r->q, s->len);

  return r;
}



// Translates s in interval [from,from+printlen[ using alphabet and prints
void printSeqRaw(FILE *file, char *s, int seqlen, char *alphabet, int from, int printlen) {
  int stop=from+printlen;
  if (stop>seqlen) stop=seqlen;
  while ( from<stop ) fputc(alphabet[(int)s[from++]],file);
}

// As above but backwards
void printSeqRawReverse(FILE *file, char *s, int seqlen, char *alphabet, int from, int printlen) {
  int n = from+printlen;
  if (n>seqlen) n=seqlen;
  while ( n-- > from ) fputc(alphabet[(int)s[n]],file);
}


// Print id, space, sequence on one line
void printSeqOneLine(FILE *file, Sequence *seq, char *alphabet) {
  int n=0;
  char *s = seq->s;

  if (seq->id != NULL) fprintf(file,"%s ",seq->id);

  while (n<seq->len) fputc(alphabet[(int)s[n++]],file);
  fputc('\n',file);
}

void printFasta(FILE *file, Sequence *seq, char *alphabet, int linelen) {
  int n=0;
  char *s = seq->s;

  if (linelen<=0) linelen=70;

  if (seq->id != NULL) fprintf(file,">%s",seq->id);
  if (seq->descr != NULL) fprintf(file," %s",seq->descr);
  fprintf(file,"\n");

  while (n<seq->len) {
    if (n>0 && n%linelen==0) fputc('\n',file);
    fputc(alphabet[(int)s[n++]],file);
  }
  fputc('\n',file);
}

// Returns 64 if triplet contains unknown char
static inline int triplet2number(char *s) {
  if (s[0]<1 || s[0]>4) return 64;
  if (s[1]<1 || s[1]>4) return 64;
  if (s[2]<1 || s[2]>4) return 64;
  return 16*(s[0]-1)+4*(s[1]-1)+(s[2]-1);
}

// Returns 64 if triplet contains unknown char
// This is used for case-sensitive alphabets
static inline int triplet2numberCaseSensitive(char *s) {
  char x[3];
  int i;
  for (i=0;i<3;++i) {
    x[i]=s[i];
    if (x[i]>4) x[i]-=4;
    if (x[i]<1 || x[i]>4) return 64;
  }
  return 16*(x[0]-1)+4*(x[1]-1)+(x[2]-1);
}

/*
  Make a hash with genetic code for a DNA alphabet

  We're assuming that a,c,g,t are the 4 first letters (1-4).
  Order does not matter.

  We're assuming that lower case and upper case letters are represented
  by same number

  Using '$' for stop codon

  Using 'X' for unknown AAs

  A string with the info is returned in alph->gCode;

  Often you want the AA coded in the protein alphabet. If prot_alph is
  given, the function changes the gCode from letters to numbers of the
  protein alphabet.

  Code used:

  AAA K     AGA R     CAA Q     CGA R     GAA E     GGA G     TAA $     TGA $
  AAC N     AGC S     CAC H     CGC R     GAC D     GGC G     TAC Y     TGC C
  AAG K     AGG R     CAG Q     CGG R     GAG E     GGG G     TAG $     TGG W
  AAT N     AGT S     CAT H     CGT R     GAT D     GGT G     TAT Y     TGT C
  ACA T     ATA I     CCA P     CTA L     GCA A     GTA V     TCA S     TTA L
  ACC T     ATC I     CCC P     CTC L     GCC A     GTC V     TCC S     TTC F
  ACG T     ATG M     CCG P     CTG L     GCG A     GTG V     TCG S     TTG L
  ACT T     ATT I     CCT P     CTT L     GCT A     GTT V     TCT S     TTT F

*/
void makeGeneticCode(AlphabetStruct *alph, AlphabetStruct *prot_alph) {
  const char *bases="ACGT";
  const char *code="KNKNTTTTRSRSIIMIQHQHPPPPRRRRLLLLEDEDAAAAGGGGVVVV$Y$YSSSS$CWCLFLFX";
  char *gCode = (char *)malloc(66*sizeof(char));
  int i, t, count[3];
  char *ltrans = alph->insenstrans;
  char seq[3];

  if ( !AlphabetStruct_test_flag(alph,AS_DNA) && !AlphabetStruct_test_flag(alph,AS_RNA) ) {
    ERROR("Translation only works for built-in DNA & RNA alhpabets",1);
  }

  /*
  for (i=0; i<4; ++i) {
    t = ltrans[(int)bases[i]];
    if (t>4 || t<1) {
      fprintf(stderr,"Alphabet %s letter=%c number=%d\n",alph->a,bases[i],t);
      ERROR("Cannot translate this alphabet to protein",1);
    }
  }
  */

  for (i=0; i<3; ++i) count[i]=0;

  /*
    seq[] is a triplet of ACGT letters, which is translated to numbers with ltrans
   */
  t = 0;
  while ( t<64 ) {
    for (i=0; i<3; ++i) seq[i] = ltrans[(int)bases[count[i]]];
    i = triplet2number(seq); // This is the number of the triplet in the alphabet (may be different from t)
    gCode[i] = code[t];



    // Go to next codon
    if ( ++count[2] > 3 ) {
      count[2]=0;
      if ( ++count[1] > 3 ) {
	count[1]=0;
	++count[0];
      }
    }
    ++t;
  }

  gCode[64]='X';
  gCode[65]='\0';

  if (prot_alph) {
    for (i=0; i<65; ++i) {
      // fprintf(stderr,"gCode %d %c",i,gCode[i]);
      gCode[i] = prot_alph->insenstrans[(int)gCode[i]];
      // fprintf(stderr," -> %c %d\n",prot_alph->a[gCode[i]],gCode[i]);
    }
  }

  if (alph->gCode) free(alph->gCode);
  alph->gCode = gCode;

}




/*
  Translate DNA to protein in all reading frames and returns as a string
  the same length as the original sequence

  Translation is case-insensitive

  Note that the alphabet is the DNA alphabet.

  Note also that makeGeneticCode(dna_alphabet,protein_alph) must be called before this function

  The first two letters of the sequence is set to unkown X

  Triplets containing non-ACGT letters are also set to X

  Stop codons are '*'

 */
char *translateDNA(Sequence *seq, AlphabetStruct *alph) {
  int i, t;
  char *translation = (char *)malloc((seq->len+1)*sizeof(char));

  if (alph->gCode==NULL) ERROR("You must call makeGeneticCode before using translateDNA",1);

  translation[0] = translation[1] = alph->gCode[64];

  if (AlphabetStruct_test_flag(alph,AS_casesens)) {
    for (i=2; i<seq->len; ++i) {
      t = triplet2numberCaseSensitive(seq->s+i-2);
      translation[i] = alph->gCode[t];
    }
  }
  else {
    for (i=2; i<seq->len; ++i) {
      t = triplet2number(seq->s+i-2);
      translation[i] = alph->gCode[t];
    }
  }
  translation[i] = '\0';

  return translation;
}
