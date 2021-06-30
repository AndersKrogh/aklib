/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/


/*
  Implements a reverse Polish calculator, which is compiled, so the same
  calculation can be run many times.

  Operators: + - * / exp exp2 exp10 log log2 log10 sqrt
  chs: unary change sign
  exch: Exchanges the two latest numbers on the stack
  pow: Raises y to power x ("7.1 3 pow" calculates 7.1 to the power of 3)
  pop: pops the stack
  dup: duplicate top number on stack
  max: takes the maximum of the two top numbers
  min: takes the minimum of the two top numbers
  step: Step function equals 0 for argument < 0 and 1 for larger arg>=0
  if: If x>=0 choose top of stack, otherwise choose top-1. Note that it pops
     the test value, so stack reduced by 2 ("1 2 1 if" -> "2")
     If you want to take log if stack value>0.01 and
     otherwise return log(0.01): "0.01 exch dup 0.01 - if log" 
  sto: store in memory (0..63) the next element on stack and pops stack 
     (so "6.3 5 sto" stores the number 6.3 in memory location 5 and
     removes both 6.3 and 5 from stack) 
  rcl: opposite of sto: Pushes on stack from memory (5 rcl would recall 6.3)

  In addition, you can set variables (single letters). "=y" sets y equal to the
  latest value on stack and pops the stack.

  These programs calculate 2^9+log10(5)-10^3 - the second illustrates variables
  "9 exp2 5 log10 + 3 exp10 -"
  "9 exp2 =x 5 log10 =y 3 exp10 =z x y + z -"

  You can set constants (or variables) and push stuff on the stack

  compile_calcuator(char *program): Compiles (and creates) calculator
  run_calculator(calculator *c): Runs it

  To make a calculator that calculates y=a+b*exp(x) of a number x with constants
  a and b, can be done like this:

  char *prog  = "exp b * a +";
  calculator *calc = compile_calcuator(prog);
  set_constant_calculator(calc,'a',3.2);  // assuming a=3.2 and b=1
  set_constant_calculator(calc,'b',1.0);
  for (i=0; i<n; ++i) {
    push_stack_calculator(calc,x[i]);
    y[i]=run_calculator(calc);
    clear_stack_calculator(calc);
  }
  free_calculator(calc);

  The three central lines could be replaced by y[i]=run_calculator_single(calc,x[i]);

  There are 128 memory locations. Letters use 65-90 & 97-122. These and the rest
  can be used by sto and rcl. So "65 sto" is equivalent to "=A".  Generally, you are
  advised not to mix the two modes and to use only 0-64 for sto/rcl.

  To have an integer calculator, use this:
  #define RPINTEGER


  For testting:
  gcc -g -D TEST -o reversePolish reversePolish.c

 */



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "reversePolish.h"


static inline void error(char *msg) {
  fprintf(stderr,"Calculator: %s\n",msg);
  exit(1);
}


static inline void cerror(char *msg) {
  fprintf(stderr,"Calculator: Don't understand %s\n",msg);
  exit(1);
}


static inline vartype pop_stack(calculator *c) {
  if ( --(c->n)<0 ) error("pop_stack: Stack empty");
  return c->stack[c->n];
}

static inline void push_stack(calculator *c, vartype x) {
  c->stack[c->n]=x;
  if ( ++(c->n)>=calc_max_size ) error("push_stack: Stack overflow");
}

void free_calculator(calculator *c) { free(c); }

void clear_stack_calculator(calculator *c) { c->n = 0; }

vartype pop_stack_calculator(calculator *c) { return pop_stack(c); }

void push_stack_calculator(calculator *c, vartype value) { push_stack(c,value); }

// Copies an array to the top of the stack
void copy_stack_calculator(calculator *c, vartype *array, int n) {
  int i;
  for (i=0; i<n; ++i) push_stack(c,array[i]);
}

/* Operations ****************************/

static void push(calculator *c) {
  c->stack[c->n]=c->constants[c->cc++];
  if ( ++(c->n)>=calc_max_size ) error("push: Stack overflow");
}

static void pop(calculator *c) {
  if ( --(c->n)<0 ) error("pop: Stack underflow");
}

static void duplicate(calculator *c) {
  if ( c->n <= 0 ) error("duplicate: Stack empty");
  push_stack(c, c->stack[c->n-1]);
}
static void exchange(calculator *c) {
  vartype tmp1 = pop_stack(c);
  vartype tmp2 = pop_stack(c);
  push_stack(c, tmp1);
  push_stack(c, tmp2);
}
static void if_func(calculator *c) {
  vartype y;
  if (pop_stack(c)<0) pop_stack(c);
  else {
    y=pop_stack(c);
    pop_stack(c);
    push_stack(c, y);
  }
}

static void max_func(calculator *c) {
  vartype tmp1 = pop_stack(c);
  vartype tmp2 = pop_stack(c);
  if (tmp1>tmp2) push_stack(c, tmp1);
  else push_stack(c, tmp2);
}

static void min_func(calculator *c) {
  vartype tmp1 = pop_stack(c);
  vartype tmp2 = pop_stack(c);
  if (tmp1<tmp2) push_stack(c, tmp1);
  else push_stack(c, tmp2);
}


static void add(calculator *c) { vartype y=pop_stack(c); push_stack(c,pop_stack(c)+y); }
static void subtract(calculator *c) { vartype y=pop_stack(c); push_stack(c,pop_stack(c)-y); }
static void multiply(calculator *c) { vartype y=pop_stack(c); push_stack(c,pop_stack(c)*y); }
static void divide(calculator *c) { vartype y=pop_stack(c); push_stack(c,pop_stack(c)/y); }
static void power(calculator *c) { vartype y=pop_stack(c); push_stack(c,pow(pop_stack(c),y)); }

static void exponential(calculator *c) { push_stack(c,exp(pop_stack(c))); }
static void exponential2(calculator *c) { push_stack(c,exp(M_LN2*pop_stack(c))); }
static void exponential10(calculator *c) { push_stack(c,exp(M_LN10*pop_stack(c))); }
static void logarithm(calculator *c) { push_stack(c,log(pop_stack(c))); }
static void logarithm2(calculator *c) { push_stack(c,log2(pop_stack(c))); }
static void logarithm10(calculator *c) { push_stack(c,log10(pop_stack(c))); }
static void step_func(calculator *c) { vartype x=1; if (pop_stack(c)<0) x=0; push_stack(c,x); }
static void change_sign(calculator *c) { push_stack(c, -pop_stack(c) ); }
static void squareroot(calculator *c) { push_stack(c,sqrt(pop_stack(c))); }


/* Save in variable */
static void store(calculator *c) { int k = (int)(0.01+pop_stack(c)); c->variable[k] = pop_stack(c); }

//static void store(calculator *c) { c->variable[ (int)(0.01+c->constants[c->cc++]) ] = pop_stack(c); }

/* Recall value of variable */
static void recall(calculator *c) { int k = (int)(0.01+pop_stack(c)); push_stack(c,c->variable[k]); }

//static void recall(calculator *c) { push_stack(c,c->variable[ (int)(0.01+c->constants[c->cc++]) ]); }


static char *next_token(char *s) {
  static char *curr;
  char *r;
  if (s) curr=s;
  if (!curr) return NULL;
  if (!s) {
    *curr = ' ';
    ++curr;
  }
  while (*curr && isspace(*curr)) ++curr;
  if (!(*curr)) return NULL;
  r = curr;
  while (*curr && !isspace(*curr)) ++curr;
  if (*curr=='\0') curr=NULL;
  else *curr='\0';
  return r;
}

#ifdef RPINTEGER
static int is_vartype(char *s) {
  // Is the first char a '-' ?
  if ( *s=='-') { ++s; if (!*s) return 0; }
  while (*s) {
    if ( !isdigit(*s) ) return 0;
    ++s;
  }
  return 1;
}
#else
static int is_vartype(char *s) {
  int decimal=0;
  // Is the first char a '-' ?
  if ( *s=='-') { ++s; if (!*s) return 0; }
  while (*s) {
    if ( *s=='.' && ++decimal>1 ) return 0;
    if ( !isdigit(*s) && *s!='.' ) return 0;
    ++s;
  }
  return 1;
}
#endif


// Store in memory location i
void store_calculator(calculator *c, int i, vartype value) {
  if (i<0 || i>127) {
    fprintf(stderr,"store_calculator: Illeagal memory location: %d\n",i);
    error("store_calculator: has to be between 0 and 127");
  }
  c->variable[i]=value;
}


/* You can preset a constant, like c=3 by calling this function before running
   the program, using set_constant(calculator,'c',3.0)
*/
void set_const_calculator(calculator *c, char nm, vartype value) {
  if (!isalpha(nm)) {
    fprintf(stderr,"set_const_calculator: Illeagal name for constant: %c\n",nm);
    error("set_const_calculator: A constant has to be a letter");
  }
  c->variable[(int)nm]=value;
}


vartype run_calculator(calculator *c) {
  int i;
  c->cc=0;
  for (i=0; i<c->no; ++i) c->operations[i](c);
  return pop_stack(c);
}


vartype run_calculator_single(calculator *calc, vartype value) {
  vartype rval;
  push_stack(calc,value);
  rval = run_calculator(calc);
  clear_stack_calculator(calc);
  return rval;
}


calculator *compile_calculator(char *program) {
  char *s;
  calculator *c = (calculator *)malloc(sizeof(calculator));

  c->n=0;
  c->nc=0;
  c->cc=0;
  c->no=0;
  s = next_token(program);
  while (s) {
    // fprintf(stderr,"%s\n",s);
    if ( is_vartype(s) ) {
      c->constants[c->nc++] = atof(s);
      c->operations[c->no++] = push;
    }
    else {
      // If single letter, it is a variable name
      if ( strlen(s)==1 && isalpha(*s) ) {
	c->constants[c->nc++] = *s;
	c->operations[c->no++] = push;
	c->operations[c->no++] = recall;
      }
      else {
	switch (*s) {
	case '+':
	  if (strlen(s)>1) cerror(s);
	  c->operations[c->no++] = add;
	  break;
	case '-':
	  if (strlen(s)>1) cerror(s);
	  c->operations[c->no++] = subtract;
	  break;
	case '*':
	  if (strlen(s)>1) cerror(s);
	  c->operations[c->no++] = multiply;
	  break;
	case '/':
	  if (strlen(s)>1) cerror(s);
	  c->operations[c->no++] = divide;
	  break;
	case 'c':
	  if (strcmp(s,"chs")==0) c->operations[c->no++] = change_sign;
	  else cerror(s);
	  break;
	case 'd':
	  if (strcmp(s,"dup")==0) c->operations[c->no++] = duplicate;
	  else cerror(s);
	  break;
	case 'e':
	  if (strcmp(s,"exp")==0) c->operations[c->no++] = exponential;
	  else if (strcmp(s,"exp2")==0) c->operations[c->no++] = exponential2;
	  else if (strcmp(s,"exp10")==0) c->operations[c->no++] = exponential10;
	  else if (strcmp(s,"exch")==0) c->operations[c->no++] = exchange;
	  else cerror(s);
	  break;
	case 'i':
	  if (strcmp(s,"if")==0) c->operations[c->no++] = if_func;
	  else cerror(s);
	  break;
	case 'l':
	  if (strcmp(s,"log")==0) c->operations[c->no++] = logarithm;
	  else if (strcmp(s,"log2")==0) c->operations[c->no++] = logarithm2;
	  else if (strcmp(s,"log10")==0) c->operations[c->no++] = logarithm10;
	  else cerror(s);
	  break;
	case 'm':
	  if (strcmp(s,"max")==0) c->operations[c->no++] = max_func;
	  else if (strcmp(s,"min")==0) c->operations[c->no++] = min_func;
	  else cerror(s);
	  break;
	case 'p':
	  if (strcmp(s,"pop")==0) c->operations[c->no++] = pop;
	  else if (strcmp(s,"pow")==0) c->operations[c->no++] = power;
	  else cerror(s);
	  break;
	case 'r':
	  if (strcmp(s,"rcl")==0) c->operations[c->no++] = recall;
	  else cerror(s);
	  break;
	case 's':
	  if (strcmp(s,"step")==0) c->operations[c->no++] = step_func;
	  else if (strcmp(s,"sto")==0) c->operations[c->no++] = store;
	  else if (strcmp(s,"sqrt")==0) c->operations[c->no++] = squareroot;
	  else cerror(s);
	  break;
	case '=':                // set value of variable
	  if ( strlen(s)!=2 || !isalpha(s[1]) ) cerror(s);
	  c->constants[c->nc++] = s[1];
	  c->operations[c->no++] = push;
	  c->operations[c->no++] = store; break;
	  break;
	default:
	  cerror(s);
	} // switch
      } // else
    } // else
    if (c->nc>=calc_max_size) error("Too many constants");
    if (c->no>=calc_max_size) error("Too many operations");
    s = next_token(NULL);
  } // while

  return c;
}


