/**
 * @file ldp.c
 * @author lutfullah erkaya
 * @brief LDP - Lutfullah Data Protocol. The best reliable data transmission protocol.
 * @version 0.1
 * @date 2021-12-18
 * special thanks to the code of udp client/server taken from beej guide: http://beej.us/guide/bgnet/html
 * @copyright Copyright (c) 2021
 *
 */

#include "ldp.h"


class LDP {

};

int Chatter::ana(int argc, char **argv) {


    return 0;
}




int Chatter::createSocket() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    struct addrinfo *myAddrInfoPtr;
    int rv;
    if ((rv = getaddrinfo(NULL, myPort.c_str(), &hints, &myAddrInfoPtr)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    struct addrinfo *p;
    // loop through all the results and bind to the first we can
    for (p = myAddrInfoPtr; p != NULL; p = p->ai_next) {
        if ((talkingSockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("chatter: socket");
            continue;
        }

        if (bind(talkingSockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(talkingSockfd);
            perror("chatter: bind");
            continue;
        }

        break;
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }
    myAddrInfo = *myAddrInfoPtr;
    freeaddrinfo(myAddrInfoPtr);
    return 0;
}

Chatter::Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : chateeIP(chateeIp), chateePort(chateePort), myPort(myPort), isClient(isClient) {
    createSocket();
    computeChateeAddrInfo();
    sendMessage("merhaba");
    close(talkingSockfd);

}

int Chatter::computeChateeAddrInfo() {
    if (isClient) {
        struct addrinfo hints, *chateeAddrInfoPtr;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;

        // rv is return value here and it returns code (error code or 0)
        int rv;
        if ((rv = getaddrinfo(chateeIP.c_str(), chateePort.c_str(), &hints, &chateeAddrInfoPtr)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
        struct addrinfo *p;
        // loop through all the results and make a socket
        for (p = chateeAddrInfoPtr; p != NULL; p = p->ai_next) {
            if (false) {
                // todo: i don't know at what condition would a wrong IP address would come
            } else {
                break;
            }
        }
        if (p == NULL) {
            fprintf(stderr, "talker: failed to getaddrinfo chatee address\n");
            return 2;
        }
        chateeAddrInfo = *chateeAddrInfoPtr;
        freeaddrinfo(chateeAddrInfoPtr);
    } else {
        // is server
    }
    return 0;
}

int Chatter::sendMessage(std::string message) {
    int numbytes;
    if ((numbytes = sendto(talkingSockfd, message.c_str(), message.length(), 0,
                           chateeAddrInfo.ai_addr, chateeAddrInfo.ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    printf("talker: sent %d bytes to %s\n", numbytes, chateeIP.c_str());
    return numbytes;
}
