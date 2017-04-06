//Lucas Lopilato
//CS176B Homework 2
#include "client.h"

int main(int argc, char* argv[])
{
	//Initialize Options to default
	struct arguments options;
	options.connections = 1; 
	options.hostname = "localhost";
	options.port = 3000;
	options.filename = NULL;
    options.verbose = 0;

    //Parse Arguments and override defaults
    int opt;
    while( (opt = getopt(argc, argv, "vn:h:p:f:")) != -1 ){
        switch(opt){
            case 'n': //Number of Interfaces Option
                options.connections = atoi(optarg);
                break;
            case 'h': //Hostname Option
                options.hostname = optarg;
                break;
            case 'p': //Port Option
                options.port = atoi(optarg);
                break;
            case 'f': //File Option
                options.filename = optarg;
                break;
            case 'v': //Verbose Option
                options.verbose = 1;
                break;
            case '?':
                invalidArgs();
        }
    }

    //TODO
    //Validate Input
    if(options.connections < 1 || options.connections > 16)
    	invalidArgs();

    /*****************Defaults*******************************/
    int timeout = 5; //5 sec timeout



    //Get Size of Input File
    FILE * input = NULL;
    int filesize = 0;
    if(options.filename != NULL && (input = fopen(options.filename, "r")) != NULL){
        //Initialize Filesize
        fseek(input, 0L, SEEK_END);
        filesize = ftell(input);
        rewind(input);
    }

/*****************Control Socket****************************/
    //Create Control Socket
    int control;
    errno = 0;
    control = mp_socket(AF_INET, SOCK_MPTCP, 0);
    switch(errno){
        case 0: break;
        default: mainError(1); break;
    }

    //Initialize Server Address
    struct sockaddr_in server;
    initForeignAddress(options.port, options.hostname, &server);

    //Create Connection With Control Port
    
    errno = 0;
    mp_connect(control, (struct sockaddr*)&server, sizeof(server));
    switch(errno){
        case 0: break;
        default:
    	   mainError(2);
           break;
    }

    struct sockaddr_in me;
    initLocalAddress(control, &me, sizeof(me));
    int seq = 1;
    //inspectAddr(&me);

    //Initialize Message Buffer
    char buffer[128];
    memset(buffer, 0, 128);

    //Create Message To Send to Control Channel
    struct packet pkt;
    sprintf(buffer, "%s %d", "MPREQ", options.connections);
    createPacket(&pkt, control, &server, 1, 0);
    setMessage(&pkt, buffer, filesize);

    //inspectPkt(&pkt);


    //Send Message To Control Channel
    int sent;
    errno = 0;
    sent = mp_send(control, &pkt, MSS, 0);
    switch(errno){
        case 0: break;
        default: mainError(3);
    }
    //inspectPkt(&pkt);

    //Receive Response
  	int received;
    errno = 0;
    received = mp_recv(control, &pkt, MSS, 0);
    switch(errno){
        case 0: break;
        default: mainError(4);
    }

    //Parse Packet Response for available ports
    int *ports = malloc(sizeof(int) * options.connections);
    parsePorts(pkt.data, received, ports, options.connections);

    //inspectPkt(&pkt);
    //Delete Packet
    deletePacket(&pkt);

/*******************Send Window Init**********************************/
    int totalPackets = 0;
    //Do Not attempt if there is no filename or the file cannot be opened
    if(input !=NULL){

        //Calculate total number of packets needed
        totalPackets = ceil((float)filesize / (float)MSS);

        //Create Queue Based On Number of Needed Packets
        queue_create(totalPackets, options.connections);

        //Create Packets and Insert them into the queue
        int i;
        for(i=0; i<totalPackets; i++){
            struct packet* tosend = (struct packet*)malloc(sizeof(struct packet));
            allocatePacket(tosend);

            //Initialize data
            tosend->header->seq_num = seq;
            seq += fread(tosend->data, 1, MSS, input);
            tosend->header->total_bytes = filesize;
            tosend->header->ack_num = 0;

            queue_insert(tosend);
        }
        fclose(input);
    }

    //printf("\n\n\n");
    //queue_print();
    //printf("\n\n\n");



/***************************Build Mutex and Spawn Processes*******************/
    struct thread_in thread_input;
    thread_input.numsockets = options.connections;
    thread_input.ports = ports;
    thread_input.server = options.hostname;
    thread_input.timeout = timeout;
    thread_input.verbose = options.verbose;

    //Pass Struct to Init threads
    initThreads(&thread_input);

    //queue_print();
/********************Free Memory*************************************/
    //Free Allocated Memory
    queue_destroy();
    free(ports);

    //Close Control Socket
    close(control);
    printf("File Successfully Transferred\n");

	return 0;
}


//Prints standardized error message for improper options
void invalidArgs(void){
    fprintf(stderr, "invalid or missing options\n");
    fprintf(stderr, "usage: mptcp [ -n num_interfaces ] [ -h hostname ] [ -p port ] [ -f filename ]\n");
    fprintf(stderr, "[ -n ] : number of connections ( paths ) you would like to have with the server\n");
    fprintf(stderr, "[ -h ] : hostname of the server where to connect\n");
    fprintf(stderr, "[ -p ] : port number on the server where to start the initial connection\n");
    fprintf(stderr, "[ -f ] : name of the file to transfer to the server\n");
    fprintf(stderr, "[ -v ] : verbose mode, prints out packets sent and received\n");
    exit(1);
}

//Prints standardized error message for errors in main
void mainError(int function){
    printf("Error Occurred In Main: ");
    switch(function){
        case 1: printf("Error Creating Socket\n"); break;
        case 2: printf("Error Connecting to Control\n"); break;
        case 3: printf("Error Sending Control Message\n"); break;
        case 4: printf("Error Receiving Control Message\n"); break;
        case 5: printf("Error Received Non-Valid Thread Count\n"); break;

    }

    exit(1);
}
