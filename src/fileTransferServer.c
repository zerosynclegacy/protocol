/*
    This program offers a prototype of the file transfering part of the zerosynch protocol.
    @@Author Bernhard Finger
    @@Language C
*/


/* testdata is a file generated with the dd app and /dev/urandom. It´s size counts 1GB */

#include "zsync_classes.h"

#define CHUNK_SIZE 30000

FILE* file;

/* not complete - here should be the handling from files done later  */
FILE* fileHandle(){
    file = fopen("/home/burne/Documents/testdata", "r");
    assert(file);
    return file;
}

int handleClient(void *serverD){
    uint64_t seq = 0;
    uint64_t offset = 0;

    while (true) {
        file = fileHandle();
        printf("Get the credit size...");
        zmsg_t *zmsg = zmsg_recv (serverD);
        zs_msg_t *rcvMsg = zs_msg_unpack (zmsg);
        printf("... %"PRId64" \n", zs_msg_get_credit(rcvMsg));
        
        while (true) {
            if(feof(file)){
                printf("End of file reached. \n");
                zmsg_t *msg = zmsg_new ();
                zframe_t *endFrame = zframe_new(NULL, 0);
                zs_msg_pack_chunk(msg, seq++, "/home/burne/Documents/testdata", offset++, endFrame);
                zmsg_send (&msg, serverD);
                printf("End message sent!\n");
                break;
            }
           
            byte *data = calloc(CHUNK_SIZE, sizeof(byte)); 
            size_t size = fread(data, 1, CHUNK_SIZE, file); //read form the file

            if(size != CHUNK_SIZE){
                data = realloc(data, size);
                printf("Realloced data! %d \n", (int)size);
            }

            zmsg_t *msg = zmsg_new ();
            zframe_t *chunk = zframe_new(data, size); 
            
            if(zs_msg_pack_chunk(serverD, seq++, "/home/burne/Documents/testdata", offset++, chunk ) == 1){
                printf("Attention: Sending failed.\n");
            }
            zmsg_send (&msg, serverD);
            printf("Chunk sent, size: %d \n", (int)size);
            free(data);
                
            /*if (size == 0){
                printf("Complete file sent!\n");
                break; // Always end with a zero-size frame
            }*/
        }
        fclose (file);
    }
    return 0;
}

int main(int argc, char **argv){
    printf("Get da server starting...\n");
    zctx_t *ctx = zctx_new();
    void *serverD = zsocket_new(ctx, ZMQ_DEALER);

  //  zsocket_set_hwm(serverD, 0); //cannot drop messages
    zsocket_bind(serverD, "tcp://*:9989");
    handleClient(serverD);

    zctx_destroy(&ctx);
    return EXIT_SUCCESS;
}
