/*
** talker.c -- a datagram "client" demo
* default execution: ./client 127.0.0.1 5200 5201
*/

#include "ldp.h"
#include "client.h"



int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr,"usage: ./client <server-ip> <server-port-number> <client-port-number> .\n");
        exit(1);
    }
    Chatter clientChatter(argv[1], argv[2], argv[3], true);
    return 0;
}
