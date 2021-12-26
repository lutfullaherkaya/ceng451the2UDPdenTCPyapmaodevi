#ifndef THE2_LDP_H
#define THE2_LDP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>

class Chatter {
public:
    std::string chateeIP;
    std::string chateePort;
    std::string myPort;
    struct addrinfo myAddrInfo;
    struct addrinfo chateeAddrInfo;
    bool isClient;
    int talkingSockfd;
    int listeningSockfd;

    Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient);

    int ana(int argc, char *argv[]);
    /**
     * also computes myAddrInfo
     * @return
     */
    int createSocket();
    int computeChateeAddrInfo();
    int sendMessage(std::string message);

};

#endif //THE2_LDP_H
