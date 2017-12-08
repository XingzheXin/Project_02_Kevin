CFLAGS = -Wall -g
CC = gcc $(CFLAGS)
TARGETS= bl-server bl-client

all: $(TARGETS)

bl-server: server.o bl-server.o simpio.o util.o
	$(CC) -o bl-server server.o bl-server.o simpio.o util.o -lpthread
	@echo bl-server is ready

bl-client: bl-client.o simpio.o util.o
	$(CC) -o bl-client bl-client.o simpio.o util.o -lpthread
	@echo bl-client is ready

bl-client.o: bl-client.c simpio.c util.c blather.h
	$(CC) -c $<

bl-server.o: bl-server.c simpio.c util.c blather.h
	$(CC) -c $<

server.o: server.c blather.h
	$(CC) -c $<

util.o: util.c blather.h
	$(CC) -c $<

simpio.o: simpio.c blather.h
	$(CC) -c $<

clean:
	@echo Cleaning up object files
	rm -f *.o *.fifo

realclean:
	@echo Removing objects and programs
	rm -f *.o *.fifo bl-client bl-server
