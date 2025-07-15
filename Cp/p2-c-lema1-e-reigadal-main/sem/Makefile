
CC=gcc
CFLAGS=-Wall -pthread -g
LIBS=
OBJS=barber.o options.o sem.o

PROGS=barber

all: $(PROGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

barber: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(PROGS) *.o *~

