/*
    This program offers a prototype of the file transfering part of the zerosynch protocol.
    @@Author Bernhard Finger   
    @@Language C
*/

#include <zmq.h>
#include <czmq.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/msg.h"

int credit = 250000;
//static char *signature = "ZS";

void give_credit(int credit, void *dealer){
    char puf[32];
    sprintf(puf, "%d", credit);
    printf("Sending... %s credit size.\n",puf);
    zstr_send(dealer, puf);
}

/* this function is handling the receiving of the chunks sent from server  */
int rcv_chunks(void *dealer){
    size_t total=0;
    size_t chunks=0;

    FILE *rcvdFile = fopen("/home/burne/Documents/rcvtestdata", "wb"); 
    assert(rcvdFile);

    while (true) {
       size_t size;
       zframe_t *frame = zframe_recv(dealer);  //Each chunk is coded as a frame - now we receive it from our dealer socket
       byte *data = zframe_data(frame);

       if(fwrite(data, 1, zframe_size(frame), rcvdFile) < 0){
           printf("Oh shit!\n");
       }

       if (!frame){
           size=0;
           printf("No more chunk received.\n");
           break; // Shutting down, quit
       }
       chunks++;
   
       size = zframe_size(frame);
       zframe_destroy(&frame);
       total += size;
       
       if (size < credit){
           printf("%d bytes totally received.\n", (int)total);
           break; // Whole file received
       }
    }
    fclose(rcvdFile);
    return chunks;
}

int main(int argc, char **argv){
    printf("Get the client party starting!\n");
    zctx_t *ctx = zctx_new();
    void *dealer = zsocket_new(ctx, ZMQ_DEALER);
    zsocket_connect(dealer, "tcp://localhost:9989");

    // give_credit(credit, dealer);
    zs_msg_send_last_state (dealer, 0x444);
    printf("Start getting chunks.\n");
    size_t chunksGet = rcv_chunks(dealer);
    

    printf("%d chunks finally get.\n", (int)chunksGet);

    return EXIT_SUCCESS;
}
