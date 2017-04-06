#ifndef CONNECTION_H
#define CONNECTION_H

#include <netdb.h>
#include <arpa/inet.h>

#ifndef MPTCP_H
#include "mptcp.h"
#endif

//File of helpers to handle packets, addresses, sockets, and ports


//Command Line Arguments
struct arguments {
	int connections; //Number of Connections to Establish
	char* hostname; //Hostname of Server
	int port; //Port Number
	char* filename; //Name of file to transfer
	int verbose; //Handles whether to print which packets are sent or received
};

//Address Operations

void setSocketTimeout(int socket, struct timeval* timeout);

//Initialize a sockaddr_in for the server
void initForeignAddress(int port, char* hostname, struct sockaddr_in* address);
void initLocalAddress(int socket, struct sockaddr_in* addr, socklen_t addrSize);

//Packet Operations

//Packet, Source, Destination, Seq, Ack
void createPacket(struct packet* pkt, int socket, struct sockaddr_in* dest, int seq, int ack);
void setMessage(struct packet* pkt,const char* data, int total);
void deletePacket(struct packet* pkt);
void allocatePacket(struct packet* pkt);
void zeroPacket(struct packet* pkt);
void zeroAddr(struct sockaddr_in* addr);
//Returns 1 if received packed ACKS expected packet 0 otherwise
int validPacketResponse(struct packet* expected, struct packet* actual);
//Utility Functions
void error(int function);

//Perform Deep Copy of A sockaddr_in
void copyAddr(struct sockaddr_in* src, struct sockaddr_in* dest);

//Parse Ports from the control server response
void parsePorts(char* message, int received, int* ports, int numports);

#endif //connection.h