#include <zmq.h>
#include <czmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

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

int main (void) {
    /* 1. create zmq context */
    void *context = zmq_ctx_new ();
    /* 2. create ROUTER socket */
    void *sink = zmq_socket (context, ZMQ_PAIR);
    void *sender = zmq_socket (context, ZMQ_PAIR);
    /* 3. bind ROUTER socket */
    int rc = zmq_bind (sink, "inproc://zframe.test");
    assert(rc == 0);
    zmq_connect (sender, "inproc://zframe.test");
    

    /* [WRITE] Serialize message into the frame */
    size_t frame_size = 2 + 2;          //  Signature and message ID
    zframe_t *sframe = zframe_new (NULL, frame_size);
    byte *data = zframe_data (sframe);  // Address of frame data
    /* Add data to frame */
    data [0] = (byte) (0x5A & 255);
    data [1] = (byte) (0x53 & 255);
    data [2] = (byte) (0x2 & 255);
    /* Send frame */
    if (zframe_send (&sframe, sender, 0)) {
        zframe_destroy (&sframe);
    }

    /* [READ] Deserialize message from frame */
    zframe_t *rframe = NULL;
    rframe = zframe_recv (sink);
    data = zframe_data(rframe);
    uint16_t signature = ((uint16_t) (data [0]) << 8) + (uint16_t) (data [1]);
    byte command = (byte) data[2];
    printf("%x\n", signature);
    printf("%x\n", command);
    zframe_destroy (&rframe);

    /* x. close & destroy */
    zmq_close (sink);
    zmq_close (sender);
    zmq_ctx_destroy (context);

    return EXIT_SUCCESS;
}

