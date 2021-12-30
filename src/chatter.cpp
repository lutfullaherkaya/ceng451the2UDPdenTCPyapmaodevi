//
// Created by lutfullah on 30.12.2021.
//

#include "chatter.h"
#include "pthread.h"



Chatter::Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : byed(false), isClient(isClient), ldp(chateeIp, chateePort, myPort, isClient) {
    // client
    byeLock = PTHREAD_MUTEX_INITIALIZER;
    ldp.createAndBindSocket();
    ldp.listen(&Chatter::printMessageHelper, this);

    // todo: durdurma mekanizmasi: flag olcak iste onu degistircekler mutexle.
    while (iDontGetByed()) {
        char message[MAXBUFLEN];
        fgets(message, MAXBUFLEN, stdin);
        sendMessage(message);
    }

    ldp.closeYourMouth();

    ldp.closeYourEars();

}

Chatter::Chatter(const std::string &myPort, bool isClient) : byed(false), isClient(isClient), ldp(myPort, isClient) {
    // server
    byeLock = PTHREAD_MUTEX_INITIALIZER;
    ldp.createAndBindSocket();
    ldp.listen(&Chatter::printMessageHelper, this);

    // todo: durdurma mekanizmasi
    while (iDontGetByed()) {
        char message[MAXBUFLEN];
        fgets(message, MAXBUFLEN, stdin);
        sendMessage(message);
    }

    ldp.closeYourMouth();
    ldp.closeYourEars();

}
void Chatter::sendMessage(std::string message) {
    ldp.sendMessage(message);
}

Chatter::~Chatter() {
}


void *Chatter::printMessage(std::string &message) {
    if (message == "BYE\n") {
        setByed(true);
        exit(0);
    }

    std::cout << message;
    return NULL;
}

void *Chatter::printMessageHelper(void *context, std::string &message) {
    return ((Chatter *) context)->printMessage(message);
}

bool Chatter::iDontGetByed() {
    bool ret_val;
    pthread_mutex_lock(&byeLock);
    ret_val = byed;
    pthread_mutex_unlock(&byeLock);
    return !ret_val;
}

void Chatter::setByed(bool byd) {
    pthread_mutex_lock(&byeLock);
    byed = byd;
    pthread_mutex_unlock(&byeLock);
}
