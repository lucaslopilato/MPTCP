#ifndef UTILITY_H
#define UTILITY_H

#include <pthread.h>
#include "mptcp.h"
#include "connection.h"

/**********Debug Functions**********/
void inspectPkt(struct packet* pkt);
void inspectAddr(struct sockaddr_in* addr);
void* printTest(void* input);

#endif //utility.h