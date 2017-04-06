//MPTCP.h
//Lucas Lopilato
//CS176B Homework 2
//
#ifndef CLIENT_H
#define CLIENT_H

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "threads.h"


#ifndef CONNECTION_H
#include "connection.h"
#endif

#ifndef UTILITY_H
#include "utility.h"
#endif

#ifndef QUEUE_H
#include "queue.h"
#endif


#ifndef debug
#define debug 1
#endif 

//Main point of Entry file
void invalidArgs(void);
void mainError(int function);

#endif