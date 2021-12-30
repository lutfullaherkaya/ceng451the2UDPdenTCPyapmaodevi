//
// Created by lutfullah on 30.12.2021.
//

#ifndef THE2_CONFIG_CONSTANTS_H
#define THE2_CONFIG_CONSTANTS_H

#define MAXBUFLEN 256

/**
 * Benchmark notes:
 * Sometimes it works but does not terminate saying:
 * chatter finished for client
 * chatter finished for server
 */

/**
 * Fine tune this
 */
#define TROLL_PROPAGATION_DELAY 10

/**
 * RTT + 4ms
 * Assume cpu takes 8 miliseconds to do its thing.
 */
#define TIMEOUT_DURATION_MS (TROLL_PROPAGATION_DELAY * 2 + 8)

/**
 * Window Size Calculation
 * Since Troll can only facilitate 16 packets atmost, the window size must be less than atmost 16.
 * Each packet will return as ACK. Therefore for each packet, there is only one packet in the
 * network at the same time, not taking into account the retransmissions from timeout.
 * As the Propagation Delay approaches infinity , the packet count in the network diverges to infinity
 * because every packet is lost and timed out, thus retransmitted. Therefore the timeout duration
 * must be more than double the propagation delay and the Propagation Delay must be finite.
 *
 * 12 seems to be a nice even number for the window size. Since the performance is directly
 * proportional to the window size, it is important for the window size to be as big as possible.
 *
 * Also the window size must not be larger than 15 bits because the sequence numbers are 2 bytes.
 *
 */
#define SENDER_WINDOW_SIZE 20

/**
 * Just for being sure, receiver window size is more.
 */
#define RECEIVER_WINDOW_SIZE (SENDER_WINDOW_SIZE+4)

/**
 * We can't queue infinite packets for sending.
 */
#define PACKETS_TO_BE_SENT_Q_SIZE (1000 + SENDER_WINDOW_SIZE)
#define PACKETS_RECEIVED_Q_SIZE PACKETS_TO_BE_SENT_Q_SIZE



#endif //THE2_CONFIG_CONSTANTS_H
