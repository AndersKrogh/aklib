/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/

#ifndef OPTIONSANDARGS
#define OPTIONSANDARGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OPTTYPE_SWITCH 1
#define OPTTYPE_VALUE 2
#define OPTTYPE_ARG 3

#define VARTYPE_int 1
#define VARTYPE_double 2
#define VARTYPE_charS 3
#define VARTYPE_intS 4
#define VARTYPE_doubleS 5
#define VARTYPE_charSS 6


typedef struct {
  int opttype;
  int vartype;
  void *pt;
  int  *count;
  char *names;
  char *text;
} OPT_STRUCT;

static char *opt_indent = "      ";


/* PRINT AN ERROR MESSAGE AND EXIT */
static void OPT_error(char *msg) {
  if (msg) fprintf(stderr, "OPT_Error: %s\n", msg);
  exit(1);
}

/* Duplicates a string */
static char *OPT_stringdup(const char *s) {
  int l=1+strlen(s);
  char *ret=(char *)malloc(l*sizeof(char));
  strcpy(ret,s);
  return ret;
}

/* Gives one line per variable with values. Lines are preceeded by
   the prefix string (maybe "").
   If verbose !=0 a more descriptive output is obtained (used with
   -help).
*/
static void OPT_print_vars(FILE *fp, OPT_STRUCT *opt, char *prefix, int  verbose) {
  int argi=1;
  int i;
  char *c;

  if (verbose && opt->text) {
      fprintf(fp,"%s\nOptions and arguments:\n\n",opt->text);
  }
  ++opt;
  while (opt->opttype) {
    if (verbose) {
      fprintf(fp,"%s",prefix);
      c = opt->names;
      if (opt->opttype==OPTTYPE_ARG) fprintf(fp,"ARG %d",argi++);
      else { fputc('-',fp); ++c; }
      while (*c) {
	  if (*c=='|' && *(c+1)) fprintf(fp,", -");
	  if (*c!='|') fputc(*c,fp);
	  ++c;
      }
      if (opt->opttype!=OPTTYPE_SWITCH) {
	switch (opt->vartype) {
	case VARTYPE_int: fprintf(fp," (integer)"); break;
        case VARTYPE_double: fprintf(fp," (double)"); break;
        case VARTYPE_charS: fprintf(fp," (string)"); break;
        case VARTYPE_intS: fprintf(fp," (integer array, current size %d)",*opt->count); break;
        case VARTYPE_doubleS: fprintf(fp," (double array, current size %d)",*opt->count); break;
        case VARTYPE_charSS: fprintf(fp," (string array, current size %d)",*opt->count); break;
        }
      }
      fprintf(fp,"\n%s%s\n%s%sValue: ",prefix,opt->text,prefix,opt_indent);
    }
    else {
      fprintf(fp,"%s",prefix);
      c = opt->names+1; while (*c!='|') fputc(*(c++),fp);
      fputc('=',fp);
    }
    if (opt->opttype==OPTTYPE_SWITCH) {
      fprintf(fp,"%s",(*(int *)(opt->pt)?"ON":"OFF"));
    }
    else {
      switch (opt->vartype) {
      case VARTYPE_int:
	fprintf(fp," %d",*(int *)(opt->pt));
	break;
      case VARTYPE_double:
	fprintf(fp," %f",*(double *)(opt->pt));
	break;
      case VARTYPE_charS:
	if (*(char **)(opt->pt)==NULL) fprintf(fp," NULL");
	fprintf(fp," %s",*(char **)(opt->pt));
	break;
      case VARTYPE_intS:
	if (*(int **)(opt->pt)==NULL) fprintf(fp," NULL");
	else for (i=0;i<*opt->count;++i)
	  fprintf(fp," %d", (*(int **)(opt->pt))[i]);
	break;
      case VARTYPE_doubleS:
	if (*(double **)(opt->pt)==NULL) fprintf(fp," NULL");
	else for (i=0;i<*opt->count;++i)
	  fprintf(fp," %f", (*(double **)(opt->pt))[i]);
	break;
      case VARTYPE_charSS:
	if (*(char ***)(opt->pt)==NULL) fprintf(fp," NULL");
	else for (i=0;i<*opt->count;++i)
	  fprintf(fp," %s", (*(char ***)(opt->pt))[i]);
	break;
      }
    }
    fprintf(fp,"\n");
    if (verbose) fprintf(fp,"%s\n",prefix);

    ++opt;
  }
}


static void OPT_help(OPT_STRUCT *opt) {
  OPT_print_vars(stdout, opt, "", 1);
}


/* Read the command line */
static void OPT_read_cmdline(OPT_STRUCT *opt, int argc, char **argv) {
  int argi=1, i, n=0, match;
  OPT_STRUCT *o;
  char argument[100];
  // void *pt;

  while (argi<argc) {
    match=0;
    o=opt+1;
    /* if option, find match */
    if (argv[argi][0]=='-') {
      while ( o->opttype ) {
	// if (o->opttype == OPTTYPE_SWITCH && argv[argi][1]=='n' && argv[argi][2]=='o') l=3;
	// else l=1;
        sprintf(argument,"|%s|",argv[argi]+1);
	if (strstr(o->names,argument)) {
//          fprintf(stderr,"Found option %s =~ %s\n",argv[argi],o->names);
	  ++argi;
	  match=1;
	  break;
	}
	++o;
      }
    }
    else { /* Otherwise it is an argument */
      while ( o->opttype ) {
	if ( o->opttype == OPTTYPE_ARG && *(o->count) == 0 ) {
//          fprintf(stderr,"Found argument %s fits %s\n",argv[argi],o->names);
	  match=1;
	  break;
        }
	++o;
      }
    }
    if (!match) {
      fprintf(stderr,"Didn't understand argument %s\n\n",argv[argi]);
      OPT_print_vars(stderr, opt, "", 1);
      OPT_error("\n");
    }
    /* Now o is pointing to the relevant option and argi is the argument to read */
    if (o->opttype == OPTTYPE_SWITCH) {
      // if (l==3) *(int *)(o->pt) = 0; else
      *(int *)(o->pt) = 1;
      *(o->count) += 1;
    }
    else {
      if (argi>=argc) OPT_error("Running out of arguments");
      if (o->vartype == VARTYPE_intS || o->vartype == VARTYPE_doubleS || o->vartype == VARTYPE_charSS) {
	// n = *(o->count) = atoi(argv[argi]);
	n = *(o->count);
	if (*(void **)(o->pt)) free(*(void **)(o->pt));
	*(void **)(o->pt) = NULL;
	if (n<=0) OPT_error("Array has zero or negative length");
	if (argi+n>argc) OPT_error("Running out of arguments");
      }
      else *(o->count) += 1;
      switch (o->vartype) {
      case VARTYPE_int:
	*(int *)(o->pt) = atoi(argv[argi++]);
	break;
      case VARTYPE_double:
	*(double *)(o->pt) = atof(argv[argi++]);
	break;
      case VARTYPE_charS:
	if (*(char **)(o->pt) && *(o->count)>1 ) free(*(char **)(o->pt));
	*(char **)(o->pt) = OPT_stringdup(argv[argi++]);
	break;
      case VARTYPE_intS:
	*(int **)(o->pt) = (int *)calloc(n,sizeof(int));
	for (i=0;i<n; ++i, ++argi) (*(int **)(o->pt))[i] = atoi(argv[argi]);
	break;
      case VARTYPE_doubleS:
	*(double **)(o->pt) = (double *)calloc(n,sizeof(double));
	for (i=0;i<n; ++i, ++argi) (*(double **)(o->pt))[i] = atof(argv[argi]);
	break;
      case VARTYPE_charSS:
	*(char ***)(o->pt) = (char **)calloc(n,sizeof(char *));
	for (i=0;i<n; ++i, ++argi) (*(char ***)(o->pt))[i] = OPT_stringdup(argv[argi]);
	break;
      }
    }
  }
  // Check for -v (always second last in opt array)
  // Go to second last entry
  o=opt+1;
  while ( (o+1)->opttype ) ++o;
  if (*(o->count)) {
    ++o;
    if (strlen(o->names)>0) fprintf(stderr,"%s ",o->names);
    fprintf(stderr,"Version %s\n",o->text);
    exit(0);
  }
}

static char **_add_char_(char **argv, int argc, int i, int c) {
  int k;
  // Alloc/re-alloc argv
  if (i==0) {
    if (argc==1) argv=(char **)calloc(100,sizeof(char*));
    else if (argc%100==0) {
      argv=(char **)realloc(argv,(argc+100)*sizeof(char*));
      for (k=0; k<100; ++k) argv[argc+k]=NULL;
    }
    argv[argc] = (char *)malloc(1001*sizeof(char));
  }
  // realloc individual arg if longer than allocation
  else if (i%1000) argv[argc] = (char *)realloc(argv[argc],(i+1001)*sizeof(char));

  // Argument ends if c==0
  if (!c) {
    argv[argc][i]='\0';
    argv[argc] = (char *)realloc(argv[argc],(i+1)*sizeof(char));
  }
  
  argv[argc][i]=c;
  // argv[argc][i+1]=0; fprintf(stderr,"ARG %d (%d) %s\n",argc,i,argv[argc]);
  return argv;
}

/* Read option file
   All lines must start with
     - '#' a comment line
     - An option (no '-' in front)
     - a blank char, which means the line contains an argument (or is empty)
   Things between "" are read together
*/

static void OPT_read_option_file(FILE *fp, OPT_STRUCT *opt) {
  int i=0, argc=1, quote=0, comment=0, newline=1, c;
  char **argv=NULL;

  while ( (c=fgetc(fp)) != EOF ) {
    // Finish argument
    if ( ( isspace(c) || c=='#') && !quote && !comment && i>0 ) {
      argv = _add_char_(argv,argc,i,0);
      // fprintf(stderr,"--------arg %d >%s<\n",argc,argv[argc]);
      ++argc;
      i=0;
    }

    // At new line a comment ends
    if (c=='\n') { newline=1; comment=0; continue; }
    if (c=='#') { comment=1; }
    if (comment) { continue; }

    if ( isspace(c) && !quote ) continue;

    // If previous was a new line an option name may follow
    if (newline && !isblank(c) ) { argv = _add_char_(argv,argc,i++,'-'); }
    newline=0;

    if ( c=='\'' || c=='\"' ) {
      if (!quote) quote=c;
      else if (c==quote) quote=0;
      continue;
    }

    argv = _add_char_(argv,argc,i++,c);

  }

  if (i>0) {
    argv = _add_char_(argv,argc,i,0);
    // fprintf(stderr,"--------arg %d >%s<\n",argc,argv[argc]);
    ++argc;
  }
 
  OPT_read_cmdline(opt, argc, argv);

  for (i=0; i<argc; ++i) free(argv[i]);
  if (argv) free(argv);
  
}


static inline void OPT_open_read_option_file(char *file, OPT_STRUCT *opt) {
  FILE *fp = fopen(file,"r");
  if (!fp) { fprintf(stderr,"Could not open file %s ",file); OPT_error("\n"); }
  OPT_read_option_file(fp, opt);
  fclose(fp);
}





#endif
