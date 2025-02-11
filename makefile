CFLAGS = -Wall -g # flags de compilacao
LDFLAGS = -lm
CC = gcc

all: LBP
LBP: lbp.o
	$(CC) -o LBP lbp.o $(LDFLAGS)

lbp.o: lbp.c
	$(CC) -c $(CFLAGS) lbp.c

clean:
	rm -f *.o *~ LBP
