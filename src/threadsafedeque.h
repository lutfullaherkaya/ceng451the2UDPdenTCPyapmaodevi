//
// Created by lutfullah on 30.12.2021.
//

#ifndef THE2_THREADSAFEDEQUE_H
#define THE2_THREADSAFEDEQUE_H

#include "utils.h"
#include <errno.h>

template<class T>
class ThreadSafeDeque {
public:
    ThreadSafeDeque(unsigned int size);

    ThreadSafeDeque<T> *lock();

    ThreadSafeDeque<T> *unlock();

    ThreadSafeDeque<T> *waitAndDecrementEmpty();

    int timedWaitAndDecrementEmpty(long ms);

    ThreadSafeDeque<T> *waitAndDecrementFull();

    int timedWaitAndDecrementFull(long ms);

    ThreadSafeDeque<T> *signalAndIncrementEmpty();

    ThreadSafeDeque<T> *signalAndIncrementFull();


    ~ThreadSafeDeque();


    std::deque<T> deq;

private:
    sem_t fullSlotCount, emptySlotCount;
    pthread_mutex_t mutex;

};


#endif //THE2_THREADSAFEDEQUE_H
