/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <string.h>
#include <stdio.h>

#include "smlt_platform.h"
#include "smlt_message.h"
#include "smlt_debug.h"

/**
 * @brief allocates a new buffer for the Smelt message
 *
 * Panic's on failure.
 *
 * @param size  the number of bytes to hold
 *
 * @returns pointer to the Smelt message.
 */
struct smlt_msg *smlt_message_alloc(uint32_t size)
{
    struct smlt_msg* msg;
    if (size < SMELT_MESSAGE_MIN_SIZE) {
        size = SMELT_MESSAGE_MIN_SIZE;
    }
    msg = (struct smlt_msg*) smlt_platform_alloc(sizeof(struct smlt_msg),
                                                SMLT_DEFAULT_ALIGNMENT,
                                                true);
    if (!msg) {
        panic("smlt_platform_alloc failed in smlt_message_alloc");
    }

    msg->words = size/sizeof(uintptr_t);

    if (size % sizeof(uintptr_t) != 0 ) {
      // size not multiple of word-size, need one more word to transfer
      msg->words++;
    }

    msg->bufsize = size;
    msg->data = (smlt_msg_payload_t*)
        smlt_platform_alloc(size, SMLT_DEFAULT_ALIGNMENT, true);

    if (!msg->data) {
        panic("smlt_platform_alloc failed in smlt_message_alloc");
    }
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
    if (!msg) {
        panic("smlt_platform_alloc failed in smlt_message_alloc");
    }

    msg->words = 0;
    msg->bufsize = 0;
    return msg;
}

/**
 * @brief frees a Smelt message
 *
 * @param msg   The Smelt message to be freed
 */
void smlt_message_free(struct smlt_msg *msg)
{
    if (msg!=NULL) {
        smlt_platform_free(msg->data);
        smlt_platform_free(msg);
    }

    msg = NULL;
}
