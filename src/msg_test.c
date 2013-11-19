#include <zmq.h>
#include <czmq.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../include/msg.h"

int 
main (void) 
{
    /* 1. create zmq context */
    void *context = zmq_ctx_new ();
    /* 2. create sockets */
    void *sink = zmq_socket (context, ZMQ_DEALER);
    void *sender = zmq_socket (context, ZMQ_DEALER);
    /* 3. bind/connect sockets */
    int rc = zmq_bind (sink, "inproc://zframe.test");
    assert(rc == 0);
    zmq_connect (sender, "inproc://zframe.test");
    
    /* [SEND] LAST STATE */
    zs_msg_send_last_state (sender, 0xFF);
    
    /* [RECV] LAST STATE */
    zs_msg_t *self = zs_msg_recv (sink);
    uint64_t last_state = zs_msg_get_state (self);
    printf("Command %d\n", zs_msg_get_cmd (self));
    printf("last state %"PRIx64"\n", last_state);
    
    // cleanup 
    zs_msg_destroy (&self);

    /* [SEND] FILE LIST */
    zlist_t *filemeta_list = zlist_new ();
    zs_fmetadata_t *fmetadata = zs_fmetadata_new ();
    zs_fmetadata_set_path (fmetadata, "%s", "a.txt");
    zs_fmetadata_set_size (fmetadata, 0x1533);
    zs_fmetadata_set_timestamp (fmetadata, 0x1dfa533);
    zlist_append(filemeta_list, fmetadata);
    zs_fmetadata_t *fmetadata2 = zs_fmetadata_new ();
    zs_fmetadata_set_path (fmetadata2, "%s", "b.txt");
    zs_fmetadata_set_size (fmetadata2, 0x1544);
    zs_fmetadata_set_timestamp (fmetadata2, 0x1dfa544);
    zlist_append(filemeta_list, fmetadata2);

    
    zs_msg_send_file_list (sender, filemeta_list);
    
    /* [RECV] FILE LIST */
    self = zs_msg_recv (sink);
    fmetadata = zs_msg_fmetadata_first (self);
    printf("Command %d\n", zs_msg_get_cmd (self));
    while (fmetadata) {
        char *path = zs_fmetadata_get_path (fmetadata);
        uint64_t size = zs_fmetadata_get_size (fmetadata);
        uint64_t timestamp = zs_fmetadata_get_timestamp (fmetadata);
        printf("%s %"PRIx64" %"PRIx64"\n", path, size, timestamp);
        
        free (path);
        fmetadata = zs_msg_fmetadata_next (self);
    }

    // cleanup
    zs_msg_destroy (&self);
   
    /* [SEND] NO UPDATE */
    zs_msg_send_no_update(sender);

    /* [RECV] NO UPDATE */
    self = zs_msg_recv (sink);
    printf("Command %d\n", zs_msg_get_cmd (self));
    
    // cleanup
    zs_msg_destroy (&self);
   

    /* [SEND] REQUEST FILES */
    zlist_t *paths = zlist_new ();
    zlist_append(paths, "test1.txt");
    zlist_append(paths, "test2.txt");
    zlist_append(paths, "test3.txt");

    zs_msg_send_request_files (sender, paths);

    /* [RECV] REQUEST FILES */
    self = zs_msg_recv (sink);
    printf("Command %d\n", zs_msg_get_cmd (self));
    char *path = zs_msg_fpaths_first (self);
    while (path) {
        printf("%s\n", path);
        // next
        free(path);
        path = zs_msg_fpaths_next (self);
    }

    // cleanup
    zs_msg_destroy (&self);

    /* 4. close & destroy */
    zmq_close (sink);
    zmq_close (sender);
    zmq_ctx_destroy (context);

    return EXIT_SUCCESS;
}

