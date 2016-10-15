/*
 * File: multi-lookup.c
 * Author: Vincent Mahathirash
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 2016/10/12
 * Description:
 * 	This file contains implementations of main functions for
 *  Programming Assignment 3.
 */

#include <pthread.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"
#include "multi-lookup.h"

// Mutexes
pthread_mutex_t writeMutex; // Protect output file
pthread_mutex_t queueMutex; // Protects queue

// Condition Variables
pthread_cond_t notFull; // Condition where queue is not full.
pthread_cond_t notEmpty; // Condition where queue is not empty.

static queue q; // Shared resource
static int requestsComplete = 0;
static int allIPAddresses = 1; // If 1, returns all IP addresses + IPv6 if available

void* Producer(void* fd)
{
  FILE* inputFile;
  char** fileName = fd;
  char hostname[MAX_NAME_LENGTH];
  char* payload;

  inputFile = fopen(*fileName, "r");

  if (!inputFile)
  {
    printf("Error opening file.\n");
    return NULL;
  }

  while (fscanf(inputFile, INPUTFS, hostname) != EOF)
  {
    // Acquire queue lock
    pthread_mutex_lock(&queueMutex);

    // If queue is full, wait until a position opens up
    while (queue_is_full(&q))
      pthread_cond_wait(&notFull, &queueMutex);

    if (!queue_is_full(&q))
    {
      payload = malloc(sizeof(hostname));
      strncpy(payload, hostname, sizeof(hostname));

      if (queue_push(&q, payload) == QUEUE_FAILURE)
        fprintf(stderr, "Queue push failed.\n");
      pthread_cond_signal(&notEmpty); // Signal that there's a request in the queue
    }
    // Release queue lock
    pthread_mutex_unlock(&queueMutex);
  }

  fclose(inputFile);
  return NULL;
}

void* Consumer(void* outputFile)
{
  char firstipstr[MAX_IP_LENGTH];
  char* hostname;

  // Continue resolving if there are requests in the queue or requests to be added
  while (!queue_is_empty(&q) || !requestsComplete)
  {
    int resolved = 0;

    // Acquire queue lock
    pthread_mutex_lock(&queueMutex);

    // If the queue is empty, wait for a request to be added
    while (queue_is_empty(&q))
      pthread_cond_wait(&notEmpty, &queueMutex);

    if(!queue_is_empty(&q))
    {
      hostname = queue_pop(&q);
      if (hostname != NULL)
      {
        resolved = 1;
        pthread_cond_signal(&notFull); // Signal that there is a request in the queue
      }
    }
    // Release queue lock
    pthread_mutex_unlock(&queueMutex);

    if (resolved)
    {
      if (allIPAddresses) // Finds all IP addresses + IPv6 addresses
      {
        queue ipList; // Holds all IP addresses
        queue_init(&ipList, 50);
        int status = dnslookup(hostname, firstipstr, sizeof(firstipstr), &ipList);
        if (status == UTIL_FAILURE)
          fprintf(stderr, "dnslookup error: %s\n", hostname);
        // Acquire output lock
        pthread_mutex_lock(&writeMutex);
        if (status == UTIL_FAILURE)
          fprintf(outputFile, "%s,", hostname);
        else
          fprintf(outputFile, "%s", hostname);
        char* ipAddr;
        while ((ipAddr = (char*) queue_pop(&ipList)) != NULL)
        {
          fprintf(outputFile, ",%s", ipAddr);
          free(ipAddr);
        }
        fprintf(outputFile, "\n");
        // Release output lock
        pthread_mutex_unlock(&writeMutex);
        free(hostname);
        queue_cleanup(&ipList);
      }
      else // Finds only a single address, no IPv6
      {
        if (dnslookup(hostname, firstipstr, sizeof(firstipstr), NULL) == UTIL_FAILURE)
        {
          fprintf(stderr, "dnslookup error: %s\n", hostname);
          pthread_mutex_lock(&writeMutex); // Acquire output lock
          fprintf(outputFile, "%s,\n", hostname);
          pthread_mutex_unlock(&writeMutex); // Release output lock
        }
        else
        {
          pthread_mutex_lock(&writeMutex); // Acquire output lock
          fprintf(outputFile, "%s,%s\n", hostname, firstipstr);
          pthread_mutex_unlock(&writeMutex); // Release output lock
        }
        free(hostname);
      }
    }
  }

  return NULL;
}

int main(int argc, char* argv[])
{
  // Check if proper number of arguments were passed
  if (argc < MINARGS)
  {
    fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
    fprintf(stderr, "Usage: %s %s\n", argv[0], USAGE);

    return EXIT_FAILURE;
  }

  // Open output file
  FILE* outputFP = fopen(argv[argc - 1], "w");
  if (!outputFP)
  {
    perror("Error opening output file.\n");
    return EXIT_FAILURE;
  }

  int ret; // Get return status of pthread_create
  long i; // Loop Counter
  int numCores = sysconf(_SC_NPROCESSORS_ONLN); // Number of processor cores
  int numResThreads = numCores >= MIN_RESOLVER_THREADS ? numCores : MAX_RESOLVER_THREADS; // Number of resolver threads
  int numReqThreads = argc - 2; // Number of request threads

  // Initialize mutexes
  if (pthread_mutex_init(&writeMutex, NULL) || pthread_mutex_init(&queueMutex, NULL))
  {
    fprintf(stderr, "Error initializing mutexes.\n");
    return EXIT_FAILURE;
  }

  // Initialize condition variables
  if (pthread_cond_init(&notEmpty, NULL) || pthread_cond_init(&notFull, NULL))
  {
    fprintf(stderr, "Error initializing condition variables.\n");
    return EXIT_FAILURE;
  }

  // Initialize queue
  if (queue_init(&q, QUEUE_SIZE) == QUEUE_FAILURE)
  {
    fprintf(stderr, "Error initializing queue.\n");
    return EXIT_FAILURE;
  }

  // Create request thread pool
  pthread_t requestThreads[MAX_REQUESTER_THREADS];

  // Spawn request threads
  for (i = 0; i < numReqThreads; ++i)
  {
    ret = pthread_create(&requestThreads[i], NULL, Producer, &argv[i + 1]);
    if (ret)
    {
      printf("Error: pthread_create returned %d.\n", ret);
      return EXIT_FAILURE;
    }
  }

  // Create resolver thread pool
  pthread_t resolverThreads[MAX_RESOLVER_THREADS];

  // Spawn resolver threads
  for (i = 0; i < numResThreads; ++i)
  {
    ret = pthread_create(&resolverThreads[i], NULL, Consumer, outputFP);
    if (ret)
    {
      printf("Error: pthread_create returned %d.\n", ret);
      return EXIT_FAILURE;
    }
  }

  //Join request threads
  for(i = 0; i < numReqThreads; ++i)
    pthread_join(requestThreads[i], NULL);
  requestsComplete = 1;

  //Join resolver threads
  for(i = 0; i < numResThreads; ++i)
    pthread_join(resolverThreads[i], NULL);

  // Destroy mutexes
  if (pthread_mutex_destroy(&writeMutex) || pthread_mutex_destroy(&queueMutex))
    fprintf(stderr, "Error destroying mutexes.\n");

  // Destroy condition variables
  if (pthread_cond_destroy(&notFull) || pthread_cond_destroy(&notEmpty))
    fprintf(stderr, "Error destroying condition variables.\n");

  queue_cleanup(&q); // Cleanup queue
  fclose(outputFP); // Close output file

  return EXIT_SUCCESS;
}
