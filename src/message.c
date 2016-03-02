/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <string.h>
#include <smlt_platform.h>
#include <smlt_message.h>


/**
 * @brief allocates a new buffer for the Smelt message
 *
 * @param size  the number of bytes to hold
 *
 * @returns pointer to the Smelt message or NULL on failure
 */
struct smlt_msg *smlt_message_alloc(uint32_t size)
{
    struct smlt_msg* msg;
    msg = (struct smlt_msg*) smlt_platform_alloc(sizeof(struct smlt_msg),
                                                SMLT_DEFAULT_ALIGNMENT,
                                                true);
    msg->offset = 0;
    msg->datalen = size;
    // TODO maxlen = total size?
    msg->maxlen = sizeof(struct smlt_msg)+size;
    msg->data = smlt_platform_alloc(size, SMLT_DEFAULT_ALIGNMENT,
                                    true);
    return msg;
}
/**
 * @brief allocates a new Smelt message without data
 *
 * @param size  the number of bytes to hold
 *
 * @returns pointer to the Smelt message or NULL on failure
 */
struct smlt_msg *smlt_message_alloc_no_buffer(void)
{
    struct smlt_msg* msg;
    msg = (struct smlt_msg*) smlt_platform_alloc(sizeof(struct smlt_msg),
                                                SMLT_DEFAULT_ALIGNMENT,
                                                true);
    msg->offset = 0;
    msg->datalen = 0;
    // TODO maxlen = total size?
    msg->maxlen = sizeof(struct smlt_msg);
    return msg;
}
/**
 * @brief frees a Smelt message
 * 
 * @param msg   The Smelt message to be freed
 */
void smlt_message_free(struct smlt_msg *msg)
{
    smlt_platform_free(msg->data);
    smlt_platform_free(msg);
}

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
                            uint32_t bytes)
{
    // TODO change interface since return value
    // is the same as bytes?
    memcpy(msg->data, data, bytes);
    return bytes;
}

/**
 * @brief reads data from the message buffer
 * 
 * @param msg   the Smelt message to read from
 * @param data  buffer to read into (destination buffer)
 * @param bytes number of bytes inteded to read
 *
 * @returns number of bytes read
 */
uint32_t smlt_message_read(struct smlt_msg *msg,
                           void *data, uint32_t size)
{
    memcpy(data, msg->data, size);
    return size;
}


