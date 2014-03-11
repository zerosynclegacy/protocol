/*  =========================================================================
    zsync_credit_msg - work with ZSYNC workers messages

    Generated codec implementation for zsync_credit_msg
    -------------------------------------------------------------------------
    Copyright (c) 2013-2014 Kevin Sapper, Bernhard Finger                   
    Copyright other contributors as noted in the AUTHORS file.              
                                                                            
    This file is part of ZeroSync, see http://zerosync.org.                 
                                                                            
    This is free software; you can redistribute it and/or modify it under   
    the terms of the GNU Lesser General Public License as published by the  
    Free Software Foundation; either version 3 of the License, or (at your  
    option) any later version.                                              
                                                                            
    This software is distributed in the hope that it will be useful, but    
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTA-   
    BILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General  
    Public License for more details.                                        
                                                                            
    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see http://www.gnu.org/licenses/.      
    =========================================================================
*/

/*
@header
    zsync_credit_msg - work with ZSYNC workers messages
@discuss
@end
*/

#include <czmq.h>
#include "../include/zsync_credit_msg.h"

//  Structure of our class

struct _zsync_credit_msg_t {
    zframe_t *address;          //  Address of peer if any
    int id;                     //  zsync_credit_msg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    char *sender;               //  
    uint64_t req_bytes;         //  
    uint64_t recv_bytes;        //  
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Strings are encoded with 1-byte length
#define STRING_MAX  255

//  Put a block to the frame
#define PUT_BLOCK(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
    }

//  Get a block from the frame
#define GET_BLOCK(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
    }

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
    }

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
    }

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
    }

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
    }

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
    }

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
    }

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
    }

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
    }

//  Put a string to the frame
#define PUT_STRING(host) { \
    string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
    }

//  Get a string from the frame
#define GET_STRING(host) { \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
    }


//  --------------------------------------------------------------------------
//  Create a new zsync_credit_msg

zsync_credit_msg_t *
zsync_credit_msg_new (int id)
{
    zsync_credit_msg_t *self = (zsync_credit_msg_t *) zmalloc (sizeof (zsync_credit_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync_credit_msg

void
zsync_credit_msg_destroy (zsync_credit_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_credit_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->address);
        free (self->sender);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive and parse a zsync_credit_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zsync_credit_msg_t *
zsync_credit_msg_recv (void *input)
{
    assert (input);
    zsync_credit_msg_t *self = zsync_credit_msg_new (0);
    zframe_t *frame = NULL;
    size_t string_size;

    //  Read valid message frame from socket; we loop over any
    //  garbage data we might receive from badly-connected peers
    while (true) {
        //  If we're reading from a ROUTER socket, get address
        if (zsocket_type (input) == ZMQ_ROUTER) {
            zframe_destroy (&self->address);
            self->address = zframe_recv (input);
            if (!self->address)
                goto empty;         //  Interrupted
            if (!zsocket_rcvmore (input))
                goto malformed;
        }
        //  Read and parse command in frame
        frame = zframe_recv (input);
        if (!frame)
            goto empty;             //  Interrupted

        //  Get and check protocol signature
        self->needle = zframe_data (frame);
        self->ceiling = self->needle + zframe_size (frame);
        uint16_t signature;
        GET_NUMBER2 (signature);
        if (signature == (0xAAA0 | 1))
            break;                  //  Valid signature

        //  Protocol assertion, drop message
        while (zsocket_rcvmore (input)) {
            zframe_destroy (&frame);
            frame = zframe_recv (input);
        }
        zframe_destroy (&frame);
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_CREDIT_MSG_REQUEST:
            free (self->sender);
            GET_STRING (self->sender);
            GET_NUMBER8 (self->req_bytes);
            break;

        case ZSYNC_CREDIT_MSG_UPDATE:
            free (self->sender);
            GET_STRING (self->sender);
            GET_NUMBER8 (self->recv_bytes);
            break;

        case ZSYNC_CREDIT_MSG_ABORT:
            free (self->sender);
            GET_STRING (self->sender);
            break;

        case ZSYNC_CREDIT_MSG_TERMINATE:
            free (self->sender);
            GET_STRING (self->sender);
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    return self;

    //  Error returns
    malformed:
        printf ("E: malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zsync_credit_msg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Send the zsync_credit_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zsync_credit_msg_send (zsync_credit_msg_t **self_p, void *output)
{
    assert (output);
    assert (self_p);
    assert (*self_p);

    //  Calculate size of serialized data
    zsync_credit_msg_t *self = *self_p;
    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZSYNC_CREDIT_MSG_REQUEST:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  req_bytes is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_CREDIT_MSG_UPDATE:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  recv_bytes is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_CREDIT_MSG_ABORT:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            break;
            
        case ZSYNC_CREDIT_MSG_TERMINATE:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            break;
            
        default:
            printf ("E: bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    size_t string_size;
    int frame_flags = 0;
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_CREDIT_MSG_REQUEST:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->req_bytes);
            break;
            
        case ZSYNC_CREDIT_MSG_UPDATE:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->recv_bytes);
            break;
            
        case ZSYNC_CREDIT_MSG_ABORT:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;
            
        case ZSYNC_CREDIT_MSG_TERMINATE:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;
            
    }
    //  If we're sending to a ROUTER, we send the address first
    if (zsocket_type (output) == ZMQ_ROUTER) {
        assert (self->address);
        if (zframe_send (&self->address, output, ZFRAME_MORE)) {
            zframe_destroy (&frame);
            zsync_credit_msg_destroy (self_p);
            return -1;
        }
    }
    //  Now send the data frame
    if (zframe_send (&frame, output, frame_flags)) {
        zframe_destroy (&frame);
        zsync_credit_msg_destroy (self_p);
        return -1;
    }
    //  Destroy zsync_credit_msg object
    zsync_credit_msg_destroy (self_p);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the REQUEST to the socket in one step

int
zsync_credit_msg_send_request (
    void *output,
    char *sender,
    uint64_t req_bytes)
{
    zsync_credit_msg_t *self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_REQUEST);
    zsync_credit_msg_set_sender (self, sender);
    zsync_credit_msg_set_req_bytes (self, req_bytes);
    return zsync_credit_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the UPDATE to the socket in one step

int
zsync_credit_msg_send_update (
    void *output,
    char *sender,
    uint64_t recv_bytes)
{
    zsync_credit_msg_t *self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_UPDATE);
    zsync_credit_msg_set_sender (self, sender);
    zsync_credit_msg_set_recv_bytes (self, recv_bytes);
    return zsync_credit_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ABORT to the socket in one step

int
zsync_credit_msg_send_abort (
    void *output,
    char *sender)
{
    zsync_credit_msg_t *self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_ABORT);
    zsync_credit_msg_set_sender (self, sender);
    return zsync_credit_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the TERMINATE to the socket in one step

int
zsync_credit_msg_send_terminate (
    void *output,
    char *sender)
{
    zsync_credit_msg_t *self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_TERMINATE);
    zsync_credit_msg_set_sender (self, sender);
    return zsync_credit_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zsync_credit_msg message

zsync_credit_msg_t *
zsync_credit_msg_dup (zsync_credit_msg_t *self)
{
    if (!self)
        return NULL;
        
    zsync_credit_msg_t *copy = zsync_credit_msg_new (self->id);
    if (self->address)
        copy->address = zframe_dup (self->address);

    switch (self->id) {
        case ZSYNC_CREDIT_MSG_REQUEST:
            copy->sender = strdup (self->sender);
            copy->req_bytes = self->req_bytes;
            break;

        case ZSYNC_CREDIT_MSG_UPDATE:
            copy->sender = strdup (self->sender);
            copy->recv_bytes = self->recv_bytes;
            break;

        case ZSYNC_CREDIT_MSG_ABORT:
            copy->sender = strdup (self->sender);
            break;

        case ZSYNC_CREDIT_MSG_TERMINATE:
            copy->sender = strdup (self->sender);
            break;

    }
    return copy;
}



//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zsync_credit_msg_dump (zsync_credit_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_CREDIT_MSG_REQUEST:
            puts ("REQUEST:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            printf ("    req_bytes=%ld\n", (long) self->req_bytes);
            break;
            
        case ZSYNC_CREDIT_MSG_UPDATE:
            puts ("UPDATE:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            printf ("    recv_bytes=%ld\n", (long) self->recv_bytes);
            break;
            
        case ZSYNC_CREDIT_MSG_ABORT:
            puts ("ABORT:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            break;
            
        case ZSYNC_CREDIT_MSG_TERMINATE:
            puts ("TERMINATE:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message address

zframe_t *
zsync_credit_msg_address (zsync_credit_msg_t *self)
{
    assert (self);
    return self->address;
}

void
zsync_credit_msg_set_address (zsync_credit_msg_t *self, zframe_t *address)
{
    if (self->address)
        zframe_destroy (&self->address);
    self->address = zframe_dup (address);
}


//  --------------------------------------------------------------------------
//  Get/set the zsync_credit_msg id

int
zsync_credit_msg_id (zsync_credit_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zsync_credit_msg_set_id (zsync_credit_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

char *
zsync_credit_msg_command (zsync_credit_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_CREDIT_MSG_REQUEST:
            return ("REQUEST");
            break;
        case ZSYNC_CREDIT_MSG_UPDATE:
            return ("UPDATE");
            break;
        case ZSYNC_CREDIT_MSG_ABORT:
            return ("ABORT");
            break;
        case ZSYNC_CREDIT_MSG_TERMINATE:
            return ("TERMINATE");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the sender field

char *
zsync_credit_msg_sender (zsync_credit_msg_t *self)
{
    assert (self);
    return self->sender;
}

void
zsync_credit_msg_set_sender (zsync_credit_msg_t *self, char *format, ...)
{
    //  Format sender from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->sender);
    self->sender = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the req_bytes field

uint64_t
zsync_credit_msg_req_bytes (zsync_credit_msg_t *self)
{
    assert (self);
    return self->req_bytes;
}

void
zsync_credit_msg_set_req_bytes (zsync_credit_msg_t *self, uint64_t req_bytes)
{
    assert (self);
    self->req_bytes = req_bytes;
}


//  --------------------------------------------------------------------------
//  Get/set the recv_bytes field

uint64_t
zsync_credit_msg_recv_bytes (zsync_credit_msg_t *self)
{
    assert (self);
    return self->recv_bytes;
}

void
zsync_credit_msg_set_recv_bytes (zsync_credit_msg_t *self, uint64_t recv_bytes)
{
    assert (self);
    self->recv_bytes = recv_bytes;
}



//  --------------------------------------------------------------------------
//  Selftest

int
zsync_credit_msg_test (bool verbose)
{
    printf (" * zsync_credit_msg: ");

    //  @selftest
    //  Simple create/destroy test
    zsync_credit_msg_t *self = zsync_credit_msg_new (0);
    assert (self);
    zsync_credit_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zctx_t *ctx = zctx_new ();
    assert (ctx);

    void *output = zsocket_new (ctx, ZMQ_DEALER);
    assert (output);
    zsocket_bind (output, "inproc://selftest");
    void *input = zsocket_new (ctx, ZMQ_ROUTER);
    assert (input);
    zsocket_connect (input, "inproc://selftest");
    
    //  Encode/send/decode and verify each message type

    self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_REQUEST);
    zsync_credit_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_credit_msg_set_req_bytes (self, 123);
    zsync_credit_msg_send (&self, output);
    
    self = zsync_credit_msg_recv (input);
    assert (self);
    assert (streq (zsync_credit_msg_sender (self), "Life is short but Now lasts for ever"));
    assert (zsync_credit_msg_req_bytes (self) == 123);
    zsync_credit_msg_destroy (&self);

    self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_UPDATE);
    zsync_credit_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_credit_msg_set_recv_bytes (self, 123);
    zsync_credit_msg_send (&self, output);
    
    self = zsync_credit_msg_recv (input);
    assert (self);
    assert (streq (zsync_credit_msg_sender (self), "Life is short but Now lasts for ever"));
    assert (zsync_credit_msg_recv_bytes (self) == 123);
    zsync_credit_msg_destroy (&self);

    self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_ABORT);
    zsync_credit_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_credit_msg_send (&self, output);
    
    self = zsync_credit_msg_recv (input);
    assert (self);
    assert (streq (zsync_credit_msg_sender (self), "Life is short but Now lasts for ever"));
    zsync_credit_msg_destroy (&self);

    self = zsync_credit_msg_new (ZSYNC_CREDIT_MSG_TERMINATE);
    zsync_credit_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_credit_msg_send (&self, output);
    
    self = zsync_credit_msg_recv (input);
    assert (self);
    assert (streq (zsync_credit_msg_sender (self), "Life is short but Now lasts for ever"));
    zsync_credit_msg_destroy (&self);

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
