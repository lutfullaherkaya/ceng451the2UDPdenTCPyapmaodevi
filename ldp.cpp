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


void *serialize_uint32_t(void *buffer, uint32_t value) {
    *((uint32_t *) buffer) = htonl(value);
    return ((uint32_t *) buffer) + 1;
}

void *deserialize_uint32_t(void *buffer, uint32_t *value) {
    *value = ntohl(*((uint32_t *) buffer));
    return ((uint32_t *) buffer) + 1;
}

void *serialize_uint16_t(void *buffer, uint16_t value) {
    *((uint16_t *) buffer) = htons(value);
    return ((uint16_t *) buffer) + 1;
}

void *deserialize_uint16_t(void *buffer, uint16_t *value) {
    *value = ntohs(*((uint16_t *) buffer));
    return ((uint16_t *) buffer) + 1;
}

/**
 *
 * @param buffer
 * @param str perhaps not null terminated string
 * @param n
 * @return
 */
void *serialize_char_array(void *buffer, char *str, size_t n) {
    for (int i = 0; i < n; ++i) {
        ((char *) buffer)[i] = str[i];
    }
    return ((char *) buffer) + n;
}

/**
 *
 * @param buffer
 * @param str the output char array
 * @param n
 * @return
 */
void *deserialize_char_array(void *buffer, char *str, size_t n) {
    for (int i = 0; i < n; ++i) {
        str[i] = ((char *) buffer)[i];
    }
    return ((char *) buffer) + n;
}

void *serialize_char(void *buffer, char value) {
    ((char *) buffer)[0] = value;
    return ((char *) buffer) + 1;
}

void *deserialize_char(void *buffer, char *value) {
    *value = ((char *) buffer)[0];
    return ((char *) buffer) + 1;
}


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
    char messageArr[MAXBUFLEN];
    for (int i = 0; i < message.length(); ++i) {
        messageArr[i] = message[i];
    }
    LDPPacket packet(messageArr, message.length());

    char encodedMessage[LDP_PACKET_SIZE];
    packet.encode(encodedMessage);

    int numbytes;
    if ((numbytes = sendto(sockfd, encodedMessage, LDP_PACKET_SIZE, 0,
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

    LDPPacket packet = LDPPacket::decode(buf, 8);
    std::string returnval;
    if (packet.isCorrupted) {
        returnval = "(corrupted string): ";
    }
    returnval += std::string(packet.payload);
    return returnval;
}

Chatter::Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : chateeIP(chateeIp), chateePort(chateePort), myPort(myPort), isClient(isClient) {
    // client
    initializeMutexes();
    createAndBindSocket();
    computeChateeAddrInfo();

    setIsListening(true);
    pthread_t listenerThreadID;
    pthread_create(&listenerThreadID, NULL, &Chatter::listenerHelper, this);

    while (getIsListening()) {
        sendMessage(getInput());
    }

    pthread_join(listenerThreadID, NULL);
    destroyMutexes();
    close(sockfd);

}

Chatter::Chatter(const std::string &myPort, bool isClient) : myPort(myPort), isClient(isClient) {
    // server
    initializeMutexes();
    createAndBindSocket();
    std::string message = receiveMessage();
    printf(ANSI_COLOR_YELLOW "CONNECTION ESTABLISHED. \nchatee: %s" ANSI_COLOR_RESET "\n", message.c_str());

    setIsListening(true);
    pthread_t listenerThreadID;
    pthread_create(&listenerThreadID, NULL, &Chatter::listenerHelper, this);

    while (getIsListening()) {
        sendMessage(getInput());
    }

    destroyMutexes();
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
        printf(ANSI_COLOR_YELLOW "chatee: %s" ANSI_COLOR_RESET "\n", message.c_str());
    } while (message != "BYE\n");
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
    pthread_mutex_init(&(ioLock), NULL);
    pthread_mutex_init(&(isListeningLock), NULL);
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

/**
 * source: https://cse.usf.edu/~kchriste/tools/checksum.c
 */
uint16_t LDPPacket::calculateChecksum(void *data, size_t n) {
    // Note: I don't include psuedo ip header in the checksum.
    // It is LDP implementation choice. It is not because I don't want to do any more work at all

    uint32_t sum = 0;

    uint16_t *dataPtr16 = (uint16_t *) data;
    while (n > 1) {
        sum += *dataPtr16;
        dataPtr16++;
        n -= 2;
    }

    // Add left-over byte, if any
    if (n > 0) {
        sum += ((uint8_t *) data)[n - 1];
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (~sum);
}


LDPPacket::LDPPacket() : isCorrupted(false) {}

LDPPacket LDPPacket::decode(void *data, size_t n) {
    LDPPacket packet;
    void *startOfDataPtr = data;
    data = deserialize_uint16_t(data, &(packet.checksum));

    data = deserialize_char_array(data, packet.payload, 8);

    packet.isCorrupted = packet.checksum != calculateChecksum(((int16_t*)startOfDataPtr)+1, LDP_PACKET_SIZE-2);
    return packet;
}

size_t LDPPacket::encode(void *data) {
    // the first 2 bytes is checksum, serialized at the end
    void *startOfDataPtr = data;
    data = ((uint16_t *) startOfDataPtr) + 1;

    data = serialize_char_array(data, payload, 8);

    checksum = calculateChecksum(((uint16_t *) startOfDataPtr) + 1, LDP_PACKET_SIZE - 2);
    serialize_uint16_t(startOfDataPtr, checksum);

    return LDP_PACKET_SIZE;
}

LDPPacket::LDPPacket(char *message, size_t n) : isCorrupted(false) {
    int i;
    for (i = 0; i < 8 && i < n; ++i) {
        payload[i] = message[i];
    }
    if (i < 8) {
        payload[i] = '\0';
    }
}
