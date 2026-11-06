CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDLIBS = -pthread

all: parent child1 child2

parent: parent.c shared.h
	$(CC) $(CFLAGS) -o parent parent.c $(LDLIBS)

child1: child1.c child_impl.h shared.h
	$(CC) $(CFLAGS) -o child1 child1.c $(LDLIBS)

child2: child2.c child_impl.h shared.h
	$(CC) $(CFLAGS) -o child2 child2.c $(LDLIBS)

clean:
	rm -f parent child1 child2 lr4_shared.dat strace.log strace.log.*

.PHONY: all clean
