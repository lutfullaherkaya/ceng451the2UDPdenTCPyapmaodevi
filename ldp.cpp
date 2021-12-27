/**
 * @file ldp.c
 * @author lutfullah erkaya
 * @brief LDP - Lutfullah Data Protocol. The best reliable data transmission protocol.
 * @version 0.1
 * @date 2021-12-18
 * special thanks to the code of udp client/server taken from beej guide: http://beej.us/guide/bgnet/html
 * @copyright Copyright (c) 2021
 *
 * Bismillahirrahmanirrahim.
 */

#include "ldp.h"


class LDP {

};


void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}


int Chatter::createAndBindSocket() {
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
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("chatter: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("chatter: bind");
            continue;
        }

        break;
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(myAddrInfoPtr);
    return 0;
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
        chateeSockaddr = *(chateeAddrInfoPtr->ai_addr);
        freeaddrinfo(chateeAddrInfoPtr);
    } else {
        // client and server also compute it when receiveMessage is called.
    }
    return 0;
}

int Chatter::sendMessage(std::string message) {
    int numbytes;
    if ((numbytes = sendto(sockfd, message.c_str(), message.length(), 0,
                           &chateeSockaddr, sizeof(chateeSockaddr))) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    return numbytes;
}


std::string Chatter::receiveMessage() {
    // it is originally writen to be ipv4-ipv6 agnostic but I don't want to change it to ipv4
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];


    addr_len = sizeof their_addr;
    int numbytes;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }


    /**
     * computeChateeAddrInfo else part
     */
    chateeSockaddr = *((struct sockaddr *) &their_addr);
    char s[INET6_ADDRSTRLEN];
    std::string sender = inet_ntop(their_addr.ss_family,
                                   get_in_addr((struct sockaddr *) &their_addr),
                                   s, sizeof s);
    buf[numbytes] = '\0';


    return buf;
}

Chatter::Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : chateeIP(chateeIp), chateePort(chateePort), myPort(myPort), isClient(isClient) {
    // client
    /*initializeMutexes();*/
    createAndBindSocket();
    computeChateeAddrInfo();

    setIsListening(true);
    pthread_t listenerThreadID;
    pthread_create(&listenerThreadID, NULL, &Chatter::listenerHelper, this);

    while (getIsListening()) {
        sendMessage(getInput());
    }

    pthread_join(listenerThreadID, NULL);
    close(sockfd);

}

Chatter::Chatter(const std::string &myPort, bool isClient) : myPort(myPort), isClient(isClient) {
    // server
    initializeMutexes();
    createAndBindSocket();
    receiveMessage();

    setIsListening(true);
    pthread_t listenerThreadID;
    pthread_create(&listenerThreadID, NULL, &Chatter::listenerHelper, this);

    while (getIsListening()) {
        sendMessage(getInput());
    }

    close(sockfd);
}

void Chatter::initiateChat() {

}

void Chatter::endChat() {
    // todo: gerekli ack mack falan filan yapilacak
    setIsListening(false);

    printf("The Chat has been terminated.\n");
}

void *Chatter::listener() {
    printf("listener: waiting to recvfrom...\n");
    std::string message;
    do {
        message = receiveMessage();
        printf(ANSI_COLOR_YELLOW "chatee: %s" ANSI_COLOR_RESET, message.c_str());
    } while (message != "BYE");
    endChat();
    return NULL;
}

void Chatter::speaker() {

}

void *Chatter::listenerHelper(void *context) {
    return ((Chatter *) context)->listener();
}

bool Chatter::getIsListening() {
    pthread_mutex_lock(&isListeningLock);
    bool isListening1 = isListening;
    pthread_mutex_unlock(&isListeningLock);
    return isListening1;
}

void Chatter::setIsListening(bool isListening1) {
    pthread_mutex_lock(&isListeningLock);
    isListening = isListening1;
    pthread_mutex_unlock(&isListeningLock);
}

Chatter::~Chatter() {
    destroyMutexes();
}

int Chatter::initializeMutexes() {
    pthread_mutex_init( &(ioLock), NULL);
    pthread_mutex_init( &(isListeningLock), NULL);
    return 0;
}

int Chatter::destroyMutexes() {
    pthread_mutex_destroy(&ioLock);
    pthread_mutex_destroy(&isListeningLock);
    return 0;
}

std::string Chatter::getInput() {
    // todo: nonblocking io yapman lazim. bunlara mutex yapinca yazarken gelen mesajlar gozukmuyor. yapmayinca s1k1nt1 olabilir.
    char message[MAXBUFLEN];
    printf(ANSI_COLOR_GREEN);
    fgets(message, MAXBUFLEN, stdin);
    printf(ANSI_COLOR_RESET);

    return message;
}
