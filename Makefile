# Makefile for client and server

CC = g++
OBJCLI = client.cpp ldp.cpp
OBJSRV = server.cpp ldp.cpp
CFLAGS = 
# setup for system
LIBS = -lpthread

all: client server

client:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)

server:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)

clean:
	rm client server
