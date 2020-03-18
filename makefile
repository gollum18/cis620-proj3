CC=gcc
CFLAGS=-g
IFLAGS=-I.

SRCFILES=client.c server.c servicemap.c
OBJFILES=client.o server.o servicemap.o
EXEFILES=client server servicemap

all: $(EXEFILES)

client: client.o
	gcc -o client client.o

server: server.o
	gcc -o server server.o

servicemap: servicemap.o
	gcc -o servicemap servicemap.o

clean:
	rm $(OBJFILES) $(EXEFILES)

depend: 
	makedepend $(IFLAGS) $(SRCFILES)
