/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_MESSAGE_H_
#define SMLT_MESSAGE_H_ 1

#include <inttypes.h>

/*
 * ===========================================================================
 * Smelt configuration
 * ===========================================================================
 */
#define SMELT_MESSAGE_MIN_SIZE 56

typedef uint64_t smlt_msg_payload_t;

/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */


/**
 * represents a smelt message handed by the system
 */
struct smlt_msg
{
    uint32_t words;
    uint32_t bufsize;
    smlt_msg_payload_t* data;
};


/*
 * ===========================================================================
 * message interface
 * ===========================================================================
 */


/**
 * @brief allocates a new buffer for the Smelt message
 *
 * @param size  the number of bytes to hold
 *
 * @returns pointer to the Smelt message or NULL on failure
 */
struct smlt_msg *smlt_message_alloc(uint32_t size);

/**
 * @brief allocates a new Smelt message without data
 *
 * @param size  the number of bytes to hold
 *
 * @returns pointer to the Smelt message or NULL on failure
 */
struct smlt_msg *smlt_message_alloc_no_buffer(void);

/**
 * @brief frees a Smelt message
 *
 * @param msg   The Smelt message to be freed
 */
void smlt_message_free(struct smlt_msg *msg);


/*
 * ===========================================================================
 * payload operations
 * ===========================================================================
 */



/*
 * ===========================================================================
 * get and set
 * ===========================================================================
 */


/**
 * @brief gets a pointer to the data part of the message
 *
 * @param msg   the Smelt message to get the data pointer
 *
 * @returns pointer ot the buffer or NULL if not set
 */
static inline smlt_msg_payload_t *smlt_message_get_data(struct smlt_msg *msg)
{
    return msg->data;
}

/**
 * @brief sets the data pointer
 *
 * @param msg   the Smelt message to udpate the buffer
 * @param data  buffer to contain the message content
 * @param words size of the buffer in words
 * @param size  maximum size of the buffer in bytes
 *
 * @returns number of bytes written
 */
static inline void smlt_message_set_data(struct smlt_msg *msg,
                                         smlt_msg_payload_t *data, uint32_t words,
                                         uint32_t size)
{
    msg->data = data;
    msg->words = words;
    msg->bufsize = size;
}


static inline void smlt_msg_set_length(struct smlt_msg *msg,
                                       uint32_t words)
{
    msg->words= words;
}


/**
 * @brief obtains the data length
 *
 * @param msg   the Smelt message
 *
 * @returns size of the data written to buffer in bytes
 */
static inline uint32_t smlt_message_length_words(struct smlt_msg *msg)
{
    return msg->words;
}


/**
 * @brief obtains the data length
 *
 * @param msg   the Smelt message
 *
 * @returns size of the data written to buffer in bytes
 */
static inline uint32_t smlt_message_length_bytes(struct smlt_msg *msg)
{
    return msg->words * sizeof(smlt_msg_payload_t);
}


#endif /* SMLT_SMLT_H_ */
