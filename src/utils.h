//
// Created by lutfullah on 30.12.2021.
//

#ifndef THE2_UTILS_H
#define THE2_UTILS_H

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
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <deque>

long getTimeDifferenceMs(timespec start, timespec end);

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

void sleepMs(long ms);


#endif //THE2_UTILS_H
