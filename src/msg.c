#include <zmq.h>
#include <czmq.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

struct _zs_msg_t {
    uint16_t signature;     // zs_msg signature
    int cmd;                // zs_msg command
    byte *needle;           // read/write pointer for serialization
    byte *ceiling;          // valid upper limit for needle
};

// Opaque class structure
typedef struct _zs_msg_t zs_msg_t;

// Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

// Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8) & 255); \
    self->needle [1] = (byte) (((host)) & 255); \
    self->needle += 2; \
}

// Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
    goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

// Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
    goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
    + (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

// --------------------------------------------------------------------------
// Create a new zs_msg

zs_msg_t * zs_msg_new (int cmd) {
    zs_msg_t *self = (zs_msg_t *) zmalloc (sizeof (zs_msg_t));
    self->cmd = cmd;
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zs_msg

void zs_msg_destroy (zs_msg_t **self_p) {
    assert (self_p);
    if (*self_p) {
        zs_msg_t *self = *self_p;

        // Free object itself
        free (self);
        *self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Receive and parse a fmq_msg from the socket. Returns new object or
// NULL if error. Will block if there's no message waiting.

zs_msg_t *zs_msg_recv (void *input) {
    
    zs_msg_t *self = zs_msg_new (0);

    // receive data
    zframe_t *rframe = NULL;
    rframe = zframe_recv (input);
    self->needle = zframe_data(rframe);
    
    // parse message
    self->signature = ((uint16_t) (self->needle [0]) << 8) + (uint16_t) (self->needle [1]);
    self->cmd = (byte) self->needle [2];
    
    // cleanup
    zframe_destroy (&rframe);
    return self;
}


// --------------------------------------------------------------------------
// Send the zs_msg to the socket, and destroy it
// Returns 0 if OK, else -1

int zs_msg_send (zs_msg_t **self_p, void *output) {

    assert (output);
    assert (self_p);
    assert (*self_p);

    zs_msg_t *self = *self_p; 
    
    size_t frame_size = 2 + 1;          //  Signature and command ID
    zframe_t *sframe = zframe_new (NULL, frame_size);
    
    self->needle = zframe_data (sframe);  // Address of frame data
    self->ceiling = self->needle + frame_size;
    
    /* Add data to frame */
    PUT_NUMBER2 (0x5A53);
    PUT_NUMBER1 (0x1);
    
    /* Send frame */
    if (zframe_send (&sframe, output, 0)) {
        zframe_destroy (&sframe);
    }
    
    /* Cleanup */
    zs_msg_destroy (&self);

    return 0;
}

int main (void) {
    /* 1. create zmq context */
    void *context = zmq_ctx_new ();
    /* 2. create ROUTER socket */
    void *sink = zmq_socket (context, ZMQ_DEALER);
    void *sender = zmq_socket (context, ZMQ_DEALER);
    /* 3. bind ROUTER socket */
    int rc = zmq_bind (sink, "inproc://zframe.test");
    assert(rc == 0);
    zmq_connect (sender, "inproc://zframe.test");
    
    /* [WRITE/SEND] Serialize message into the frame */
    zs_msg_t *self = zs_msg_new (0x1);
    zs_msg_send (&self, sender);
    
    /* [READ/RECV] Deserialize message from frame */
    self = zs_msg_recv (sink);
    printf("signature %"PRIx16" command %x\n", self->signature, self->cmd);
    // cleanup 
    zs_msg_destroy (&self);
    
    /* 4. close & destroy */
    zmq_close (sink);
    zmq_close (sender);
    zmq_ctx_destroy (context);

    return EXIT_SUCCESS;
}

