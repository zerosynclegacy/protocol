/*  =========================================================================
    zsync_ftm_msg - file transfer manager api

    Codec class for zsync_ftm_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

    * The XML model used for this code generation: zsync_ftm_msg
    * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    
    Copyright (c) 2013-2014 Kevin Sapper                                    
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
    zsync_ftm_msg - file transfer manager api
@discuss
@end
*/

#include <czmq.h>
#include "../include/zsync_ftm_msg.h"

//  Structure of our class

struct _zsync_ftm_msg_t {
    zframe_t *routing_id;       //  Routing_id from ROUTER, if any
    int id;                     //  zsync_ftm_msg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    char *sender;               //  UUID that identifies the sender
    zlist_t *paths;             //  
    uint64_t credit;            //  
    char *receiver;             //  UUID that identifies the receiver
    char *path;                 //  Path of file that the 'chunk' belongs to
    uint64_t sequence;          //  
    uint64_t chunk_size;        //  Size of the requested chunk in bytes
    uint64_t offset;            //  File offset for for the chunk in bytes
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
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
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new zsync_ftm_msg

zsync_ftm_msg_t *
zsync_ftm_msg_new (int id)
{
    zsync_ftm_msg_t *self = (zsync_ftm_msg_t *) zmalloc (sizeof (zsync_ftm_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync_ftm_msg

void
zsync_ftm_msg_destroy (zsync_ftm_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_ftm_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->sender);
        if (self->paths)
            zlist_destroy (&self->paths);
        free (self->receiver);
        free (self->path);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Parse a zsync_ftm_msg from zmsg_t. Returns new object or NULL if error.

zsync_ftm_msg_t *
zsync_ftm_msg_decode (zmsg_t *msg)
{
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (0);
    zframe_t *frame = NULL;
    //  Verify message is present
    if (!msg)
        goto empty;

    //  Read and parse command in frame
    frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Interrupted

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 1))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_FTM_MSG_REQUEST:
            GET_STRING (self->sender);
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                self->paths = zlist_new ();
                zlist_autofree (self->paths);
                while (list_size--) {
                    char *string;
                    GET_LONGSTR (string);
                    zlist_append (self->paths, string);
                    free (string);
                }
            }
            break;

        case ZSYNC_FTM_MSG_CREDIT:
            GET_STRING (self->sender);
            GET_NUMBER8 (self->credit);
            break;

        case ZSYNC_FTM_MSG_CHUNK:
            GET_STRING (self->receiver);
            GET_STRING (self->path);
            GET_NUMBER8 (self->sequence);
            GET_NUMBER8 (self->chunk_size);
            GET_NUMBER8 (self->offset);
            break;

        case ZSYNC_FTM_MSG_ABORT:
            GET_STRING (self->sender);
            GET_STRING (self->path);
            break;

        case ZSYNC_FTM_MSG_TERMINATE:
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zmsg_destroy (&msg);
    return self;

    //  Error returns
    malformed:
        printf ("E: malformed message '%d'\n", self->id);
    empty:
        if (frame)
            zframe_destroy (&frame);
        zmsg_destroy (&msg);
        zsync_ftm_msg_destroy (&self);
        return (NULL);
}

//  --------------------------------------------------------------------------
//  Receive and parse a zsync_ftm_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zsync_ftm_msg_t *
zsync_ftm_msg_recv (void *input)
{
    assert (input);
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (0);
    zmsg_t *msg = NULL;
    //  Read message from input
    msg = zmsg_recv (input);
    if (!msg)
        return (NULL);

    //  If we're reading from a ROUTER socket, get routing_id
    if (zsocket_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zmsg_pop (msg);
        if (!self->routing_id)
            return (NULL);      //  Interrupted
        if (!zmsg_next (msg))
            return (NULL);      //  Malformed
    }

    //  Parse zmsg to zsync_ftm_msg
    self = zsync_ftm_msg_decode (msg);
    return self;
}

//  --------------------------------------------------------------------------
//  Receive and parse a zsync_ftm_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.

zsync_ftm_msg_t *
zsync_ftm_msg_recv_nowait (void *input)
{
    assert (input);
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (0);
    zmsg_t *msg = NULL;
    //  Read message from input
    msg = zmsg_recv_nowait (input);
    if (!msg)
        return (NULL);

    //  If we're reading from a ROUTER socket, get routing_id
    if (zsocket_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zmsg_pop (msg);
        if (!self->routing_id)
            return (NULL);      //  Interrupted
        if (!zmsg_next (msg))
            return (NULL);      //  Malformed
    }

    //  Parse zmsg to zsync_ftm_msg
    self = zsync_ftm_msg_decode (msg);
    return self;
}



//  --------------------------------------------------------------------------
//  Encode zsync_ftm_msg into zmsg and destroy it. Returns a newly created 
//  object or NULL if error. 

zmsg_t *
zsync_ftm_msg_encode (zsync_ftm_msg_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZSYNC_FTM_MSG_REQUEST:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  paths is an array of strings
            frame_size += 4;    //  Size is 4 octets
            if (self->paths) {
                //  Add up size of list contents
                char *paths = (char *) zlist_first (self->paths);
                while (paths) {
                    frame_size += 4 + strlen (paths);
                    paths = (char *) zlist_next (self->paths);
                }
            }
            break;
            
        case ZSYNC_FTM_MSG_CREDIT:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  credit is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_FTM_MSG_CHUNK:
            //  receiver is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->receiver)
                frame_size += strlen (self->receiver);
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            //  sequence is a 8-byte integer
            frame_size += 8;
            //  chunk_size is a 8-byte integer
            frame_size += 8;
            //  offset is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_FTM_MSG_ABORT:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            break;
            
        case ZSYNC_FTM_MSG_TERMINATE:
            break;
            
        default:
            printf ("E: bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_FTM_MSG_REQUEST:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->paths) {
                PUT_NUMBER4 (zlist_size (self->paths));
                char *paths = (char *) zlist_first (self->paths);
                while (paths) {
                    PUT_LONGSTR (paths);
                    paths = (char *) zlist_next (self->paths);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty string array
            break;

        case ZSYNC_FTM_MSG_CREDIT:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->credit);
            break;

        case ZSYNC_FTM_MSG_CHUNK:
            if (self->receiver) {
                PUT_STRING (self->receiver);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->path) {
                PUT_STRING (self->path);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->sequence);
            PUT_NUMBER8 (self->chunk_size);
            PUT_NUMBER8 (self->offset);
            break;

        case ZSYNC_FTM_MSG_ABORT:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->path) {
                PUT_STRING (self->path);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case ZSYNC_FTM_MSG_TERMINATE:
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        zsync_ftm_msg_destroy (&self);
        return NULL;
    }
    //  Destroy zsync_ftm_msg object
    zsync_ftm_msg_destroy (&self);
    return msg;

}

//  --------------------------------------------------------------------------
//  Send the zsync_ftm_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zsync_ftm_msg_send (zsync_ftm_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    zsync_ftm_msg_t *self = *self_p;
    zmsg_t *msg = NULL;
    //  Encode zsync_ftm_msg into zmsg.
    msg = zsync_ftm_msg_encode (self);

    //  If we're sending to a ROUTER, we send the routing_id first
    if (zsocket_type (output) == ZMQ_ROUTER) {
        assert (self->routing_id);
        if (zmsg_push (msg, self->routing_id)) {
            return -1;
        }
    }
    //  Now send the message
    if (zmsg_send (&msg, output)) {
        return -1;
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the zsync_ftm_msg to the output, and do not destroy it

int
zsync_ftm_msg_send_again (zsync_ftm_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = zsync_ftm_msg_dup (self);
    return zsync_ftm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REQUEST to the socket in one step

int
zsync_ftm_msg_send_request (
    void *output,
    char *sender,
    zlist_t *paths)
{
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_REQUEST);
    zsync_ftm_msg_set_sender (self, sender);
    zsync_ftm_msg_set_paths (self, zlist_dup (paths));
    return zsync_ftm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CREDIT to the socket in one step

int
zsync_ftm_msg_send_credit (
    void *output,
    char *sender,
    uint64_t credit)
{
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_CREDIT);
    zsync_ftm_msg_set_sender (self, sender);
    zsync_ftm_msg_set_credit (self, credit);
    return zsync_ftm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CHUNK to the socket in one step

int
zsync_ftm_msg_send_chunk (
    void *output,
    char *receiver,
    char *path,
    uint64_t sequence,
    uint64_t chunk_size,
    uint64_t offset)
{
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_CHUNK);
    zsync_ftm_msg_set_receiver (self, receiver);
    zsync_ftm_msg_set_path (self, path);
    zsync_ftm_msg_set_sequence (self, sequence);
    zsync_ftm_msg_set_chunk_size (self, chunk_size);
    zsync_ftm_msg_set_offset (self, offset);
    return zsync_ftm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ABORT to the socket in one step

int
zsync_ftm_msg_send_abort (
    void *output,
    char *sender,
    char *path)
{
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_ABORT);
    zsync_ftm_msg_set_sender (self, sender);
    zsync_ftm_msg_set_path (self, path);
    return zsync_ftm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the TERMINATE to the socket in one step

int
zsync_ftm_msg_send_terminate (
    void *output)
{
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_TERMINATE);
    return zsync_ftm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zsync_ftm_msg message

zsync_ftm_msg_t *
zsync_ftm_msg_dup (zsync_ftm_msg_t *self)
{
    if (!self)
        return NULL;
        
    zsync_ftm_msg_t *copy = zsync_ftm_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);

    switch (self->id) {
        case ZSYNC_FTM_MSG_REQUEST:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->paths = self->paths? zlist_dup (self->paths): NULL;
            break;

        case ZSYNC_FTM_MSG_CREDIT:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->credit = self->credit;
            break;

        case ZSYNC_FTM_MSG_CHUNK:
            copy->receiver = self->receiver? strdup (self->receiver): NULL;
            copy->path = self->path? strdup (self->path): NULL;
            copy->sequence = self->sequence;
            copy->chunk_size = self->chunk_size;
            copy->offset = self->offset;
            break;

        case ZSYNC_FTM_MSG_ABORT:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->path = self->path? strdup (self->path): NULL;
            break;

        case ZSYNC_FTM_MSG_TERMINATE:
            break;

    }
    return copy;
}



//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zsync_ftm_msg_dump (zsync_ftm_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_FTM_MSG_REQUEST:
            puts ("REQUEST:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            printf ("    paths={");
            if (self->paths) {
                char *paths = (char *) zlist_first (self->paths);
                while (paths) {
                    printf (" '%s'", paths);
                    paths = (char *) zlist_next (self->paths);
                }
            }
            printf (" }\n");
            break;
            
        case ZSYNC_FTM_MSG_CREDIT:
            puts ("CREDIT:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            printf ("    credit=%ld\n", (long) self->credit);
            break;
            
        case ZSYNC_FTM_MSG_CHUNK:
            puts ("CHUNK:");
            if (self->receiver)
                printf ("    receiver='%s'\n", self->receiver);
            else
                printf ("    receiver=\n");
            if (self->path)
                printf ("    path='%s'\n", self->path);
            else
                printf ("    path=\n");
            printf ("    sequence=%ld\n", (long) self->sequence);
            printf ("    chunk_size=%ld\n", (long) self->chunk_size);
            printf ("    offset=%ld\n", (long) self->offset);
            break;
            
        case ZSYNC_FTM_MSG_ABORT:
            puts ("ABORT:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            if (self->path)
                printf ("    path='%s'\n", self->path);
            else
                printf ("    path=\n");
            break;
            
        case ZSYNC_FTM_MSG_TERMINATE:
            puts ("TERMINATE:");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zsync_ftm_msg_routing_id (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zsync_ftm_msg_set_routing_id (zsync_ftm_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zsync_ftm_msg id

int
zsync_ftm_msg_id (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zsync_ftm_msg_set_id (zsync_ftm_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

char *
zsync_ftm_msg_command (zsync_ftm_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_FTM_MSG_REQUEST:
            return ("REQUEST");
            break;
        case ZSYNC_FTM_MSG_CREDIT:
            return ("CREDIT");
            break;
        case ZSYNC_FTM_MSG_CHUNK:
            return ("CHUNK");
            break;
        case ZSYNC_FTM_MSG_ABORT:
            return ("ABORT");
            break;
        case ZSYNC_FTM_MSG_TERMINATE:
            return ("TERMINATE");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the sender field

char *
zsync_ftm_msg_sender (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->sender;
}

void
zsync_ftm_msg_set_sender (zsync_ftm_msg_t *self, char *format, ...)
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
//  Get/set the paths field

zlist_t *
zsync_ftm_msg_paths (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->paths;
}

//  Greedy function, takes ownership of paths; if you don't want that
//  then use zlist_dup() to pass a copy of paths

void
zsync_ftm_msg_set_paths (zsync_ftm_msg_t *self, zlist_t *paths)
{
    assert (self);
    zlist_destroy (&self->paths);
    self->paths = paths;
}

//  --------------------------------------------------------------------------
//  Iterate through the paths field, and append a paths value

char *
zsync_ftm_msg_paths_first (zsync_ftm_msg_t *self)
{
    assert (self);
    if (self->paths)
        return (char *) (zlist_first (self->paths));
    else
        return NULL;
}

char *
zsync_ftm_msg_paths_next (zsync_ftm_msg_t *self)
{
    assert (self);
    if (self->paths)
        return (char *) (zlist_next (self->paths));
    else
        return NULL;
}

void
zsync_ftm_msg_paths_append (zsync_ftm_msg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Attach string to list
    if (!self->paths) {
        self->paths = zlist_new ();
        zlist_autofree (self->paths);
    }
    zlist_append (self->paths, string);
    free (string);
}

size_t
zsync_ftm_msg_paths_size (zsync_ftm_msg_t *self)
{
    return zlist_size (self->paths);
}


//  --------------------------------------------------------------------------
//  Get/set the credit field

uint64_t
zsync_ftm_msg_credit (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->credit;
}

void
zsync_ftm_msg_set_credit (zsync_ftm_msg_t *self, uint64_t credit)
{
    assert (self);
    self->credit = credit;
}


//  --------------------------------------------------------------------------
//  Get/set the receiver field

char *
zsync_ftm_msg_receiver (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->receiver;
}

void
zsync_ftm_msg_set_receiver (zsync_ftm_msg_t *self, char *format, ...)
{
    //  Format receiver from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->receiver);
    self->receiver = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the path field

char *
zsync_ftm_msg_path (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->path;
}

void
zsync_ftm_msg_set_path (zsync_ftm_msg_t *self, char *format, ...)
{
    //  Format path from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->path);
    self->path = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the sequence field

uint64_t
zsync_ftm_msg_sequence (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->sequence;
}

void
zsync_ftm_msg_set_sequence (zsync_ftm_msg_t *self, uint64_t sequence)
{
    assert (self);
    self->sequence = sequence;
}


//  --------------------------------------------------------------------------
//  Get/set the chunk_size field

uint64_t
zsync_ftm_msg_chunk_size (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->chunk_size;
}

void
zsync_ftm_msg_set_chunk_size (zsync_ftm_msg_t *self, uint64_t chunk_size)
{
    assert (self);
    self->chunk_size = chunk_size;
}


//  --------------------------------------------------------------------------
//  Get/set the offset field

uint64_t
zsync_ftm_msg_offset (zsync_ftm_msg_t *self)
{
    assert (self);
    return self->offset;
}

void
zsync_ftm_msg_set_offset (zsync_ftm_msg_t *self, uint64_t offset)
{
    assert (self);
    self->offset = offset;
}



//  --------------------------------------------------------------------------
//  Selftest

int
zsync_ftm_msg_test (bool verbose)
{
    printf (" * zsync_ftm_msg: ");

    //  @selftest
    //  Simple create/destroy test
    zsync_ftm_msg_t *self = zsync_ftm_msg_new (0);
    assert (self);
    zsync_ftm_msg_destroy (&self);

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
    int instance;
    zsync_ftm_msg_t *copy;
    self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_REQUEST);
    
    //  Check that _dup works on empty message
    copy = zsync_ftm_msg_dup (self);
    assert (copy);
    zsync_ftm_msg_destroy (&copy);

    zsync_ftm_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_ftm_msg_paths_append (self, "Name: %s", "Brutus");
    zsync_ftm_msg_paths_append (self, "Age: %d", 43);
    //  Send twice from same object
    zsync_ftm_msg_send_again (self, output);
    zsync_ftm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_ftm_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_ftm_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_ftm_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (zsync_ftm_msg_paths_size (self) == 2);
        assert (streq (zsync_ftm_msg_paths_first (self), "Name: Brutus"));
        assert (streq (zsync_ftm_msg_paths_next (self), "Age: 43"));
        zsync_ftm_msg_destroy (&self);
    }
    self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_CREDIT);
    
    //  Check that _dup works on empty message
    copy = zsync_ftm_msg_dup (self);
    assert (copy);
    zsync_ftm_msg_destroy (&copy);

    zsync_ftm_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_ftm_msg_set_credit (self, 123);
    //  Send twice from same object
    zsync_ftm_msg_send_again (self, output);
    zsync_ftm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_ftm_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_ftm_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_ftm_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (zsync_ftm_msg_credit (self) == 123);
        zsync_ftm_msg_destroy (&self);
    }
    self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_CHUNK);
    
    //  Check that _dup works on empty message
    copy = zsync_ftm_msg_dup (self);
    assert (copy);
    zsync_ftm_msg_destroy (&copy);

    zsync_ftm_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zsync_ftm_msg_set_path (self, "Life is short but Now lasts for ever");
    zsync_ftm_msg_set_sequence (self, 123);
    zsync_ftm_msg_set_chunk_size (self, 123);
    zsync_ftm_msg_set_offset (self, 123);
    //  Send twice from same object
    zsync_ftm_msg_send_again (self, output);
    zsync_ftm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_ftm_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_ftm_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_ftm_msg_receiver (self), "Life is short but Now lasts for ever"));
        assert (streq (zsync_ftm_msg_path (self), "Life is short but Now lasts for ever"));
        assert (zsync_ftm_msg_sequence (self) == 123);
        assert (zsync_ftm_msg_chunk_size (self) == 123);
        assert (zsync_ftm_msg_offset (self) == 123);
        zsync_ftm_msg_destroy (&self);
    }
    self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_ABORT);
    
    //  Check that _dup works on empty message
    copy = zsync_ftm_msg_dup (self);
    assert (copy);
    zsync_ftm_msg_destroy (&copy);

    zsync_ftm_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_ftm_msg_set_path (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    zsync_ftm_msg_send_again (self, output);
    zsync_ftm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_ftm_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_ftm_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_ftm_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (zsync_ftm_msg_path (self), "Life is short but Now lasts for ever"));
        zsync_ftm_msg_destroy (&self);
    }
    self = zsync_ftm_msg_new (ZSYNC_FTM_MSG_TERMINATE);
    
    //  Check that _dup works on empty message
    copy = zsync_ftm_msg_dup (self);
    assert (copy);
    zsync_ftm_msg_destroy (&copy);

    //  Send twice from same object
    zsync_ftm_msg_send_again (self, output);
    zsync_ftm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_ftm_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_ftm_msg_recv_nowait (input);
        }
        assert (self);
        
        zsync_ftm_msg_destroy (&self);
    }

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
