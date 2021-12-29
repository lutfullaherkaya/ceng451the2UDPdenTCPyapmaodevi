//
// Created by lutfullah on 30.12.2021.
//

#include "chatter.h"




Chatter::Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient)
        : isClient(isClient), ldp(chateeIp, chateePort, myPort, isClient) {
    // client
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

Chatter::Chatter(const std::string &myPort, bool isClient) : isClient(isClient), ldp(myPort, isClient) {
    // server
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
    ldp.send(message);
}

Chatter::~Chatter() {
}


void *Chatter::printMessage(std::string &message) {
    if (message == "BYE\n") {

    }
    std::cout << message << std::endl;
    return NULL;
}

void *Chatter::printMessageHelper(void *context, std::string &message) {
    return ((Chatter *) context)->printMessage(message);
}
