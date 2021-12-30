# Makefile for client and server

CC = g++
OBJCLI = client.cpp ldp.cpp chatter.cpp utils.cpp
OBJSRV = server.cpp ldp.cpp chatter.cpp utils.cpp
CPPFLAGS = -g
# setup for system
LIBS = -lpthread
DEPS = client.h ldp.h chatter.h utils.h config_constants.h

all: client server

client:	$(OBJCLI) $(DEPS)
	$(CC) $(CPPFLAGS) -o ../obj-files-and-benchmark/client $(OBJCLI) $(LIBS)

server:	$(OBJSRV) $(DEPS)
	$(CC) $(CPPFLAGS) -o ../obj-files-and-benchmark/server $(OBJSRV) $(LIBS)

clean:
	rm client server
