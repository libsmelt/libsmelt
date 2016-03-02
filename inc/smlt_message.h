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
#define SMELT_MESSAGE_MAX_SIZE 128

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
    uint32_t offset;    ///< offset into the data region
    uint32_t datalen;   ///< length of the data region in bytes
    uint32_t maxlen;    ///< maximum length of the data region
    uint32_t _pad;      ///< unused padding for now
    void*    data;
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


/**
 * @brief writes data into the message buffer
 * 
 * @param msg   the Smelt message to write to
 * @param data  data to write (source buffer)
 * @param bytes number of bytes inteded to write
 *
 * @returns number of bytes written
 */
uint32_t smlt_message_write(struct smlt_msg *msg,
                            void *data, 
                            uint32_t bytes);

/**
 * @brief writes data into the message buffer
 * 
 * @param msg   the Smelt message to read from
 * @param data  buffer to read into (destination buffer)
 * @param bytes number of bytes inteded to read
 *
 * @returns number of bytes written
 */
uint32_t smlt_message_read(struct smlt_msg *msg,
                           void *data, uint32_t size);


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
static inline void *smlt_message_get_data(struct smlt_msg *msg)
{
    return msg->data;
}

/**
 * @brief sets the data pointer
 * 
 * @param msg   the Smelt message to udpate the buffer
 * @param data  buffer to contain the message content
 * @param bytes size of the buffer
 *
 * @returns number of bytes written
 */
static inline void smlt_message_set_data(struct smlt_msg *msg, 
                                         void *data, uint32_t length)
{
    msg->data = data;
    msg->maxlen = length;    
}

/**
 * @brief obtains the data length
 * 
 * @param msg   the Smelt message 
 *
 * @returns size of the data written to buffer in bytes
 */
static inline uint32_t smlt_message_get_datalen(struct smlt_msg *msg)
{
    return msg->datalen;
}

/**
 * @brief obtains the data length
 * 
 * @param msg   the Smelt message 
 *
 * @returns maximum capacity size of the data buffer in bytes
 */
static inline uint32_t smlt_message_get_maxlen(struct smlt_msg *msg)
{
    return msg->maxlen;
}



#endif /* SMLT_SMLT_H_ */
