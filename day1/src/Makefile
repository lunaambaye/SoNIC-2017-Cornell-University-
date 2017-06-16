all: parser

CFLAGS = -Wall -O2
CC = gcc $(CFLAGS)

util.o: util.c
	$(CC) -c util.c

parser: parser.c util.o
	$(CC) -o $@ util.o parser.c -lpcap

clean:
	rm -fr *.pyc *.o *~ parser *.dSYM
