/**
 * @file ldp.h
 * @author lutfullah erkaya
 * @brief LDP - Lutfullah Data Protocol. The best reliable data transmission protocol.
 * @version 0.1
 * @date 2021-12-18
 * special thanks to the code of udp client/server taken from beej guide: http://beej.us/guide/bgnet/html
 * @copyright Copyright (c) 2021
 *
 * Bismillahirrahmanirrahim.
 */

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
#include <pthread.h>

#include <string>
#include <vector>

// todo: sil?
#define MAXBUFLEN 123

#define LDP_PACKET_SIZE 10

void *get_in_addr(struct sockaddr *sa);

// serialize source (mostly modified): https://stackoverflow.com/questions/1577161/passing-a-structure-through-sockets-in-c
void *serialize_uint32_t(void *buffer, uint32_t value);

void *deserialize_uint32_t(void *buffer, uint32_t *value);

void *serialize_uint16_t(void *buffer, uint16_t value);

void *deserialize_uint16_t(void *buffer, uint16_t *value);

/**
 *
 * @param buffer
 * @param str perhaps not null terminated string
 * @param n
 * @return
 */
void *serialize_char_array(void *buffer, char *str, size_t n);

/**
 *
 * @param buffer
 * @param str the output char array
 * @param n
 * @return
 */
void *deserialize_char_array(void *buffer, char *str, size_t n);

void *serialize_char(void *buffer, char value);

void *deserialize_char(void *buffer, char *value);

class LDPPacket {
public:
    // PACKET START ------------------------------------------------
    /**
     * the checksum field of the decoded packet.
     * calculated when sending and deserialized from the packet data when receiving.
     * note: is is not 16 byte asdfasdfasd it is 16 bit, so 2 byte.
     */
    uint16_t checksum;

    char payload[8];

    // PACKET END ----------------------------------------

    bool isCorrupted;

    LDPPacket();


    LDPPacket(const char *data, size_t n);

    /**
     *
     * @param data the output
     * @return the output byte count
     */
    size_t encode(void *data);

    /**
     * decodes the data and returns the packet object
     * @param data encoded data
     * @param n size
     */
    static LDPPacket decode(void *data, size_t n);


    static uint16_t calculateChecksum(void *data, size_t n);
};

class LDP {
public:
    std::string chateeIP;
    std::string chateePort;
    std::string myPort;
    struct sockaddr chateeSockaddr;
    /**
     * client starts the connection
     */
    bool isClient;
    int sockfd;

    LDP(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient);

    LDP(const std::string &myPort, bool isClient);

    /**
     * also computes myAddrInfo
     * @return
     */
    int createAndBindSocket();

    int computeChateeAddrInfo();

    void send(std::string &message);

    void udpSend(LDPPacket &packet);

    LDPPacket makePkt(void *data, size_t n);

    /**
     *
     * @return may receive corrupted packet with obj.isCorrupted=1
     */
    LDPPacket receive();

    std::string deliverData();



};

class Chatter {
private:
    bool isListening;
public:
    LDP ldp;
    bool getIsListening();
    void setIsListening(bool isListening);

    bool isClient;
    pthread_mutex_t ioLock;
    pthread_mutex_t isListeningLock;

    Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient);

    Chatter(const std::string &myPort, bool isClient);

    int initializeMutexes();

    int destroyMutexes();

    void initiateChat();

    void sendMessage(std::string message);

    std::string receiveMessage();

    void endChat();

    // source: https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    void *listener();

    static void *listenerHelper(void *context);

    void speaker();

    virtual ~Chatter();


};

#endif //THE2_LDP_H
