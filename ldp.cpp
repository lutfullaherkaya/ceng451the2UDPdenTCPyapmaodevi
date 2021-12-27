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

// todo: turkce karakterler s1k1nt1l1 cunku galiba utf-8 oldugu icin birden fazla baytli

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

void Chatter::sendMessage(std::string message) {
    ldp.send(message);
}


std::string Chatter::receiveMessage() {
    return ldp.deliverData();
}


Chatter::Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : isClient(isClient), ldp(chateeIp, chateePort, myPort, isClient) {
    // client
    initializeMutexes();
    ldp.createAndBindSocket();
    ldp.computeChateeAddrInfo();

    setIsListening(true);
    pthread_t listenerThreadID;
    pthread_create(&listenerThreadID, NULL, &Chatter::listenerHelper, this);

    while (getIsListening()) {
        char message[MAXBUFLEN];
        fgets(message, MAXBUFLEN, stdin);
        sendMessage(message);
    }

    pthread_join(listenerThreadID, NULL);
    destroyMutexes();
}

Chatter::Chatter(const std::string &myPort, bool isClient) : isClient(isClient), ldp(myPort, isClient) {
    // server
    initializeMutexes();
    ldp.createAndBindSocket();

    std::string connectionStartedMessage = receiveMessage();
    printf("CONNECTION ESTABLISHED. \nchatee: %s\n", connectionStartedMessage.c_str());

    setIsListening(true);
    pthread_t listenerThreadID;
    pthread_create(&listenerThreadID, NULL, &Chatter::listenerHelper, this);

    while (getIsListening()) {
        char message[MAXBUFLEN];
        fgets(message, MAXBUFLEN, stdin);
        sendMessage(message);
    }

    destroyMutexes();
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
        printf("chatee: %s\n", message.c_str());
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

    packet.isCorrupted = packet.checksum != calculateChecksum(((int16_t *) startOfDataPtr) + 1, LDP_PACKET_SIZE - 2);
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

LDPPacket::LDPPacket(const char *message, size_t n) : isCorrupted(false) {
    int i;
    for (i = 0; i < 8 && i < n; ++i) {
        payload[i] = message[i];
    }
    if (i < 8) {
        payload[i] = '\0';
    }
}

int LDP::createAndBindSocket() {
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

int LDP::computeChateeAddrInfo() {
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

LDP::LDP(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : chateeIP(chateeIp), chateePort(chateePort), myPort(myPort), isClient(isClient) {
    // client


    // TODO: chat bitince yapilsin bu baska yerde
    /*close(sockfd);*/

}

LDP::LDP(const std::string &myPort, bool isClient) : myPort(myPort), isClient(isClient) {
    // server

}


std::string LDP::deliverData() {
    // it is originally writen to be ipv4-ipv6 agnostic but I don't want to change it to ipv4

    LDPPacket packet = receive();
    std::string returnval;
    if (packet.isCorrupted) {
        returnval = "(corrupted string): ";
    }
    returnval += std::string(packet.payload);
    return returnval;
}

LDPPacket LDP::receive() {
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    unsigned char buf[LDP_PACKET_SIZE];


    addr_len = sizeof their_addr;
    int numbytes;
    if ((numbytes = recvfrom(sockfd, buf, LDP_PACKET_SIZE, 0,
                             (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    // todo: maybe should check if it is the same person sending the mesages. also no need for this at all for client.
    chateeSockaddr = *((struct sockaddr *) &their_addr);
    char s[INET6_ADDRSTRLEN];
    std::string sender = inet_ntop(their_addr.ss_family,
                                   get_in_addr((struct sockaddr *) &their_addr),
                                   s, sizeof s);
    return LDPPacket::decode(buf, 8);
}

void LDP::send(std::string &message) {
    std::vector<LDPPacket> packets;
    /**
     * I need my last packet to be null terminated and
     * the c_str pointer ensures that the string is null terminated.
     * The +1's are also for this reason there.
     */
    const char *messagePtr = message.c_str();
    for (int i = 0; i < message.length() + 1; i += 8) {
        // Note: no need to limit n to 8 because the constructor reads 8 bytes maximum.
        packets.push_back(LDPPacket(messagePtr + i, message.length() - i + 1));

    }
    for (int i = 0; i < packets.size(); ++i) {
        udpSend(packets[i]);
    }
}

void LDP::udpSend(LDPPacket &packet) {
    char encodedMessage[LDP_PACKET_SIZE];
    packet.encode(encodedMessage);

    int numbytes;
    if ((numbytes = sendto(sockfd, encodedMessage, LDP_PACKET_SIZE, 0,
                           &chateeSockaddr, sizeof(chateeSockaddr))) == -1) {
        perror("talker: sendto");
        exit(1);
    }
}

