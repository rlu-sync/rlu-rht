URCUDIR ?= /usr/local

CC = gcc
LD = gcc

CFLAGS += -I$(URCUDIR)/include
CFLAGS += -D_REENTRANT
#CFLAGS += -DNDEBUG
CFLAGS += -g -mrtm -O3

CFLAGS += -Winline --param inline-unit-growth=1000 

IS_RCU = -DIS_RCU
IS_RLU = -DIS_RLU

LDFLAGS += -L$(URCUDIR)/lib
LDFLAGS += -lpthread

BINS = bench-rcu bench-rlu

.PHONY:	all clean

all: $(BINS)

atomics.o: atomics.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

htm.o: htm.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

rlu.o: rlu.c

new-urcu.o: new-urcu.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

hash-list-resize.o: hash-list-resize.c
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

bench-rcu.o: bench.c
	$(CC) $(CFLAGS) $(IS_RCU) $(DEFINES) -c -o $@ $<

bench-rlu.o: bench.c
	$(CC) $(CFLAGS) $(IS_RLU) $(DEFINES) -c -o $@ $<

bench-rcu: atomics.o htm.o new-urcu.o rlu.o hash-list-resize.o bench-rcu.o
	$(LD) -o $@ $^ $(LDFLAGS)

bench-rlu: atomics.o htm.o new-urcu.o rlu.o hash-list-resize.o bench-rlu.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINS) *.o
