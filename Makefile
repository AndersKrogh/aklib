# This file is part of the aklib c library
# Copyright 2016-2019 by Anders Krogh.
# aklib licensed under the GPLv3, see the file LICENSE.


# Use "make CFLAGS=-O3" or optimization 

#OFLAGS = -g
OFLAGS = -O3

CFLAGS  = $(OFLAGS) $(PROF) -Wall -Wno-unused-function  # Turn off warnings of unused funcs
# CFLAGS  = -O3 -Wall -Wno-unused-function  # Turn off warnings of unused funcs

VPATH = ./src

HFILESA = akstandard.h simpleHash.h sequence.h reversePolish.h inThreads.h kmers.h
OFILES = akstandard.o simpleHash.o sequence.o reversePolish.o inThreads.o kmers.o


ALL: libaklib.a aklib.h


# Create lib

libaklib.a: $(OFILES) $(HFILES)
	ar rcu $@ $(OFILES)
	ranlib $@

aklib.h: $(HFILESA)
	echo "#ifndef AKLIB_H" > $@
	echo "#define AKLIB_H" >> $@
	cat $+ >> $@
	echo "#endif" >> $@


# lib o files

inThreads.o: inThreads.c inThreads.h

simpleHash.o: simpleHash.c simpleHash.h

akstandard.o: akstandard.c akstandard.h

sequence.o: sequence.c sequence.h akstandard.h

reversePolish.o: reversePolish.c reversePolish.h

kmers.o: kmers.c kmers.h

clean:
	- rm -f *.o *~ src/*~ src/*.old

cleanall: clean
	- rm -f libaklib.a aklib.h



