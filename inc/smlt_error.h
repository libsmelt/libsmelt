/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_ERROR_H_
#define SMLT_ERROR_H_ 1

/*
 * ===========================================================================
 * Smelt error values
 *
 * This is a adapted from Barrelfish's error handling
 * ===========================================================================
 */

/**
 * Specifies the error values that can occur
 */
enum smlt_err_code {
    SMLT_SUCCESS = 0  ///< function call succeeded
};


/*
 * ===========================================================================
 * Function declarations
 * ===========================================================================
 */


/**
 * @brief obtains the last occurred error on the error stack
 *
 * @param errval    error value
 *
 * @return last occurred error
 */
static inline enum smlt_err_code smlt_err_no(errval_t errval)
{
    return (((enum smlt_err_code) (errval & ((1 << 10) - 1))));
}

/**
 * @brief checks whether the error value represents a failure
 *
 * @param errval    error value to check
 *
 * @return TRUE if the value indicates a failure, FALSE if success
 */
static inline bool smlt_err_is_fail(errval_t errval)
{
    return (smlt_err_no(errval) != SMLT_SUCCESS);
}

/**
 * @brief checks whether the error value represents success
 *
 * @param errval    error value to check
 *
 * @return TRUE if success, FALSE on failure
 */
static inline bool smlt_err_is_ok(errval_t errval)
{
    return (smlt_err_no(errval) == SMLT_SUCCESS);
}

/**
 * @brief pushes a new error onto the error value stack
 *
 * @param errval    the error value stack
 * @param errcode   the error code to push
 *
 * @return new error value stack
 */
static inline errval_t smlt_err_push(errval_t errval, enum smlt_err_code errcode)
{
    return (((errval << 10) | ((errval_t) (1023 & errcode))));
}

/**
 * @brief returns a string representation of the error code
 *
 * @param errval    error value to print
 *
 * @return pointer to the string of the error code
 */
char* smlt_err_get_code(errval_t errval);

/**
 * @brief returns a string representation of the error description
 *
 * @param errval    the error value
 *
 * @return pointer to the string of the error code
 */
char* smlt_err_get_string(errval_t errval);

/**
 * @brief prints the call trace of the errors
 *
 * @param errval    error value stack
 */
void smlt_err_print_calltrace(errval_t errval);

#endif /* SMLT_ERROR_H_ */