#include "threads.h"

/**********************Thread Setup and Teardown**********************/
void initThreads(struct thread_in* in){
    //Validate Input
    if(in == NULL) threadError(3);
    if(in->numsockets < 0 || in->numsockets > 16) threadError(0);
    if(in->ports == NULL) threadError(2);
    if(in->server == NULL) threadError(8);

    //Suppress StdErr to get rid of temporarily unavailable messages
    //Approach Taken From http://stackoverflow.com/questions/4832603/how-could-i-temporary-redirect-stdout-to-a-file-in-a-c-program
    int bak, new;
    if(in->verbose == 0){
        fflush(stdout);
        bak = dup(2);
        new = open("/dev/null", O_WRONLY);
        dup2(new, 2);
        close(new);
    }

    //Build Mutex
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    //Build Timeout Information
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    //Number of Threads to spawn
    int number_threads = in->numsockets * 2;

    //Build Input Struct for each pair of threads
    struct function_in* input = malloc(sizeof(struct function_in) * in->numsockets);
    int i, socket;
    for(i=0; i<in->numsockets; i++){
        //Init Socket
        socket = mp_socket(AF_INET, SOCK_MPTCP, 0);
        setSocketTimeout(socket, &timeout);

        //Initialize Input
        input[i].socket = socket;
        input[i].lock = &lock;
        input[i].port = in->ports[i];
        input[i].pid = i+1;
        input[i].server = in->server;
        input[i].timeout = in->timeout;
        input[i].verbose = in->verbose;
    }

    //Build References to all threads
    if(number_threads < 1) threadError(1);
    pthread_t *threads = malloc(sizeof(pthread_t) * number_threads);
    spawnThreads(threads, number_threads, input, in->numsockets);



    //Gather Threads and Free Memory
    gatherThreads(threads, number_threads);

    //Reattach StdErr
    if(in->verbose == 0){
        fflush(stdout);
        dup2(bak, 2);
        close(bak);
    }

    //Destroy Sockets
    for(i=0; i< in->numsockets; i++)
        close(input[i].socket);
    free(input);
    free(threads);
    pthread_mutex_destroy(&lock);

}

//Spawns a given number of threads to send and receive
void spawnThreads(pthread_t *threads, int size, struct function_in* input,int inputSize){
    int i;
    for(i=0; i<size; i++){
        if(i%2 == 0){ //Even Threads Send
            pthread_create(&(threads[i]), NULL, thread_send, &input[i/2]);
        }
        else{ //Odd Threads Send
            pthread_create(&(threads[i]), NULL, thread_receive, &input[i/2]); 
        }
    }
}


//Gathers all outstanding threads
void gatherThreads(pthread_t *threads, int size){
    int i;
    for(i=0; i<size; i++) 
        pthread_join(threads[i], NULL);
}

/**********************************Send and Receive Functions*****************************/
void* thread_send(void* input){
    if(input == NULL) threadError(6);
    struct function_in* in = (struct function_in*)input;

    if(in->socket == -1){
        //printf("Invalid Socket in thread_send\n");
        pthread_exit(0);
    }

    //Build Target Address to Initialize Packets
    struct sockaddr_in server;
    initForeignAddress(in->port, in->server, &server);

    if(mp_connect(in->socket, (struct sockaddr*)&server, sizeof(server)) != 0){
        //printf("Error occurred in a thread's mp connect\n");
        pthread_exit(0);
    }

    //Pointer to the item to send
    struct queue_item* item = NULL;
    struct queue_item* lastSent = NULL;

    //Timer to handle dropped packets
    //taken from http://stackoverflow.com/questions/459691/best-timing-method-in-c
    clock_t start, diff;

    //Loop Until Various Exit Conditions Met
    while(1){
        pthread_mutex_lock(in->lock);
            
        //If there is nothing to send, exit the thread
        if(queue_empty() == 1){
            pthread_mutex_unlock(in->lock);
            close(in->socket);
            pthread_exit(0);
        }

        //Get the next item
        if(item == NULL || item->index < queue_front){
            item = queue_claim(in->pid, in->socket, &server);
        }
        pthread_mutex_unlock(in->lock);

        //Try to send the item
        if(item != NULL && item != lastSent){

            //Print if Verbose Set
            if(in->verbose == 1){
                printf("Sender %d Sending Packet:\n", in->pid);
                inspectPkt(item->pkt);
            }

            //Check for Errors in send
            errno = 0;
            mp_send(in->socket, item->pkt, MSS, 0);
            switch(errno){
                case 0:
                    start = clock(); //Start the clock
                    lastSent = item;
                    break;
                case EAGAIN: break;
                default: 
                    //printf("Connection Closed\n");
                    pthread_exit(0);
                    break;
            }

        }
        //If you had the packet for a while
        //and it hasn't budged, it is probably dropped
        //So relinquish control to the queue
        else if(item != NULL && item == lastSent){
            diff = clock() - start;

            int sec  = ((diff * 1000) / CLOCKS_PER_SEC) / 1000;
            if(sec >= in->timeout){

                //Lock and unclaim packet
                pthread_mutex_lock(in->lock);
                queue_unclaim(item->pkt->header->seq_num);
                pthread_mutex_unlock(in->lock);

                //Reset Local Variables
                lastSent = NULL;
            }
        }
    }

    //printf("Sending Thread\n");
    return NULL;
}

void* thread_receive(void* input){
    if(input == NULL) threadError(7);
    struct function_in* in = (struct function_in*)input;

    if(in->socket == -1){
        printf("Invalid Socket in thread_receive\n");
        pthread_exit(0);
    }

    //Init Receive Buffer
    struct packet rcv;
    allocatePacket(&rcv);

    //Loop until various end conditions met
    while(1){

        //If the Queue is empty, unlock and exit
        pthread_mutex_lock(in->lock);
        //printf("Receiver %d in loop\n", in->pid);
        if(queue_empty() == 1){
            pthread_mutex_unlock(in->lock);
            //printf("Receiver %d Exiting\n", in->pid);     
            pthread_exit(0);
        }
        else{
            //printf("Queue not found empty\n");
            //queue_print();
        }
        pthread_mutex_unlock(in->lock);

        //Listen for ACKs
        errno = 0;
        mp_recv(in->socket, &rcv, MSS, 0);
        switch(errno){
            case 0:
                if(in->verbose == 1){
                    printf("Pair %d received packet\n", in->pid);
                    inspectPkt(&rcv);
                }

                pthread_mutex_lock(in->lock);
                queue_ack(rcv.header->ack_num);                
                pthread_mutex_unlock(in->lock);
            case EAGAIN: break;//Maybe Wait
            default:
                //printf("Receiver %d exiting\n", in->pid);
                pthread_exit(0);
        }
    }

    pthread_exit(0);
}


/***********************************Utility Functions************************************/
//Displays Error Depending On where error occurred
void threadError(int function){
    switch(function){
        case 0: printf("Invalid Connection # Passed to Init Threads\n"); break;
        case 1: printf("Invalid # of threads to create\n"); break;
        case 2: printf("Nonvalid Ports array passed to threads\n"); break;
        case 3: printf("Null input passed to initThreads\n"); break;
        case 4: printf("Null input passed to initWindow\n"); break;
        case 5: printf("Invalid size passed to initWindow\n"); break;
        case 6: printf("Null arguments passed to thread_send\n"); break;
        case 7: printf("Null arguments passed to thread_receive\n"); break;
        case 8: printf("Null server passed to initThreads\n"); break;
    }
    exit(function);
}

void initWindow(struct queue_item** window, int size){
    if(window == NULL) threadError(4);
    if(size < 0 || size > RWIN/MSS) threadError(5);
    int i;
    for(i=0; i<size; i++)
        window[i] = NULL;
}
