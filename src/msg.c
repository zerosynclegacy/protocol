#include <zmq.h>
#include <czmq.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../include/msg.h"

struct _zs_msg_t {
    uint16_t signature;     // zs_msg signature
    int cmd;                // zs_msg command
    byte *needle;           // read/write pointer for serialization
    byte *ceiling;          // valid upper limit for needle
    uint64_t state;
    uint64_t credit;        //given credit for RP 
};

// --------------------------------------------------------------------------
// Network data encoding macros

// Strings are encoded with 2-byte length
#define STRING_MAX 65535 // 2-byte - 1-bit for trailing \0

// Put a block to the frame
#define PUT_BLOCK(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

// Get a block from the frame
#define GET_BLOCK(host,size) { \
    /*if (self->needle + size > self->ceiling) \
    goto malformed; */ \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

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

// Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8) & 255); \
    self->needle [7] = (byte) (((host)) & 255); \
    self->needle += 8; \
}

// Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    /*if (self->needle + 1 > self->ceiling) \
    goto malformed;*/ \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

// Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    /*if (self->needle + 2 > self->ceiling) \
    goto malformed;*/ \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
    + (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

// Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    /*if (self->needle + 8 > self->ceiling) \
    goto malformed;*/ \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
    + ((uint64_t) (self->needle [1]) << 48) \
    + ((uint64_t) (self->needle [2]) << 40) \
    + ((uint64_t) (self->needle [3]) << 32) \
    + ((uint64_t) (self->needle [4]) << 24) \
    + ((uint64_t) (self->needle [5]) << 16) \
    + ((uint64_t) (self->needle [6]) << 8) \
    + (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

// Put a string to the frame
#define PUT_STRING(host) { \
    string_size = strlen (host); \
    PUT_NUMBER2 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

// Get a string from the frame
#define GET_STRING(host) { \
    GET_NUMBER2 (string_size); \
    /*if (self->needle + string_size > (self->ceiling)) \
    goto malformed; */ \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

// --------------------------------------------------------------------------
// Create a new zs_msg

zs_msg_t * 
zs_msg_new (int cmd) 
{
    zs_msg_t *self = (zs_msg_t *) zmalloc (sizeof (zs_msg_t));
    self->cmd = cmd;
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zs_msg

void 
zs_msg_destroy (zs_msg_t **self_p) 
{
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

zs_msg_t *
zs_msg_recv (void *input) 
{
    zs_msg_t *self = zs_msg_new (0);

    // receive data
    zframe_t *rframe = NULL;
    rframe = zframe_recv (input);
    self->needle = zframe_data(rframe);
    
    // TODO ceiling
    self->ceiling = self->needle + 11;
    
    // parse message
    GET_NUMBER2(self->signature);
    GET_NUMBER1(self->cmd);
    
    switch(self->cmd) {
        case 0x1:
            GET_NUMBER8(self->state);
            break;
        case ZS_CMD_GIVE_CREDIT:
            GET_NUMBER8(self->credit);      
            break;
        default:
            break;
    }

    // cleanup
    zframe_destroy (&rframe);
    return self;
}


// --------------------------------------------------------------------------
// Send the zs_msg to the socket, and destroy it
// Returns 0 if OK, else -1

int 
zs_msg_send (zs_msg_t **self_p, void *output) 
{
    assert (output);
    assert (self_p);
    assert (*self_p);

    zs_msg_t *self = *self_p; 
    
    size_t frame_size = 2 + 1 + 8;          //  Signature and command ID
    zframe_t *sframe = zframe_new (NULL, frame_size);
    
    self->needle = zframe_data (sframe);  // Address of frame data
    self->ceiling = self->needle + frame_size;
    
    /* Add data to frame */
    PUT_NUMBER2 (0x5A53);
    PUT_NUMBER1 (self->cmd);

    switch(self->cmd) {
        case 0x1:
            PUT_NUMBER8 (self->state);
            break;
        case ZS_CMD_GIVE_CREDIT:
            PUT_NUMBER8 (self->credit);
            break;
        default:
            break;
    }
    
    /* Send frame */
    if (zframe_send (&sframe, output, 0)) {
        zframe_destroy (&sframe);
    }
    
    /* Cleanup */
    zs_msg_destroy (&self);

    return 0;
}

// --------------------------------------------------------------------------
// Send the LAST_STATE to the socket in one step

int 
zs_msg_send_last_state (void *output, uint64_t state) 
{
    zs_msg_t *self = zs_msg_new (0x1);
    zs_msg_set_state (self, state);
    return zs_msg_send (&self, output);
}

// -------------------------------------------------------------------------
// Send the given credit to a RP (receiving peer)
int 
zs_msg_send_give_credit (void *output, uint64_t credit)
{ 
    assert(output);
    assert(credit);

    zs_msg_t *msg = zs_msg_new (ZS_CMD_GIVE_CREDIT);
    zs_msg_set_credit (msg, credit); 
    return zs_msg_send (&msg, output); 
}

// --------------------------------------------------------------------------
// Get/Set the state field

void 
zs_msg_set_state (zs_msg_t *self, uint64_t state) 
{
    assert (self);
    self->state = state;
}

uint64_t 
zs_msg_get_state (zs_msg_t *self) 
{
    assert (self);
    return self->state;
}

void 
zs_msg_set_credit (zs_msg_t *self, uint64_t credit)
{
    assert (self);
    self->credit = credit;
}

uint64_t 
zs_msg_get_credit (zs_msg_t *self)
{
    assert (self);
    return self->credit;
}
