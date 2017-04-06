#include "client.h"


/***********Address Operations*************/
void setSocketTimeout(int socket, struct timeval* timeout){
    if (setsockopt (socket, SOL_SOCKET, SO_RCVTIMEO, (char *)timeout, sizeof(*timeout)) < 0)
        error(10);
}

/**********Packet Functions**********/

//Packet, Source, Destination, Seq, Ack
void createPacket(struct packet* pkt, int socket, struct sockaddr_in* dest, int seq, int ack){
    //Check For Invalid Input
    if(pkt == NULL || dest == NULL) error(1);
    if(seq < 0 || ack < 0) error(2);

    //Allocate Memory For the Packet
    allocatePacket(pkt);
    pkt->header->seq_num = seq;
    pkt->header->ack_num = ack;

    //Initialize My Address Given Socket
    initLocalAddress(socket, &(pkt->header->src_addr), sizeof(pkt->header->src_addr));

    //Set Members
    copyAddr(dest, &(pkt->header->dest_addr));
}

void setMessage(struct packet* pkt, const char* data, int total){
    if(data == NULL) error(3);

    //Write Data and Update Payload
    sprintf(pkt->data, "%s", data);
    pkt->header->total_bytes = total;
    pkt->header->ack_num = pkt->header->seq_num + pkt->header->total_bytes;
}

void allocatePacket(struct packet* pkt){
    if(pkt == NULL) error(1);
    pkt->header = (struct mptcp_header*)malloc(sizeof(struct mptcp_header));
    pkt->data = malloc(MSS);

    zeroPacket(pkt);
}

void zeroPacket(struct packet* pkt){
    if(pkt == NULL) return;

    //Zero Data
    memset(pkt->data, 0, MSS);

    //Zero Header
    pkt->header->total_bytes = 0;
    pkt->header->seq_num = 0;
    pkt->header->ack_num = 0;    

    //Zero addr_ins
    zeroAddr(&pkt->header->dest_addr);
    zeroAddr(&pkt->header->src_addr);
}


void zeroAddr(struct sockaddr_in* addr){
    if(addr == NULL) return;
    addr->sin_family = AF_INET;
    addr->sin_port = 0;
    addr->sin_addr.s_addr = 0;
    memset(addr->sin_zero, 0, 8);
}

void deletePacket(struct packet* pkt){
    if(pkt == NULL) return;
    if(pkt->header != NULL) free(pkt->header);
    if(pkt->data != NULL) free(pkt->data);
}

int validPacketResponse(struct packet* expected, struct packet* actual){
    if(expected == NULL || actual == NULL){
        printf("NULL packet sent to validPacketResponse\n");
        exit(1);
    }

    //If end of packet
    if(expected->header->total_bytes - expected->header->seq_num <= MSS){
        if(actual->header->ack_num == -1) return 1;
    }
    else{
        if((actual->header->ack_num) == (expected->header->seq_num + MSS))
            return 1;
    }


    //Otherwise No
    return 0;
}
//Determine i

/**********Address Functions**********/

//port: Port Number of new Foreign Address
//hostname: C String representation of the new Addresses' hostname
//address: Location of sockaddr_in to save vars
void initForeignAddress(int port, char* hostname, struct sockaddr_in* address){

    //Resolve hostname
    struct hostent *hostEnt;
    hostEnt = gethostbyname(hostname);
    if(hostEnt == NULL) error(0);
 
    //Initialize servAddr structure
    address->sin_family = AF_INET; //Family of address
    address->sin_port = htons(port); //Port number
    inet_aton(hostname, &(address->sin_addr)); //Hostname
    bcopy((char *)hostEnt->h_addr, 
        (char *)&(address->sin_addr.s_addr), hostEnt->h_length);
}

void initLocalAddress(int socket, struct sockaddr_in* addr, socklen_t addrSize){
    addr->sin_family = AF_INET;
    if(getsockname(socket, (struct sockaddr*)addr, &addrSize) == -1){
        exit(4);
    }
}

/**********Utility Functions**********/
//Performs Deep Copy Of  sockaddr_in
void copyAddr(struct sockaddr_in* src, struct sockaddr_in* dest){
    if(src == NULL || dest == NULL) error(5);

    //Perform Deep Copy
    dest->sin_family = src->sin_family;
    dest->sin_port = src->sin_port;
    dest->sin_addr.s_addr = src->sin_addr.s_addr; 
}

//Displays Error Depending On where error occurred
void error(int function){
    if(debug == 1){
        switch(function){
        case 0: printf("couldn't resolve host in initializeAddress\n"); break;
        case 1: printf("attempted to init packet with a null address\n"); break;
        case 2: printf("attempted to init packet with a negative seq or ack\n"); break;
        case 3: printf("attempted to init packet with empty data\n"); break;
        case 4: printf("couldn't find myaddress when initializing packet\n"); break;
        case 5: printf("cannot perform copyAddr on null src or dest\n"); break;
        case 6: printf("Empty message or ports given to parsePorts\n"); break;
        case 7: printf("parsePorts given nonvalid number of ports \n"); break;
        case 8: printf("MPOK Not Received From Control Channel\n"); break;
        case 9: printf("Init Threads received an invalid connection number\n"); break;
        case 10: printf("Failed to Set socket timeout\n"); break;
        }

        exit(function);
    }
}

//Parse Ports from the Control Server Response
void parsePorts(char* message, int received, int* ports, int numports){
    if(message == NULL || ports == NULL)
        error(6);
    if(numports < 1 || numports > 16)
        error(7);

    char buffer[MSS];
    memset(buffer, 0, MSS); //Reset Buffer
    memset(ports, 0, numports);
    //strncpy(buffer, message, 5);
    //if(strcmp(buffer, "MPOK ") != 0)
        //error(8);
    //Copy Rest of Response into Buffer to Parse
    memset(buffer, 0, MSS); //Reset Buffer
    strcpy(buffer, message+5);

    if(numports == 1){
        if((ports[0] = atoi(buffer)) == 0)
            error(8);
    }
    else{
        char *each = NULL;
        each = strtok(buffer, ":");
        int i;
        for(i=0; i<numports; i++){
            if((ports[i] = atoi(each)) == 0)
                error(8);
            //printf("%d\n", ports[i]);
            each = strtok(NULL, ":");
        }
    }
}


