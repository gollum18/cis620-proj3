CC=gcc
IFLAGS=-I.
CFLAGS=-g
EXEFILES=client server servicemap
OBJFILES=client.o server.o servicemap.o

all: $(EXEFILES)

client: client.o
	gcc -o client client.o

server: server.o
	gcc -o server server.o

servicemap: servicemap.o
	gcc -o servicemap servicemap.o

clean:
	rm $(EXEFILES) $(OBJFILES)
	
submit: 
	turnin -c cis620s -p proj3 report.pdf client.c server.c servicemap.c makefile
