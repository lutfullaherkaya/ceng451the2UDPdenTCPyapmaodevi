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
#include <semaphore.h>


#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <deque>

// todo: sil?
#define MAXBUFLEN 123



/**
 * In LDP, the packet size is constant. The 8 bytes of payload is always sent.
 */
#define LDP_PACKET_SIZE 15

/**
 * Window Size Calculation
 * Since Troll can only facilitate 16 packets atmost, the window size must be less than atmost 16.
 * Each packet will return as ACK. Therefore for each packet, there is only one packet in the
 * network at the same time, not taking into account the retransmissions from timeout.
 * As the Propagation Delay approaches infinity , the packet count in the network diverges to infinity
 * because every packet is lost and timed out, thus retransmitted. Therefore the timeout duration
 * must be more than double the propagation delay and the Propagation Delay must be finite.
 *
 * 12 seems to be a nice even number for the window size. Since the performance is directly
 * proportional to the window size, it is important for the window size to be as big as possible.
 *
 * Also the window size must not be larger than 15 bits because the sequence numbers are 2 bytes.
 *
 */
#define WINDOW_SIZE 12

/**
 * We can't queue infinite packets for sending.
 * todo: Crime book is 3.4MB. What happens if this huge input is inputted from stdin? Does fgets work properly or does it drop some lines? Try it.
 */
#define WINDOW_CONTAINER_MAX_SIZE 1024

/**
 * Since the maximum message count per packet is 1, then there won't be more than window size messages
 * at a time in the message queue. Make it + 1 just to be sure.
 */
#define MAX_REC_MESSAGE_Q_SIZE WINDOW_SIZE + 1


void *get_in_addr(struct sockaddr *sa);

// serialize source (mostly modified): https://stackoverflow.com/questions/1577161/passing-a-structure-through-sockets-in-c
void *serialize_uint32_t(void *buffer, uint32_t value);

void *deserialize_uint32_t(void *buffer, uint32_t *value);

void *serialize_uint16_t(void *buffer, uint16_t value);

void *deserialize_uint16_t(void *buffer, uint16_t *value);

void *serialize_uint8_t(void *buffer, uint8_t value);

void *deserialize_uint8_t(void *buffer, uint8_t *value);

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
    /**
     * The flags will be stored in one uint8_t.
     *
     * A packet can be both ack and data. If i have time, I will implement it and
     * acks and data will be in the same packet if it is efficient that way.
     *
     * Also, using the same implementation (waiting a little bit before sending), I can also
     * do cumulative acks. But this is the last day of the homework and I will probably not
     * have enough time for all that.
     */
    bool isAck;
    bool isSeq;
    /**
     * In my implementation, a message is text ending with a newline character and
     * messages are sent in packets. To know when a message ends in a possibly infinite stream
     * of packets, this flag is neccesary.
     */
    bool isTheLastPacketOfTheMessage;
    uint16_t seqNumber;
    uint16_t ackNumber;

    /**
     * May or may not end with null. Depends on the message.
     * Also LDP does not need SYN or FIN flags. The connection starts when the server
     * receives a message and ends when someone says BYE.
     */
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
     * @param data the encoded data
     * @param n the encoded data size
     */
    static LDPPacket decode(void *data, size_t n);


    static uint16_t calculateChecksum(void *data, size_t n);
};

class LDP {
public:
    // todo: tum semaphorelerin ve mutexlerin ve threadlerin ve falan filan destroy edildiginden emin ol
    std::string chateeIP;
    std::string chateePort;
    std::string myPort;
    struct sockaddr chateeSockaddr;
    bool isClient;
    bool isListening;
    int sockfd;
    void *(*onMessage)(void *, std::string &message);
    void *onMessageArg;

    std::queue<std::string> receivedMessageQueue;
    sem_t receivedMessageQueueFullSlotCount, receivedMessageQueueEmptySlotCount;
    pthread_mutex_t receivedMessageQueueMutex;

    std::deque<LDPPacket> senderWindow;
    sem_t senderWindowFullSlotCount, senderWindowEmptySlotCount;
    pthread_mutex_t senderWindowMutex;

    std::deque<LDPPacket> receiverWindow;
    sem_t receiverWindowFullSlotCount, receiverWindowEmptySlotCount;
    pthread_mutex_t receiverWindowMutex;

    bool getIsListening() const;

    void setIsListening(bool isListening);

    /**
     * todo: bir sey donebiliriz bye alinca.
     */
    void listen(void *(*onMessageEventHandler)(void *, std::string &message), void *arg);


    LDP(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient);

    LDP(const std::string &myPort, bool isClient);

    void initializeThreadMhreadEtcMtc();

    int createAndBindSocket();

    int computeChateeAddrInfo();

    /**
     * senderWindow producer
     */
    void send(std::string &message);



    std::string deliverData();

private:
    static void *ldpPacketReceiverHelper(void *context);
    void *ldpPacketReceiver();

    /**
     * senderWindow consumer
     * @return
     */
    void *ldpPacketSender();
    static void *ldpPacketSenderHelper(void *context);

    void *messageConsumer();
    static void *messageConsumerHelper(void *context);

    void udpSend(LDPPacket &packet);
    /**
     *
     * @return may receive corrupted packet with obj.isCorrupted=1
     */
    LDPPacket udpReceive();

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

    /**
     * source: https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
     */
    void *printMessage(std::string &message);

    static void *printMessageHelper(void *context, std::string &message);

    void endChat();

    ~Chatter();


};

#endif //THE2_LDP_H
