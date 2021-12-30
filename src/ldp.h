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

#include "utils.h"
#include "config_constants.h"

/**
 * In LDP, the packet size is constant. The 8 bytes of payload is always sent.
 */
#define LDP_PACKET_SIZE 15

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

    LDPPacket(const LDPPacket &packet);

    LDPPacket();

    LDPPacket(const char *data, size_t n, bool isAck, bool isSeq, bool isTheLastPacketOfTheMessage, uint16_t seqNumber,
              uint16_t ackNumber);

    LDPPacket(bool isAck, bool isSeq, bool isTheLastPacketOfTheMessage, uint16_t seqNumber,
              uint16_t ackNumber);

    void print(bool sending) const;


    std::string payloadToString();

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
/**
 * if isNULL: not received yet but expected to be
 * if not in window: not yet sent
 */
class LDPPacketInWindow {
public:
    LDPPacketInWindow(LDPPacket &p, bool isAck, uint16_t seqNumber);

    LDPPacketInWindow(const LDPPacketInWindow &p);

    LDPPacketInWindow();

    LDPPacketInWindow(uint16_t seqNum);

    LDPPacketInWindow &operator=(const LDPPacketInWindow &p);

    LDPPacketInWindow *make(LDPPacket &p, bool isAck, uint16_t seqNum);

    void shallowSwap(LDPPacketInWindow &p);

    bool isNull() const;


    ~LDPPacketInWindow();

    /**
     * Just to make this nullable
     * here I make it be on heap
     */
    LDPPacket *packet;
    uint16_t expectedSeqNumber;
    bool isACKd;
};

class PacketAndItsTimeout {
public:
    LDPPacketInWindow *packet;
    timespec sentTime;

    PacketAndItsTimeout(LDPPacketInWindow *packet, timespec sentTime);
    long timeUntilTimeout() const;

};

class LDP {
public:
    // todo: tum semaphorelerin ve mutexlerin ve threadlerin ve falan filan destroy edildiginden emin ol
    std::string chateeIP;
    std::string chateePort;
    std::string myPort;
    struct sockaddr chateeSockaddr;
    bool isClient;
    bool havePartnerIP;
    bool isListening;
    int sockfd;

    void *(*onMessage)(void *, std::string &message);

    void *onMessageArg;

    /**
     * The lifecycle of a packet:
     * todo: producer consumerlerini de yaz bunlarin.
     * packetsToBeSent -> senderWindow -> receiverWindow -> packetsReceived -> receivedMessageQueue
     */


    std::queue<LDPPacket> packetsToBeSent;
    sem_t packetsToBeSentFullSlotCount, packetsToBeSentEmptySlotCount;
    pthread_mutex_t packetsToBeSentMutex;

    std::deque<LDPPacketInWindow> senderWindow;
    sem_t senderWindowFullSlotCount, senderWindowEmptySlotCount;
    pthread_mutex_t senderWindowMutex;
    uint16_t seqNumOfTheLastPacketInSenderWind;

    /**
     * senderWindow consumer&producer
     *
     * @param ackNum
     */
    void ackAndSlideSenderWindow(uint16_t ackNum);


    /**
     * Receiver window is always full with null packets (i.e. expected but not yet received packets).
     */
    std::deque<LDPPacketInWindow> receiverWindow;
    sem_t receiverWindowFullSlotCount, receiverWindowEmptySlotCount;
    pthread_mutex_t receiverWindowMutex;
    uint16_t seqNumOfTheLastPacketInReceiverWind;

    /**
     * packetsReceived producer
     */
    void receiveAndSlideReceiverWindow(LDPPacket &p);

    std::queue<LDPPacket> packetsReceived;
    sem_t packetsReceivedFullSlotCount, packetsReceivedEmptySlotCount;
    pthread_mutex_t packetsReceivedMutex;


    pthread_mutex_t byeLock;
    long byeSeq;

    void printRecWindow() const;
    void printSendWindow() const;


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
     * packetsToBeSent producer
     */
    void sendMessage(std::string &message);

    void closeYourMouth();

    void closeYourEars();

private:
    /**
     * packetsToBeSent consumer
     * senderWindow producer
     * @return
     */
    void *ldpPacketSender();

    static void *ldpPacketSenderHelper(void *context);

    void *ldpPacketRetransmitter();

    static void *ldpPacketRetransmitterHelper(void *context);

    /**
     * note: received packets dont get buffered in my code. perhaps it is buffered on udp code, idk
     * but it isn't neccesary to buffer them since sender only sends packets on their same sized window.
     *
     * receiverWindow producer
     *
     * @return
     */
    void *ldpPacketReceiver();

    static void *ldpPacketReceiverHelper(void *context);

    /**
     * packetsReceived consumer
     */
    void *messageProducer();

    static void *messageProducerHelper(void *context);



    void udpSend(LDPPacket &packet);


    /**
     *
     * @return may receive corrupted packet with obj.isCorrupted=1
     */
    LDPPacket udpReceive();

};



#endif //THE2_LDP_H
