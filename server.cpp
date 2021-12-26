/*
** listener.c -- a datagram sockets "server" demo
* default execution: ./server 5202
* troll: ./troll -C 127.0.0.1 -S 127.0.0.1 -a 5202 -b 5201 5200
*/

#include "ldp.h"
#include "server.h"


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: ./server <server-port-number>\n");
        exit(1);
    }
    Chatter clientChatter(argv[1], false);
    return 0;
}
