//
// Created by lutfullah on 30.12.2021.
//

#include "threadsafedeque.h"

template<class T>
ThreadSafeDeque<T>::ThreadSafeDeque(unsigned int size) {
    pthread_mutex_init(&mutex, NULL);
    sem_init(&emptySlotCount, 0, size);
    sem_init(&fullSlotCount, 0, 0);
}

template<class T>
ThreadSafeDeque<T>::~ThreadSafeDeque() {
    pthread_mutex_destroy(&mutex);
    sem_destroy(&emptySlotCount);
    sem_destroy(&fullSlotCount);
}

template<class T>
ThreadSafeDeque<T> *ThreadSafeDeque<T>::lock() {
    pthread_mutex_lock(&mutex);
    return this;
}

template<class T>
ThreadSafeDeque<T> *ThreadSafeDeque<T>::unlock() {
    pthread_mutex_unlock(&mutex);
    return this;
}

template<class T>
ThreadSafeDeque<T> *ThreadSafeDeque<T>::waitAndDecrementEmpty() {
    sem_wait(&emptySlotCount);
    return this;
}

template<class T>
ThreadSafeDeque<T> *ThreadSafeDeque<T>::waitAndDecrementFull() {
    sem_wait(&fullSlotCount);
    return this;
}

template<class T>
ThreadSafeDeque<T> *ThreadSafeDeque<T>::signalAndIncrementEmpty() {
    sem_post(&emptySlotCount);
    return this;
}

template<class T>
ThreadSafeDeque<T> *ThreadSafeDeque<T>::signalAndIncrementFull() {
    sem_post(&fullSlotCount);
    return this;
}

template<class T>
int ThreadSafeDeque<T>::timedWaitAndDecrementEmpty(long ms) {
    return sem_timedwait_millsecs(&emptySlotCount, ms);
}

template<class T>
int ThreadSafeDeque<T>::timedWaitAndDecrementFull(long ms) {
    return sem_timedwait_millsecs(&fullSlotCount, ms);
}
