#include "utility.h"

/**********Debug Functions**********/
void inspectPkt(struct packet* pkt){
    struct mptcp_header* h = pkt->header;

    printf("----------------Begin Packet--------------------\n");
    printf("Source IP: %s, Source Port: %d\n", inet_ntoa(h->src_addr.sin_addr), h->src_addr.sin_port);
    printf("Dest IP: %s, Dest Port: %d\n", inet_ntoa(h->dest_addr.sin_addr), h->dest_addr.sin_port);
    printf("Seq: %d, Ack: %d, Bytes: %d\n", h->seq_num, h->ack_num, h->total_bytes);
    printf("----------------Data----------------------------\n");
    printf("%s\n", pkt->data);
    printf("-----------------End Packet----------------------\n\n");
}

void inspectAddr(struct sockaddr_in* addr){
	printf("sin_family: %d\n", addr->sin_family);
	printf("sin_family: %d\n", addr->sin_port);
	printf("sin_family: %s\n", inet_ntoa(addr->sin_addr));
}

void* printTest(void* input){
    printf("Test\n");
    return NULL;
}