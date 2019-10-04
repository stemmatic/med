CFLAGS=-g -O -Wall #-pg
LDFLAGS=-lm #-pg

BIN=~/bin

all::	$(BIN)/med ut

$(BIN)/med:	med
	ln -f med $(BIN)/med

SRCS=med.c norm.c lemma.c
OBJS=$(SRCS:%.c=%.o)

med:	$(OBJS)

.PHONY: ut
ut:	$(BIN)/med utest utout 
	$(BIN)/med <utest >_utout
	diff utout _utout

.PHONY: depend
depend:
	makedepend $(SRCS)

# DO NOT DELETE


lemma.o: lemma.h med.h norm.h
med.o: med.h
norm.o: lemma.h med.h norm.h
