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

// todo: sil?
#define MAXBUFLEN 100
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void *get_in_addr(struct sockaddr *sa);


class Chatter {
private:
    bool isListening;
public:
    bool getIsListening();

    void setIsListening(bool isListening);


    std::string chateeIP;
    std::string chateePort;
    std::string myPort;
    struct sockaddr chateeSockaddr;
    bool isClient;
    int sockfd;
    pthread_mutex_t ioLock;
    pthread_mutex_t isListeningLock;

    Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient);

    Chatter(const std::string &myPort, bool isClient);

    int initializeMutexes();
    int destroyMutexes();
    /**
     * also computes myAddrInfo
     * @return
     */
    int createAndBindSocket();

    int computeChateeAddrInfo();

    void initiateChat();

    std::string getInput();
    int sendMessage(std::string message);

    std::string receiveMessage();

    void endChat();

    // source: https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
    void *listener();

    static void *listenerHelper(void *context);

    void speaker();

    virtual ~Chatter();


};

#endif //THE2_LDP_H
