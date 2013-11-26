/*
    This program offers a prototype of the file transfering part of the zerosynch protocol.
    @@Author Bernhard Finger   
    @@Language C
*/

#include "zsync_classes.h"

//Static credit
#define credit 250000

/* this function is handling the receiving of the chunks sent from server  */
int rcv_chunks(void *dealer){
    size_t total = 0;
    size_t chunks = 0;
    size_t size = 0;

    FILE *rcvdFile = fopen("/home/burne/Documents/rcvtestdata", "wb"); 
    assert(rcvdFile);

    while (true) {
        zmsg_t *zmsg = zmsg_recv (dealer); 
       zs_msg_t *msg = zs_msg_unpack (zmsg);
       zframe_t *chunk_frame = zs_msg_get_chunk(msg);
       printf("%d bytes of chunk received.\n", (int)zframe_size(chunk_frame));

       if (zframe_size(chunk_frame) == 0){
           printf("File completely received.\n");
           break; 
       }
       
       byte *data = zframe_data(chunk_frame);

       if(fwrite(data, 1, zframe_size(chunk_frame), rcvdFile) < 0){
           printf("Oh shit!\n");
       }

       chunks++;
   
       size = zframe_size(chunk_frame);
       total += size;
       
    }
    fclose(rcvdFile);
    return chunks;
}

int main(int argc, char **argv){
    printf("Get the client party starting!\n");
    zctx_t *ctx = zctx_new();
    void *dealer = zsocket_new(ctx, ZMQ_DEALER);
    zsocket_connect(dealer, "tcp://localhost:9989");
    
    zmsg_t *msg = zmsg_new ();
    if(zs_msg_pack_give_credit (msg, (uint64_t) credit) != 0) {
        printf("PANIC!\n");
    }
    zmsg_send (&msg, dealer);
    printf("Start getting chunks.\n");
    size_t chunksGet = rcv_chunks(dealer);
    

    printf("%d chunks finally get.\n", (int)chunksGet);

    return EXIT_SUCCESS;
}
