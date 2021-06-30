/*
This file is part of the aklib c library
Copyright 2016-2021 by Anders Krogh.
aklib licensed under the GPLv3, see the file LICENSE.
*/


#ifdef RPINTEGER
typedef int vartype;
#else
typedef double vartype;
#endif


#include <stdlib.h>

#define calc_max_size 100

typedef struct _calculator_ {
  int n;   // Stack size
  int nc;  // Number of constants
  int cc;  // Current constant
  int no;  // Number of operations
  vartype stack[calc_max_size];      // Stack
  vartype constants[calc_max_size];  // Numbers to be pushed on stack
  vartype variable[128];       // Any letter can be used as variable
  void (*operations[calc_max_size])(struct _calculator_ *c);
} calculator;

static inline int max_size_calculator(calculator *c) { return calc_max_size; }
static inline int stack_size_calculator(calculator *c) { return c->n; }

void store_calculator(calculator *c, int i, vartype value);
void set_const_calculator(calculator *c, char nm, vartype value);
vartype run_calculator(calculator *c);
vartype run_calculator_single(calculator *calc, vartype value);
calculator *compile_calculator(char *program);
void free_calculator(calculator *c);
void clear_stack_calculator(calculator *c);
vartype pop_stack_calculator(calculator *c);
void push_stack_calculator(calculator *c, vartype value);
void copy_stack_calculator(calculator *c, vartype *array, int n);
