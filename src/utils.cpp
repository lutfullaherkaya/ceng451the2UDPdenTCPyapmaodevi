//
// Created by lutfullah on 30.12.2021.
//

#include "utils.h"

long getTimeDifferenceMs(timespec start, timespec end) {
    return ((long) end.tv_sec - (long) start.tv_sec) * 1000 + ((long) end.tv_nsec - (long) start.tv_nsec) / 1000000;
}


void *serialize_uint32_t(void *buffer, uint32_t value) {
    *((uint32_t *) buffer) = htonl(value);
    return ((uint32_t *) buffer) + 1;
}

void *deserialize_uint32_t(void *buffer, uint32_t *value) {
    *value = ntohl(*((uint32_t *) buffer));
    return ((uint32_t *) buffer) + 1;
}

void *serialize_uint16_t(void *buffer, uint16_t value) {
    *((uint16_t *) buffer) = htons(value);
    return ((uint16_t *) buffer) + 1;
}

void *deserialize_uint16_t(void *buffer, uint16_t *value) {
    *value = ntohs(*((uint16_t *) buffer));
    return ((uint16_t *) buffer) + 1;
}

void *serialize_uint8_t(void *buffer, uint8_t value) {
    *((uint8_t *) buffer) = value;
    return ((uint8_t *) buffer) + 1;
}

void *deserialize_uint8_t(void *buffer, uint8_t *value) {
    *value = *((uint8_t *) buffer);
    return ((uint8_t *) buffer) + 1;
}

/**
 *
 * @param buffer
 * @param str perhaps not null terminated string
 * @param n
 * @return
 */
void *serialize_char_array(void *buffer, char *str, size_t n) {
    for (int i = 0; i < n; ++i) {
        ((char *) buffer)[i] = str[i];
    }
    return ((char *) buffer) + n;
}

/**
 *
 * @param buffer
 * @param str the output char array
 * @param n
 * @return
 */
void *deserialize_char_array(void *buffer, char *str, size_t n) {
    for (int i = 0; i < n; ++i) {
        str[i] = ((char *) buffer)[i];
    }
    return ((char *) buffer) + n;
}

void *serialize_char(void *buffer, char value) {
    ((char *) buffer)[0] = value;
    return ((char *) buffer) + 1;
}

void *deserialize_char(void *buffer, char *value) {
    *value = ((char *) buffer)[0];
    return ((char *) buffer) + 1;
}


void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void sleepMs(long ms) {
    long secs = ms / 1000;
    if (secs > 0) {
        sleep(secs);
    }
    if (ms * 1000 > 0) {
        usleep(ms * 1000);
    }
}

/**
 * source https://titanwolf.org/Network/Articles/Article?AID=e0cc10a2-dd8f-4029-a088-763e843c7092
 */
int sem_timedwait_millsecs(sem_t *sem, long msecs) {
    struct timespec ts;
    clock_gettime(MY_CLOCK, &ts);
    long secs = msecs / 1000;
    msecs = msecs % 1000;

    long add = 0;
    msecs = msecs * 1000 * 1000 + ts.tv_nsec;
    add = msecs / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = msecs % (1000 * 1000 * 1000);

    return sem_timedwait(sem, &ts);
}