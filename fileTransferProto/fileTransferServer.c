/* testdata is a file generated with the dd app and /dev/urandom. It´s size counts 1GB */

#include <czmq.h>
#include <zmq.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

FILE* file;

/* not complete - here should be the handling from files done later  */
FILE* fileHandle(){
    file = fopen("/home/burne/Documents/testdata", "r");
    assert(file);
    return file;
}

int handleClient(void *serverD){
    while (true) {
        printf("Get the credit size...");
        char *credit = zstr_recv(serverD);
        printf("\n...%s set as credit size.\n",credit);
        int chunk_size = atoi(credit);
        
        while (true) {
            byte *data = calloc(chunk_size, sizeof(byte));
            size_t size = fread(data, 1, chunk_size, fileHandle()); //read form the file
            
            zframe_t *chunk = zframe_new (data, size);  //get the frame ready to send
            zframe_send (&chunk, serverD, 0);

            free(data);

            if (size == 0){
                printf("Complete file sent!\n");
                break; // Always end with a zero-size frame
            }

        }
        fclose (file);
    }
    return 0;
}

int main(int argc, char **argv){
    zctx_t *ctx = zctx_new();
    void *serverD = zsocket_new(ctx, ZMQ_DEALER);

  //  zsocket_set_hwm(serverD, 0); //cannot drop messages
    zsocket_bind(serverD, "tcp://*:9989");
    handleClient(serverD);

    zctx_destroy(&ctx);
    return EXIT_SUCCESS;
}
