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
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
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

    if (isClient) {
        computeChateeAddrInfo();
        havePartnerIP = true;
    }
    initializeThreadMhreadEtcMtc();
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
        if (0 != (rv = getaddrinfo(chateeIP.c_str(), chateePort.c_str(), &hints, &chateeAddrInfoPtr))) {
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
        : chateeIP(chateeIp), chateePort(chateePort), myPort(myPort), isClient(isClient), havePartnerIP(false),
          packetsToBeSent(PACKETS_TO_BE_SENT_Q_SIZE), senderWindow(SENDER_WINDOW_SIZE),
          seqNumOfTheLastPacketInSenderWind(0),
          receiverWindow(RECEIVER_WINDOW_SIZE), seqNumOfTheLastPacketInReceiverWind(0),
          packetsReceived(PACKETS_RECEIVED_Q_SIZE) {
    // client

}

LDP::LDP(const std::string &myPort, bool isClient) : myPort(myPort), isClient(isClient), havePartnerIP(false),
                                                     packetsToBeSent(PACKETS_TO_BE_SENT_Q_SIZE),
                                                     senderWindow(SENDER_WINDOW_SIZE),
                                                     seqNumOfTheLastPacketInSenderWind(0),
                                                     receiverWindow(RECEIVER_WINDOW_SIZE),
                                                     seqNumOfTheLastPacketInReceiverWind(0),
                                                     packetsReceived(PACKETS_RECEIVED_Q_SIZE) {
    // server
}


LDPPacket LDP::udpReceive() {
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    unsigned char buf[LDP_PACKET_SIZE];


    addr_len = sizeof their_addr;
    ssize_t numbytes = recvfrom(sockfd, buf, LDP_PACKET_SIZE, 0, (struct sockaddr *) &their_addr, &addr_len);
    if (numbytes == -1) {
        perror("recvfrom");
        exit(1);
    }

    // todo: maybe should check if it is the same person sending the mesages. also no need for this at all for client.
    chateeSockaddr = *((struct sockaddr *) &their_addr);
    havePartnerIP = true;
    char s[INET6_ADDRSTRLEN];
    std::string sender = inet_ntop(their_addr.ss_family,
                                   get_in_addr((struct sockaddr *) &their_addr),
                                   s, sizeof s);
    return LDPPacket::decode(buf, 8);
}

/**
 * packetsToBeSent producer
 *
 * Important note: pushing directly to the window deque unneccesarily makes the window busy.
 * Instead I should keep the packets in line on another queue, and when window deque gets low in packets,
 * then I cant add more packets.
 *
 * @param message
 */
void LDP::sendMessage(std::string &message) {
    std::vector<LDPPacket> packets;
    /**
     * I need my last packet to be null terminated and
     * the c_str() function ensures that the string is null terminated I think reading the docs of the function.
     * The +1's are also for this reason there.
     *
     * We do the calculations before accessing the critic queue in hope for not busying the senderWindow thread.
     */
    const char *messagePtr = message.c_str();
    for (int i = 0; i < message.length() + 1; i += 8) {
        packetsToBeSent.waitAndDecrementEmpty()->lock();

        bool isTheLast = i + 8 > message.length();
        packetsToBeSent.deq.emplace_back(messagePtr + i, message.length() - i + 1, false, true, isTheLast, 0, 0);

        packetsToBeSent.signalAndIncrementFull()->unlock();

    }
}

void LDP::udpSend(LDPPacket &packet) {
    char encodedMessage[LDP_PACKET_SIZE];
    packet.encode(encodedMessage);

    packet.print(true);
    ssize_t numbytes = sendto(sockfd, encodedMessage, LDP_PACKET_SIZE, 0, &chateeSockaddr, sizeof(chateeSockaddr));
    if (numbytes == -1) {
        perror("talker: sendto");
        exit(1);
    }
}

void *LDP::ldpPacketReceiverHelper(void *context) {
    return ((LDP *) context)->ldpPacketReceiver();
}

void *LDP::ldpPacketReceiver() {
    while (true) {
        LDPPacket packet = udpReceive();
        if (!packet.isCorrupted) {
            packet.print(false);
            if (packet.isAck) {
                pthread_mutex_lock(&byeLock);
                if (packet.ackNumber == byeSeq) {
                    LDPPacket ackPacket(true, false, false, 0,
                                        packet.seqNumber);
                    udpSend(ackPacket);
                }
                if (packet.payload[0] == 0 && packet.payload[1] == 'b') {
                    // ack to ack to bye
                    exit(0);
                }
                pthread_mutex_unlock(&byeLock);

                fprintf(stderr, "Received ACK%d.\n", packet.ackNumber);
                ackAndSlideSenderWindow(packet.ackNumber);
            }
            if (packet.isSeq) {
                receiveAndSlideReceiverWindow(packet);
                LDPPacket ackPacket(true, false, false, 0,
                                    packet.seqNumber);
                udpSend(ackPacket);
            }
        } else {
            /*fprintf(stderr, "Received and dropped garbled packet. \n");*/
        }

    }
    return NULL;
}

void *LDP::ldpPacketSenderHelper(void *context) {
    return ((LDP *) context)->ldpPacketSender();
}

void *LDP::ldpPacketSender() {
    while (true) {
        LDPPacketInWindow packet;

        while (!havePartnerIP) {
            usleep(1000);
        }

        // consume packetsToBeSent
        packetsToBeSent.waitAndDecrementFull()->lock();

        packet.make(packetsToBeSent.deq.front(), false, 0);
        packetsToBeSent.deq.pop_front();

        packetsToBeSent.signalAndIncrementEmpty()->unlock();

        // produce senderWindow and send packet
        senderWindow.waitAndDecrementEmpty()->lock();

        if (packet.packet->isSeq) {
            packet.packet->seqNumber = ++seqNumOfTheLastPacketInSenderWind;
        }

        if (packet.packet->payloadToString() == "BYE\n") {
            pthread_mutex_lock(&byeLock);
            byeSeq = packet.packet->seqNumber;
            pthread_mutex_unlock(&byeLock);
        }

        udpSend(*(packet.packet));
        senderWindow.deq.emplace_back();
        senderWindow.deq.back().shallowSwap(packet);

        senderWindow.signalAndIncrementFull()->unlock();

    }
    return NULL;
}

void *LDP::ldpPacketRetransmitterHelper(void *context) {
    return ((LDP *) context)->ldpPacketRetransmitter();
}


void LDP::initializeThreadMhreadEtcMtc() {
    pthread_mutex_init(&byeLock, NULL);
    byeSeq = -1;

    for (int i = 0; i < RECEIVER_WINDOW_SIZE; ++i) {
        seqNumOfTheLastPacketInReceiverWind++;
        receiverWindow.deq.emplace_back(seqNumOfTheLastPacketInReceiverWind);
    }

    pthread_create(&ldpPacketSenderThreadID, NULL, &LDP::ldpPacketSenderHelper, this);
    pthread_create(&ldpPacketRetransmitterThreadID, NULL, &LDP::ldpPacketRetransmitterHelper, this);

    /*pthread_join(listenerThreadID, NULL);*/

    /*close(sockfd);*/
}

void LDP::listen(void *(*onMessageEventHandler)(void *, std::string &), void *arg) {
    onMessage = onMessageEventHandler;
    onMessageArg = arg;


    pthread_create(&listenerThreadID, NULL, &LDP::ldpPacketReceiverHelper, this);
    pthread_create(&messageProducerThreadID, NULL, &LDP::messageProducerHelper, this);


    // todo: join and finish clean up threads
}

void *LDP::messageProducer() {
    while (true) {
        std::string message;
        char *payload;
        bool lastPacketOfTheMessageReceived = false;
        while (!lastPacketOfTheMessageReceived) {
            packetsReceived.waitAndDecrementFull()->lock();

            LDPPacket &packet = packetsReceived.deq.front();
            message += packet.payloadToString();
            lastPacketOfTheMessageReceived = packet.isTheLastPacketOfTheMessage;

            packetsReceived.deq.pop_front();

            packetsReceived.signalAndIncrementEmpty()->unlock();
        }
        (*onMessage)(onMessageArg, message);
    }
    return NULL;
}

void *LDP::messageProducerHelper(void *context) {
    return ((LDP *) context)->messageProducer();
}

void LDP::ackAndSlideSenderWindow(uint16_t ackNum) {

    senderWindow.lock();
    // ack
    for (int i = 0; i < senderWindow.deq.size(); ++i) {
        if (senderWindow.deq[i].packet->seqNumber == ackNum) {
            senderWindow.deq[i].isACKd = true;
            break;
        }
    }
    // slide
    while (!senderWindow.deq.empty()) {
        if (senderWindow.deq.front().isACKd) {
            senderWindow.waitAndDecrementFull();

            // packet is deleted now.
            senderWindow.deq.pop_front();
            senderWindow.signalAndIncrementEmpty();
        } else {
            break;
        }
    }
    senderWindow.unlock();
}

void LDP::receiveAndSlideReceiverWindow(LDPPacket &p) {
    receiverWindow.lock();
    // receive
    /**
     * If packet is expected but not yet received or acceptable within window
     * note: receiverWindow will never be full because it is bounded by senderWindow
     * and the receiverWindow is bigger than senderWindow, therefore this operation will not
     * block the ldpPacketReceiver from taking acks. However receiverWindow is always full with
     * null packets with expected sequence numbers.
     */
    for (int i = 0; i < receiverWindow.deq.size(); ++i) {
        if (receiverWindow.deq[i].expectedSeqNumber == p.seqNumber) {
            if (receiverWindow.deq[i].isNull()) {
                receiverWindow.deq[i].make(p, true, receiverWindow.deq[i].expectedSeqNumber);
            } else {
                fprintf(stderr, "duplicate payload: %s \n", p.payloadToString().c_str());
            }

            break;
        }
    }
    // if loop finishes then the packet is duplicate from older windows.

    // slide
    while (!(receiverWindow.deq.front().isNull())) {
        packetsReceived.waitAndDecrementEmpty()->lock();

        packetsReceived.deq.push_back(*(receiverWindow.deq.front().packet));

        packetsReceived.signalAndIncrementFull()->unlock();

        receiverWindow.deq.pop_front();
        seqNumOfTheLastPacketInReceiverWind++;
        receiverWindow.deq.emplace_back(seqNumOfTheLastPacketInReceiverWind);


    }
    receiverWindow.unlock();
}

void *LDP::ldpPacketRetransmitter() {
    while (true) {
        senderWindow.lock();

        for (int i = 0; i < senderWindow.deq.size(); ++i) {
            if (!senderWindow.deq[i].isACKd) {
                LDPPacket packet = *senderWindow.deq[i].packet;
                udpSend(packet);
            }
        }
        senderWindow.unlock();

        sleepMs(TIMEOUT_DURATION_MS);
    }


    return NULL;
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
        sum += ((uint8_t *) dataPtr16)[n - 1];
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

    uint8_t flags;
    data = deserialize_uint8_t(data, &flags);
    packet.isAck = flags & 0x1;
    packet.isSeq = (flags >> 1) & 0x1;
    packet.isTheLastPacketOfTheMessage = (flags >> 2) & 0x1;

    data = deserialize_uint16_t(data, &packet.seqNumber);
    data = deserialize_uint16_t(data, &packet.ackNumber);
    data = deserialize_char_array(data, packet.payload, 8);

    packet.isCorrupted = packet.checksum != calculateChecksum(((int16_t *) startOfDataPtr) + 1, LDP_PACKET_SIZE - 2);
    return packet;
}

size_t LDPPacket::encode(void *data) {
    // the first 2 bytes is checksum, serialized at the end
    void *startOfDataPtr = data;
    data = ((uint16_t *) startOfDataPtr) + 1;

    uint8_t flags = 0;
    flags |= isAck;
    flags |= isSeq << 1;
    flags |= isTheLastPacketOfTheMessage << 2;

    data = serialize_uint8_t(data, flags);
    data = serialize_uint16_t(data, seqNumber);
    data = serialize_uint16_t(data, ackNumber);
    data = serialize_char_array(data, payload, 8);

    checksum = calculateChecksum(((uint16_t *) startOfDataPtr) + 1, LDP_PACKET_SIZE - 2);
    serialize_uint16_t(startOfDataPtr, checksum);

    return LDP_PACKET_SIZE;
}


LDPPacket::LDPPacket(const LDPPacket &p1) {
    checksum = p1.checksum;
    isAck = p1.isAck;
    isSeq = p1.isSeq;
    isTheLastPacketOfTheMessage = p1.isTheLastPacketOfTheMessage;
    seqNumber = p1.seqNumber;
    ackNumber = p1.ackNumber;
    for (int i = 0; i < 8; ++i) {
        payload[i] = p1.payload[i];
    }
    isCorrupted = p1.isCorrupted;
}

LDPPacket::LDPPacket(const char *data, size_t n, bool isAck, bool isSeq, bool isTheLastPacketOfTheMessage,
                     uint16_t seqNumber, uint16_t ackNumber)
        : isAck(isAck), isSeq(isSeq), isTheLastPacketOfTheMessage(isTheLastPacketOfTheMessage), seqNumber(seqNumber),
          ackNumber(ackNumber) {
    int i;
    for (i = 0; i < 8 && i < n; ++i) {
        payload[i] = data[i];
    }
    for (; i < 8; ++i) {
        payload[i] = '\0';
    }
}


LDPPacket::LDPPacket(bool isAck, bool isSeq, bool isTheLastPacketOfTheMessage, uint16_t seqNumber, uint16_t ackNumber)
        : isAck(isAck), isSeq(isSeq), isTheLastPacketOfTheMessage(isTheLastPacketOfTheMessage), seqNumber(seqNumber),
          ackNumber(ackNumber) {
    int i;
    for (i = 0; i < 8; ++i) {
        payload[i] = 0;
    }
}


std::string LDPPacket::payloadToString() {
    std::string str;
    for (int i = 0; i < 8; ++i) {
        if (payload[i]) {
            str += payload[i];
        }
    }
    return str;
}

void LDPPacket::print(bool sending) const {
    fprintf(stderr, "%s {isAck:%d, isSeq:%d, isLast:%d, ackNum:%d, seqNum:%d}\n",
            (sending ? "sending:   " : "receiving: "), isAck, isSeq,
            isTheLastPacketOfTheMessage, ackNumber, seqNumber);
}

LDPPacket LDPPacket::mergeAckAndSeqPackets(LDPPacket &ack, LDPPacket &seq) {
    LDPPacket packet(seq.payload, 8, true, true,
                     seq.isTheLastPacketOfTheMessage,
                     seq.seqNumber, ack.ackNumber);
    return packet;
}

void LDP::printSendWindow() const {
    fprintf(stderr, "sender window: ");
    for (int i = 0; i < senderWindow.deq.size(); ++i) {
        fprintf(stderr, "{p:%d, sq:%d, ackd:%d},",
                !senderWindow.deq[i].isNull(),
                senderWindow.deq[i].expectedSeqNumber, senderWindow.deq[i].isACKd);
    }
    fprintf(stderr, "\n");
}

void LDP::printRecWindow() const {
    fprintf(stderr, "receiver window: ");
    for (int i = 0; i < receiverWindow.deq.size(); ++i) {
        fprintf(stderr, "{p:%d, sq:%d, ackd:%d},",
                !receiverWindow.deq[i].isNull(),
                receiverWindow.deq[i].expectedSeqNumber, receiverWindow.deq[i].isACKd);
    }
    fprintf(stderr, "\n");

}

void LDP::closeYourMouth() {
    pthread_cancel(ldpPacketSenderThreadID);
    pthread_cancel(ldpPacketRetransmitterThreadID);
}

void LDP::closeYourEars() {
    pthread_cancel(listenerThreadID);

    /* we can't cancel this because closing is done on this thread.
     * pthread_cancel(messageProducerThreadID);*/
}

void LDP::waitUntilAckToAckToBye() {
    /**
     * this is two general problem. we can never know for sure if previous ack is ackd. therefore i will just close connection after 40 tries.
     * there is no way none of the 30 packets reach their destination.
     * 3ms overhead
     */
    for (int i = 0; i < 30; ++i) {
        LDPPacket packet = LDPPacket(true, false, false, byeSeq, byeSeq);
        packet.payload[0] = 0;
        packet.payload[1] = 'b';
        udpSend(packet);
        usleep(100);
    }
    exit(0);
}


LDPPacketInWindow::LDPPacketInWindow(LDPPacket &p, bool isAck, uint16_t seqNumber = 0) : isACKd(isAck),
                                                                                         expectedSeqNumber(seqNumber) {
    packet = new LDPPacket(p);
}

LDPPacketInWindow::~LDPPacketInWindow() {
    if (packet) {
        delete packet;
    }
}


LDPPacketInWindow *LDPPacketInWindow::make(LDPPacket &p, bool isAck, uint16_t seqNum) {
    if (packet != NULL) {
        delete packet;
    }
    packet = new LDPPacket(p);
    isACKd = isAck;
    expectedSeqNumber = seqNum;
    return this;
}

LDPPacketInWindow::LDPPacketInWindow() {
    packet = NULL;
    isACKd = false;
    expectedSeqNumber = 0;
}

LDPPacketInWindow::LDPPacketInWindow(uint16_t seqNum) {
    packet = NULL;
    isACKd = false;
    expectedSeqNumber = seqNum;
}

void LDPPacketInWindow::shallowSwap(LDPPacketInWindow &p) {
    LDPPacket *temp = p.packet;
    p.packet = packet;
    packet = temp;

    bool tempAck = p.isACKd;
    p.isACKd = tempAck;
    isACKd = tempAck;

    uint16_t tempSeqNumber = p.expectedSeqNumber;
    p.expectedSeqNumber = tempSeqNumber;
    expectedSeqNumber = tempSeqNumber;
}

bool LDPPacketInWindow::isNull() const {
    return packet == NULL;
}

LDPPacketInWindow::LDPPacketInWindow(const LDPPacketInWindow &p) : packet(NULL) {
    *this = p;
}

LDPPacketInWindow &LDPPacketInWindow::operator=(const LDPPacketInWindow &p) {
    if (this == &p) {
        return *this;
    }
    if (packet != NULL) {
        delete packet;
    }
    if (p.packet == NULL) {
        packet = NULL;
    } else {
        packet = new LDPPacket(*(p.packet));
    }
    isACKd = p.isACKd;
    expectedSeqNumber = p.expectedSeqNumber;
    return *this;
}

LDPPacketInWindow::LDPPacketInWindow(const char *data, size_t n, bool isAck, bool isSeq,
                                     bool isTheLastPacketOfTheMessage, uint16_t seqNumber, uint16_t ackNumber,
                                     bool isAckd, uint16_t expectedSeqNumber) : isACKd(isAckd),
                                                                                expectedSeqNumber(expectedSeqNumber) {
    packet = new LDPPacket(data, n, isAck, isSeq, isTheLastPacketOfTheMessage, seqNumber, ackNumber);

}

PacketAndItsTimeout::PacketAndItsTimeout(LDPPacketInWindow *packet, timespec sentTime) : packet(packet),
                                                                                         sentTime(sentTime) {

}

long PacketAndItsTimeout::timeUntilTimeout() const {
    struct timespec spec;
    clock_gettime(MY_CLOCK, &spec);
    return getTimeDifferenceMs(spec, sentTime) + TIMEOUT_DURATION_MS;
}
