//
// Created by lutfullah on 30.12.2021.
//

#ifndef THE2_CHATTER_H
#define THE2_CHATTER_H

#include "ldp.h"

class Chatter {
public:
    LDP ldp;
    bool byed;

    bool iDontGetByed();

    void setByed(bool byed);

    pthread_mutex_t byeLock;

    bool isClient;

    Chatter(const std::string &chateeIp, const std::string &chateePort, const std::string &myPort, bool isClient);

    Chatter(const std::string &myPort, bool isClient);


    void sendMessage(std::string message);

    /**
     * source: https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
     */
    void *printMessage(std::string &message);

    static void *printMessageHelper(void *context, std::string &message);



    ~Chatter();
};


#endif //THE2_CHATTER_H
