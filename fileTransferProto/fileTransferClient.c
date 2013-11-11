/*
    This program offers a prototype of the file transfering part of the zerosynch protocol.
    @@Author Bernhard Finger   
    @@language C
*/

#include <zmq.h>
#include <czmq.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//static char *signature = "ZS";

void give_credit(int credit, void *dealer){
    char puf[16];
    sprintf(puf, "%d", credit);
    puf[strlen(puf)] = '\0';    //terminating the string
    printf("Sending... %s credit size.\n",puf);
    zstr_send(dealer, puf);
}

int rcv_chunks(void *dealer){
    size_t total=0;
    size_t chunks=0;
   
    while (true) {
       zframe_t *frame = zframe_recv(dealer);  
       
       if (!frame){
           printf("No more chunk received.\n");
           break; // Shutting down, quit
       }
       chunks++;
   
       size_t size = zframe_size(frame);
       zframe_destroy(&frame);
       total += size;
        
       if (size == 0){
           printf("%d bytes totally received.\n", (int)total);
           break; // Whole file received
       }
    }
   return chunks;
}

int main(int argc, char **argv){
    printf("Get the client party starting!\n");
    zctx_t *ctx = zctx_new();
    void *dealer = zsocket_new(ctx, ZMQ_DEALER);
    zsocket_connect(dealer, "tcp://localhost:9989");

    give_credit(1000, dealer);
    printf("Start getting chunks.\n");
    size_t chunksGet = rcv_chunks(dealer);
    
    printf("%d chunks finally get.\n", (int)chunksGet);

    return EXIT_SUCCESS;
}
