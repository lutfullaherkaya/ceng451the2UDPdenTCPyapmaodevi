# Makefile for client and server

CC = g++
OBJCLI = client.cpp ldp.cpp chatter.cpp utils.cpp
OBJSRV = server.cpp ldp.cpp chatter.cpp utils.cpp
CPPFLAGS = -g
# setup for system
LIBS = -lpthread


all: client server



client:	$(OBJCLI)
	$(CC) $(CPPFLAGS) -o $@ $(OBJCLI) $(LIBS)

server:	$(OBJSRV)
	$(CC) $(CPPFLAGS) -o $@ $(OBJSRV) $(LIBS)

clean:
	rm client server
