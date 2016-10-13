/*
 * File: util.h
 * Author: Andy Sayler
 * Modified: Shiv Mishra, Vincent Mahathirash
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 2012/02/01
 * Modify Date: 2012/02/01
 * Modify Date: 2016/09/26
 * Modify Date: 2016/10/12
 * Description:
 * 	This file contains declarations of utility functions for
 *  Programming Assignment 3.
 */

#ifndef UTIL_H
#define UTIL_H

/* Define the following to enable debug statments */
// #define UTIL_DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "queue.h"

#define UTIL_FAILURE -1
#define UTIL_SUCCESS 0

// Utility function to add IP address ip to user-supplied queue q.
void addToList(queue* q, char* ip);

/*
  Return the first IP address or all IP addresses for hostname.
  For single IP address, address returned as string firstIPstr of size maxSize
  For all IP addresses, addresses added as strings to user-supplied queue
*/
int dnslookup(const char* hostname, char* firstIPstr, int maxSize, queue* q);

#endif
