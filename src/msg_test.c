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
    
    /* [WRITE/SEND] Serialize message into the frame */
    zs_msg_send_last_state (sender, 0xFF);
    
    /* [READ/RECV] Deserialize message from frame */
    zs_msg_t *self = zs_msg_recv (sink);
    uint64_t last_state = zs_msg_get_state (self);
    printf("last state %"PRIx64"\n", last_state);
    
    // cleanup 
    zs_msg_destroy (&self);

    /* [SEND] FILE LIST */
    zlist_t *filemeta_list = zlist_new ();
    zs_filemeta_data_t *filemeta_data = zs_filemeta_data_new ();
    zs_filemeta_data_set_path (filemeta_data, "a.txt");
    zs_filemeta_data_set_size (filemeta_data, 0x1533);
    zs_filemeta_data_set_timestamp (filemeta_data, 0x1dfa533);
    zlist_append(filemeta_list, filemeta_data);
    
    zs_msg_send_file_list (sender, filemeta_list);
    
    /* [RECV] FILE LIST */
    self = zs_msg_recv (sink);
    zlist_t *rfilemeta_list = zs_msg_get_file_meta (self);
    filemeta_data = zlist_first (rfilemeta_list);
    char * path = zs_filemeta_data_get_path (filemeta_data);
    uint64_t size = zs_filemeta_data_get_size (filemeta_data);
    uint64_t timestamp = zs_filemeta_data_get_timestamp (filemeta_data);
    printf("%s %"PRIx64" %"PRIx64"\n", path, size, timestamp);

    // cleanup
    free (path);
    zs_filemeta_data_destroy (&filemeta_data); 
    zs_msg_destroy (&self);
    
    /* 4. close & destroy */
    zmq_close (sink);
    zmq_close (sender);
    zmq_ctx_destroy (context);

    return EXIT_SUCCESS;
}

