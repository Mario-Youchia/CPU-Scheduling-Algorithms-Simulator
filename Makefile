CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: clk.out process.out scheduler.out process_generator.out test_generator.out

clk.out: clk.c headers.h
	$(CC) $(CFLAGS) -o clk.out clk.c

process.out: process.c headers.h
	$(CC) $(CFLAGS) -o process.out process.c

scheduler.out: scheduler.c headers.h
	$(CC) $(CFLAGS) -o scheduler.out scheduler.c -lm

process_generator.out: process_generator.c headers.h
	$(CC) $(CFLAGS) -o process_generator.out process_generator.c

test_generator.out: test_generator.c headers.h
	$(CC) $(CFLAGS) -o test_generator.out test_generator.c

clean:
	rm -f *.out scheduler.log scheduler.perf
