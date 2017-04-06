/*queue.h*/
/*Pseudo class to act as send queue*/
//Handles most TCP operations like sliding window
//ACKs, and getting packets

#ifndef QUEUE_H
#define QUEUE_H

#ifndef MPTCP_H
#include "mptcp.h"
#endif

#include "utility.h"

struct queue_item{

	//Who Owns the item
	//0 is unassigned
	//>0 pthread id
	//-1 ACKed
	int owner;

	//Index in the queue for quick lookup
	int index;

	//Actual Data
	struct packet* pkt;
	socklen_t addrSize;
};

//Struct for keeping track of duplicate ACKs
struct queue_ack{
	int seq_num;
	int occurrances;
};

extern int queue_size;
extern int queue_front;
extern int queue_back;
extern int window;
extern struct queue_ack queue_duplicates;

//Queue
extern struct queue_item * queue;

//Information Functions
int queue_empty();

//Construct and Destruct
void queue_create(int size, int windowSize);
void queue_destroy();

//Insert Claim and Remove
int queue_insert(struct packet* pkt);
//Acks a received packet
void queue_ack(int ack_num);

//Only for starting threads. Inits the packet to send and returns to
//sender
struct queue_item * queue_getOwned(int pid, struct queue_item* lastSent);

//Claims the next packet to be sent
struct queue_item * queue_claim(int pid, int socket, struct sockaddr_in* serv);
void queue_unclaim(int seq_num); 
	


//Utility Functions
void qError(int function);
void queue_print();


#endif //queue.h