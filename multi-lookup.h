/*
 * File: multi-lookup.h
 * Author: Vincent Mahathirash
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 2016/10/12
 * Description:
 * 	This file contains declarations of main functions for
 *  Programming Assignment 3.
 */

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#define MAX_INPUT_FILES 10
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MIN_RESOLVER_THREADS 2
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN
#define INPUTFS "%1024s"
#define MINARGS 3
#define QUEUE_SIZE 10
#define USAGE "<inputFilePath> <outputFilePath>"

void* Producer(void* fd);
void* Consumer(void* outputFile);

#endif
