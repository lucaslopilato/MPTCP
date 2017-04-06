#include "queue.h"

//Queue
struct queue_item * queue = NULL;
int queue_size = 0; //TODO Fix
int queue_front = 0;
int queue_back = -1;
int queue_window = 1;
struct queue_ack queue_duplicates;


//Information Functions
int queue_empty(){
	if(queue_front >= queue_size) return 1;
	else return 0;
}



//Construct and Destruct
void queue_create(int size, int windowSize){
	//Check if already initialized
	if(queue != NULL) qError(0);
	if(windowSize < 0) qError(1);
	if(windowSize > size) windowSize = size;

	//Build Queue
	queue = malloc(sizeof(struct queue_item) * size);
	queue_size = size;
	queue_window = windowSize;

	//Initialize Queue
	int i;
	struct queue_item* local;
	for(i=0; i<size; i++){
		local = &queue[i];
		local->pkt = NULL;
		local->addrSize = 0;
		local->owner = 0;
		local->index = i;
	}

	queue_duplicates.seq_num = -1;
	queue_duplicates.occurrances = 0;
}

void queue_destroy(){
	if(queue == NULL)
		return;
	else{
		int i;
		for(i=0; i<queue_size; i++)
			if(queue[i].pkt != NULL)
				free(queue[i].pkt);
		free(queue);
	}
}

//Insert and Remove

//Setup Function add packet to queue
int queue_insert(struct packet* pkt){
	if(pkt == NULL) return -1;
	
	queue_back ++;

	if(queue_back == queue_size)
		qError(2);

	queue[queue_back].pkt = pkt;
	queue[queue_back].addrSize = sizeof(pkt->header->src_addr);
	queue[queue_back].index = queue_back;

	return queue_back;
}

//Claim Next Available Packet Within Window
struct queue_item * queue_claim(int pid, int socket, struct sockaddr_in* serv){
	if(queue_empty())
		return NULL; //TODO Might need something more descriptive

	int i = queue_front;
	struct mptcp_header* target = NULL;
	while(i != queue_size && i - queue_front < queue_window){
		if(queue[i].owner == 0){
			//Initialize the object's addresses
			target = queue[i].pkt->header;
			copyAddr(serv, &target->dest_addr);
			initLocalAddress(socket, &(target->src_addr), sizeof(target->src_addr));

			//Claim Ownership
			queue[i].owner = pid;
			return &queue[i];
		}

		i++;
	}

	return NULL;
}


//Acks Packets up the the ACK_Num
//Also Handles duplicate ACKs
void queue_ack(int ack_num){
	//If -1, ack all outstanding packets
	if(ack_num == -1){
		while(queue_empty() == 0){
			queue[queue_front].owner = -1;
			queue_front++;
		}
		return;
	}

	//Else Make sure the ack is legitimate
	if(ack_num % (MSS) != 1){
		//Disown the sequence that didn't go across
		int i;
		for(i=queue_front; i<queue_window; i++){
			//If Item contains the bytes this ack contains
			if(queue[queue_front+i].pkt->header->seq_num < ack_num &&
				queue[queue_front+i].pkt->header->seq_num + MSS > ack_num){

				queue[queue_front+i].owner = 0;
			}
		}
		return;
	}
	else{ //Otherwise a legitimate ack is received
		//Update Duplicate Acks to check for duplicates
		if(queue_duplicates.seq_num == -1){
			//Record this ack
			queue_duplicates.seq_num = ack_num;
			queue_duplicates.occurrances = 1;
		}
		//If Seq # is the same as in duplicates
		else if(queue_duplicates.seq_num == ack_num){
			queue_duplicates.occurrances ++;

			//Triple Duplicate ACK
			if(queue_duplicates.occurrances == 3){
				//Unclaim
				queue_unclaim(ack_num);

				//Reset Count
				queue_duplicates.seq_num = -1;
				queue_duplicates.occurrances = 0;

				//Halve Cwnd
				queue_window = queue_window / 2;
				if(queue_window < 1) queue_window = 1;
			}

			return;
		}
		//Increase Queue Window by 1
		queue_window ++;

		//Slide window
		while(queue_empty() == 0 && queue[queue_front].pkt->header->seq_num < ack_num){
			//Check if ack is legitimate
			queue[queue_front].owner = -1;
			queue_front++;
		}
	}
}

//Used by sending threads. Formats local address to be sent
struct queue_item * queue_getOwned(int pid, struct queue_item* lastSent){
	//If there is No item, or the last sent was acked get a new packet
	if(lastSent == NULL || lastSent->index < queue_front){
		int i = queue_front;
		while(i != queue_size && (i-queue_front) < queue_window){

			//If a new an unclaimed item is found
			if(queue[i].owner == 0){
				queue[i].owner = pid;
				return &(queue[i]);
			}

			i++;
		}

		//If no suitable Item found, return NULL
		return NULL;
	}
	//Otherwise, Return the Packet that has yet to be acked
	else{
		return lastSent;
	}
}

//Unclaims the packet from its owner and makes it available to any sender
void queue_unclaim(int seq_num){
	if(seq_num == -1){
		return;
	}

	int i;
	for(i=0; i<queue_window; i++){
		if(queue_front + i >= queue_size) return; //Prevent going out of bounds
		if(queue[queue_front + i].pkt->header->seq_num == seq_num){
			queue[queue_front + i].owner = 0;
		}
	}
}




/************Utility Functions***********/
void qError(int function){
	printf("Error occurred in queue\n");
	switch(function){
		case(0): printf("Queue Already Initialized\n"); break;
		case(1): printf("Bad Window Size to Init Queue\n"); break;
		case(2): printf("Tried to insert more packets than space in Q\n"); break;
	}
	exit(2);
}

void queue_print(){
	int i;
	if(queue == NULL){ printf("Queue uninitizialized\n"); return;}
	for(i=0; i<queue_size; i++){
		printf("--------------Begin Queue Item-------------\n");
		printf("Owner: %d\n", queue[i].owner);
		printf("Index: %d\n", queue[i].index);
		printf("AddrSize: %d\n", (int)queue[i].addrSize);
		if(queue[i].pkt == NULL)
			printf("Pkt: NULL\n");
		else inspectPkt(queue[i].pkt);
		printf("--------------End Queue Item-------------\n");

	}
}