/*  =========================================================================
    zsync_msg - the API which is used to comminicate with user interface clients

    Codec class for zsync_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

    * The XML model used for this code generation: zsync_msg
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
    zsync_msg - the API which is used to comminicate with user interface clients
@discuss
@end
*/

#include <czmq.h>
#include "../include/zsync_msg.h"

//  Structure of our class

struct _zsync_msg_t {
    zframe_t *routing_id;       //  Routing_id from ROUTER, if any
    int id;                     //  zsync_msg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    uint64_t state;             //  
    char *sender;               //  UUID that identifies the sender
    zmsg_t *update_msg;         //  List of updated files and their metadata
    char *receiver;             //  UUID that identifies the receiver
    zlist_t *files;             //  List of file names
    uint64_t size;              //  Total size of all files in bytes
    char *path;                 //  Path of file that the 'chunk' belongs to 
    uint64_t chunk_size;        //  Size of the requested chunk in bytes
    uint64_t offset;            //  File offset for for the chunk in bytes
    zchunk_t *chunk;            //  Requested chunk
    uint64_t sequence;          //  Defines which chunk of the file at 'path' this is!
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
//  Create a new zsync_msg

zsync_msg_t *
zsync_msg_new (int id)
{
    zsync_msg_t *self = (zsync_msg_t *) zmalloc (sizeof (zsync_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zsync_msg

void
zsync_msg_destroy (zsync_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->sender);
        zmsg_destroy (&self->update_msg);
        free (self->receiver);
        if (self->files)
            zlist_destroy (&self->files);
        free (self->path);
        zchunk_destroy (&self->chunk);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Parse a zsync_msg from zmsg_t. Returns new object or NULL if error.

zsync_msg_t *
zsync_msg_decode (zmsg_t *msg)
{
    zsync_msg_t *self = zsync_msg_new (0);
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
    if (signature != (0xAAA0 | 0))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            break;

        case ZSYNC_MSG_RES_STATE:
            GET_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_REQ_UPDATE:
            GET_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_UPDATE:
            GET_STRING (self->sender);
            //  Get zero or more remaining frames,
            //  leave current frame untouched
            self->update_msg = zmsg_new ();
            zframe_t *update_msg_part = zmsg_pop (msg);
            while (update_msg_part) {
                zmsg_add (self->update_msg, update_msg_part);
                update_msg_part = zmsg_pop (msg);
            }
            break;

        case ZSYNC_MSG_REQ_FILES:
            GET_STRING (self->receiver);
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                self->files = zlist_new ();
                zlist_autofree (self->files);
                while (list_size--) {
                    char *string;
                    GET_LONGSTR (string);
                    zlist_append (self->files, string);
                    free (string);
                }
            }
            GET_NUMBER8 (self->size);
            break;

        case ZSYNC_MSG_REQ_CHUNK:
            GET_STRING (self->path);
            GET_NUMBER8 (self->chunk_size);
            GET_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_RES_CHUNK:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->chunk = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case ZSYNC_MSG_CHUNK:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                    goto malformed;
                self->chunk = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            GET_STRING (self->path);
            GET_NUMBER8 (self->sequence);
            GET_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_ABORT:
            GET_STRING (self->receiver);
            GET_STRING (self->path);
            break;

        case ZSYNC_MSG_TERMINATE:
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
        zsync_msg_destroy (&self);
        return (NULL);
}

//  --------------------------------------------------------------------------
//  Receive and parse a zsync_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zsync_msg_t *
zsync_msg_recv (void *input)
{
    assert (input);
    zsync_msg_t *self = zsync_msg_new (0);
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

    //  Parse zmsg to zsync_msg
    self = zsync_msg_decode (msg);
    return self;
}

//  --------------------------------------------------------------------------
//  Receive and parse a zsync_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.

zsync_msg_t *
zsync_msg_recv_nowait (void *input)
{
    assert (input);
    zsync_msg_t *self = zsync_msg_new (0);
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

    //  Parse zmsg to zsync_msg
    self = zsync_msg_decode (msg);
    return self;
}



//  --------------------------------------------------------------------------
//  Encode zsync_msg into zmsg and destroy it. Returns a newly created 
//  object or NULL if error. 

zmsg_t *
zsync_msg_encode (zsync_msg_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            break;
            
        case ZSYNC_MSG_RES_STATE:
            //  state is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_REQ_UPDATE:
            //  state is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_UPDATE:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            break;
            
        case ZSYNC_MSG_REQ_FILES:
            //  receiver is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->receiver)
                frame_size += strlen (self->receiver);
            //  files is an array of strings
            frame_size += 4;    //  Size is 4 octets
            if (self->files) {
                //  Add up size of list contents
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    frame_size += 4 + strlen (files);
                    files = (char *) zlist_next (self->files);
                }
            }
            //  size is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_REQ_CHUNK:
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            //  chunk_size is a 8-byte integer
            frame_size += 8;
            //  offset is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_RES_CHUNK:
            //  chunk is a chunk with 4-byte length
            frame_size += 4;
            if (self->chunk)
                frame_size += zchunk_size (self->chunk);
            break;
            
        case ZSYNC_MSG_CHUNK:
            //  chunk is a chunk with 4-byte length
            frame_size += 4;
            if (self->chunk)
                frame_size += zchunk_size (self->chunk);
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            //  sequence is a 8-byte integer
            frame_size += 8;
            //  offset is a 8-byte integer
            frame_size += 8;
            break;
            
        case ZSYNC_MSG_ABORT:
            //  receiver is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->receiver)
                frame_size += strlen (self->receiver);
            //  path is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->path)
                frame_size += strlen (self->path);
            break;
            
        case ZSYNC_MSG_TERMINATE:
            break;
            
        default:
            printf ("E: bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            break;

        case ZSYNC_MSG_RES_STATE:
            PUT_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_REQ_UPDATE:
            PUT_NUMBER8 (self->state);
            break;

        case ZSYNC_MSG_UPDATE:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case ZSYNC_MSG_REQ_FILES:
            if (self->receiver) {
                PUT_STRING (self->receiver);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->files) {
                PUT_NUMBER4 (zlist_size (self->files));
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    PUT_LONGSTR (files);
                    files = (char *) zlist_next (self->files);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty string array
            PUT_NUMBER8 (self->size);
            break;

        case ZSYNC_MSG_REQ_CHUNK:
            if (self->path) {
                PUT_STRING (self->path);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->chunk_size);
            PUT_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_RES_CHUNK:
            PUT_NUMBER4 (zchunk_size (self->chunk));
            if (self->chunk) {
                memcpy (self->needle,
                        zchunk_data (self->chunk),
                        zchunk_size (self->chunk));
                self->needle += zchunk_size (self->chunk);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case ZSYNC_MSG_CHUNK:
            PUT_NUMBER4 (zchunk_size (self->chunk));
            if (self->chunk) {
                memcpy (self->needle,
                        zchunk_data (self->chunk),
                        zchunk_size (self->chunk));
                self->needle += zchunk_size (self->chunk);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            if (self->path) {
                PUT_STRING (self->path);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER8 (self->sequence);
            PUT_NUMBER8 (self->offset);
            break;

        case ZSYNC_MSG_ABORT:
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
            break;

        case ZSYNC_MSG_TERMINATE:
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        zsync_msg_destroy (&self);
        return NULL;
    }
    //  Now send the update_msg field if set
    if (self->id == ZSYNC_MSG_UPDATE) {
        zframe_t *update_msg_part = zmsg_pop (self->update_msg);
        while (update_msg_part) {
            zmsg_append (msg, &update_msg_part);
            update_msg_part = zmsg_pop (self->update_msg);
        }
    }
    //  Destroy zsync_msg object
    zsync_msg_destroy (&self);
    return msg;

}

//  --------------------------------------------------------------------------
//  Send the zsync_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zsync_msg_send (zsync_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    zsync_msg_t *self = *self_p;
    zmsg_t *msg = NULL;
    //  Encode zsync_msg into zmsg.
    msg = zsync_msg_encode (self);

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
//  Send the zsync_msg to the output, and do not destroy it

int
zsync_msg_send_again (zsync_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = zsync_msg_dup (self);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REQ_STATE to the socket in one step

int
zsync_msg_send_req_state (
    void *output)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_REQ_STATE);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RES_STATE to the socket in one step

int
zsync_msg_send_res_state (
    void *output,
    uint64_t state)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_RES_STATE);
    zsync_msg_set_state (self, state);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REQ_UPDATE to the socket in one step

int
zsync_msg_send_req_update (
    void *output,
    uint64_t state)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_REQ_UPDATE);
    zsync_msg_set_state (self, state);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the UPDATE to the socket in one step

int
zsync_msg_send_update (
    void *output,
    char *sender,
    zmsg_t *update_msg)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_UPDATE);
    zsync_msg_set_sender (self, sender);
    zsync_msg_set_update_msg (self, zmsg_dup (update_msg));
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REQ_FILES to the socket in one step

int
zsync_msg_send_req_files (
    void *output,
    char *receiver,
    zlist_t *files,
    uint64_t size)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_REQ_FILES);
    zsync_msg_set_receiver (self, receiver);
    zsync_msg_set_files (self, zlist_dup (files));
    zsync_msg_set_size (self, size);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REQ_CHUNK to the socket in one step

int
zsync_msg_send_req_chunk (
    void *output,
    char *path,
    uint64_t chunk_size,
    uint64_t offset)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_REQ_CHUNK);
    zsync_msg_set_path (self, path);
    zsync_msg_set_chunk_size (self, chunk_size);
    zsync_msg_set_offset (self, offset);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the RES_CHUNK to the socket in one step

int
zsync_msg_send_res_chunk (
    void *output,
    zchunk_t *chunk)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_RES_CHUNK);
    zsync_msg_set_chunk (self, zchunk_dup (chunk));
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CHUNK to the socket in one step

int
zsync_msg_send_chunk (
    void *output,
    zchunk_t *chunk,
    char *path,
    uint64_t sequence,
    uint64_t offset)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_CHUNK);
    zsync_msg_set_chunk (self, zchunk_dup (chunk));
    zsync_msg_set_path (self, path);
    zsync_msg_set_sequence (self, sequence);
    zsync_msg_set_offset (self, offset);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ABORT to the socket in one step

int
zsync_msg_send_abort (
    void *output,
    char *receiver,
    char *path)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_ABORT);
    zsync_msg_set_receiver (self, receiver);
    zsync_msg_set_path (self, path);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the TERMINATE to the socket in one step

int
zsync_msg_send_terminate (
    void *output)
{
    zsync_msg_t *self = zsync_msg_new (ZSYNC_MSG_TERMINATE);
    return zsync_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zsync_msg message

zsync_msg_t *
zsync_msg_dup (zsync_msg_t *self)
{
    if (!self)
        return NULL;
        
    zsync_msg_t *copy = zsync_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);

    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            break;

        case ZSYNC_MSG_RES_STATE:
            copy->state = self->state;
            break;

        case ZSYNC_MSG_REQ_UPDATE:
            copy->state = self->state;
            break;

        case ZSYNC_MSG_UPDATE:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->update_msg = self->update_msg? zmsg_dup (self->update_msg): NULL;
            break;

        case ZSYNC_MSG_REQ_FILES:
            copy->receiver = self->receiver? strdup (self->receiver): NULL;
            copy->files = self->files? zlist_dup (self->files): NULL;
            copy->size = self->size;
            break;

        case ZSYNC_MSG_REQ_CHUNK:
            copy->path = self->path? strdup (self->path): NULL;
            copy->chunk_size = self->chunk_size;
            copy->offset = self->offset;
            break;

        case ZSYNC_MSG_RES_CHUNK:
            copy->chunk = self->chunk? zchunk_dup (self->chunk): NULL;
            break;

        case ZSYNC_MSG_CHUNK:
            copy->chunk = self->chunk? zchunk_dup (self->chunk): NULL;
            copy->path = self->path? strdup (self->path): NULL;
            copy->sequence = self->sequence;
            copy->offset = self->offset;
            break;

        case ZSYNC_MSG_ABORT:
            copy->receiver = self->receiver? strdup (self->receiver): NULL;
            copy->path = self->path? strdup (self->path): NULL;
            break;

        case ZSYNC_MSG_TERMINATE:
            break;

    }
    return copy;
}



//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zsync_msg_dump (zsync_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            puts ("REQ_STATE:");
            break;
            
        case ZSYNC_MSG_RES_STATE:
            puts ("RES_STATE:");
            printf ("    state=%ld\n", (long) self->state);
            break;
            
        case ZSYNC_MSG_REQ_UPDATE:
            puts ("REQ_UPDATE:");
            printf ("    state=%ld\n", (long) self->state);
            break;
            
        case ZSYNC_MSG_UPDATE:
            puts ("UPDATE:");
            if (self->sender)
                printf ("    sender='%s'\n", self->sender);
            else
                printf ("    sender=\n");
            printf ("    update_msg={\n");
            if (self->update_msg)
                zmsg_dump (self->update_msg);
            else
                printf ("(NULL)\n");
            printf ("    }\n");
            break;
            
        case ZSYNC_MSG_REQ_FILES:
            puts ("REQ_FILES:");
            if (self->receiver)
                printf ("    receiver='%s'\n", self->receiver);
            else
                printf ("    receiver=\n");
            printf ("    files={");
            if (self->files) {
                char *files = (char *) zlist_first (self->files);
                while (files) {
                    printf (" '%s'", files);
                    files = (char *) zlist_next (self->files);
                }
            }
            printf (" }\n");
            printf ("    size=%ld\n", (long) self->size);
            break;
            
        case ZSYNC_MSG_REQ_CHUNK:
            puts ("REQ_CHUNK:");
            if (self->path)
                printf ("    path='%s'\n", self->path);
            else
                printf ("    path=\n");
            printf ("    chunk_size=%ld\n", (long) self->chunk_size);
            printf ("    offset=%ld\n", (long) self->offset);
            break;
            
        case ZSYNC_MSG_RES_CHUNK:
            puts ("RES_CHUNK:");
            printf ("    chunk={\n");
            if (self->chunk)
                zchunk_print (self->chunk);
            else
                printf ("(NULL)\n");
            printf ("    }\n");
            break;
            
        case ZSYNC_MSG_CHUNK:
            puts ("CHUNK:");
            printf ("    chunk={\n");
            if (self->chunk)
                zchunk_print (self->chunk);
            else
                printf ("(NULL)\n");
            printf ("    }\n");
            if (self->path)
                printf ("    path='%s'\n", self->path);
            else
                printf ("    path=\n");
            printf ("    sequence=%ld\n", (long) self->sequence);
            printf ("    offset=%ld\n", (long) self->offset);
            break;
            
        case ZSYNC_MSG_ABORT:
            puts ("ABORT:");
            if (self->receiver)
                printf ("    receiver='%s'\n", self->receiver);
            else
                printf ("    receiver=\n");
            if (self->path)
                printf ("    path='%s'\n", self->path);
            else
                printf ("    path=\n");
            break;
            
        case ZSYNC_MSG_TERMINATE:
            puts ("TERMINATE:");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zsync_msg_routing_id (zsync_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zsync_msg_set_routing_id (zsync_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zsync_msg id

int
zsync_msg_id (zsync_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zsync_msg_set_id (zsync_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

char *
zsync_msg_command (zsync_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZSYNC_MSG_REQ_STATE:
            return ("REQ_STATE");
            break;
        case ZSYNC_MSG_RES_STATE:
            return ("RES_STATE");
            break;
        case ZSYNC_MSG_REQ_UPDATE:
            return ("REQ_UPDATE");
            break;
        case ZSYNC_MSG_UPDATE:
            return ("UPDATE");
            break;
        case ZSYNC_MSG_REQ_FILES:
            return ("REQ_FILES");
            break;
        case ZSYNC_MSG_REQ_CHUNK:
            return ("REQ_CHUNK");
            break;
        case ZSYNC_MSG_RES_CHUNK:
            return ("RES_CHUNK");
            break;
        case ZSYNC_MSG_CHUNK:
            return ("CHUNK");
            break;
        case ZSYNC_MSG_ABORT:
            return ("ABORT");
            break;
        case ZSYNC_MSG_TERMINATE:
            return ("TERMINATE");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the state field

uint64_t
zsync_msg_state (zsync_msg_t *self)
{
    assert (self);
    return self->state;
}

void
zsync_msg_set_state (zsync_msg_t *self, uint64_t state)
{
    assert (self);
    self->state = state;
}


//  --------------------------------------------------------------------------
//  Get/set the sender field

char *
zsync_msg_sender (zsync_msg_t *self)
{
    assert (self);
    return self->sender;
}

void
zsync_msg_set_sender (zsync_msg_t *self, char *format, ...)
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
//  Get/set the update_msg field

zmsg_t *
zsync_msg_update_msg (zsync_msg_t *self)
{
    assert (self);
    return self->update_msg;
}

//  Takes ownership of supplied msg
void
zsync_msg_set_update_msg (zsync_msg_t *self, zmsg_t *msg)
{
    assert (self);
    if (self->update_msg)
        zmsg_destroy (&self->update_msg);
    self->update_msg = msg;
}

//  --------------------------------------------------------------------------
//  Get/set the receiver field

char *
zsync_msg_receiver (zsync_msg_t *self)
{
    assert (self);
    return self->receiver;
}

void
zsync_msg_set_receiver (zsync_msg_t *self, char *format, ...)
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
//  Get/set the files field

zlist_t *
zsync_msg_files (zsync_msg_t *self)
{
    assert (self);
    return self->files;
}

//  Greedy function, takes ownership of files; if you don't want that
//  then use zlist_dup() to pass a copy of files

void
zsync_msg_set_files (zsync_msg_t *self, zlist_t *files)
{
    assert (self);
    zlist_destroy (&self->files);
    self->files = files;
}

//  --------------------------------------------------------------------------
//  Iterate through the files field, and append a files value

char *
zsync_msg_files_first (zsync_msg_t *self)
{
    assert (self);
    if (self->files)
        return (char *) (zlist_first (self->files));
    else
        return NULL;
}

char *
zsync_msg_files_next (zsync_msg_t *self)
{
    assert (self);
    if (self->files)
        return (char *) (zlist_next (self->files));
    else
        return NULL;
}

void
zsync_msg_files_append (zsync_msg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Attach string to list
    if (!self->files) {
        self->files = zlist_new ();
        zlist_autofree (self->files);
    }
    zlist_append (self->files, string);
    free (string);
}

size_t
zsync_msg_files_size (zsync_msg_t *self)
{
    return zlist_size (self->files);
}


//  --------------------------------------------------------------------------
//  Get/set the size field

uint64_t
zsync_msg_size (zsync_msg_t *self)
{
    assert (self);
    return self->size;
}

void
zsync_msg_set_size (zsync_msg_t *self, uint64_t size)
{
    assert (self);
    self->size = size;
}


//  --------------------------------------------------------------------------
//  Get/set the path field

char *
zsync_msg_path (zsync_msg_t *self)
{
    assert (self);
    return self->path;
}

void
zsync_msg_set_path (zsync_msg_t *self, char *format, ...)
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
//  Get/set the chunk_size field

uint64_t
zsync_msg_chunk_size (zsync_msg_t *self)
{
    assert (self);
    return self->chunk_size;
}

void
zsync_msg_set_chunk_size (zsync_msg_t *self, uint64_t chunk_size)
{
    assert (self);
    self->chunk_size = chunk_size;
}


//  --------------------------------------------------------------------------
//  Get/set the offset field

uint64_t
zsync_msg_offset (zsync_msg_t *self)
{
    assert (self);
    return self->offset;
}

void
zsync_msg_set_offset (zsync_msg_t *self, uint64_t offset)
{
    assert (self);
    self->offset = offset;
}


//  --------------------------------------------------------------------------
//  Get/set the chunk field

zchunk_t *
zsync_msg_chunk (zsync_msg_t *self)
{
    assert (self);
    return self->chunk;
}

//  Takes ownership of supplied chunk
void
zsync_msg_set_chunk (zsync_msg_t *self, zchunk_t *chunk)
{
    assert (self);
    if (self->chunk)
        zchunk_destroy (&self->chunk);
    self->chunk = chunk;
}

//  --------------------------------------------------------------------------
//  Get/set the sequence field

uint64_t
zsync_msg_sequence (zsync_msg_t *self)
{
    assert (self);
    return self->sequence;
}

void
zsync_msg_set_sequence (zsync_msg_t *self, uint64_t sequence)
{
    assert (self);
    self->sequence = sequence;
}



//  --------------------------------------------------------------------------
//  Selftest

int
zsync_msg_test (bool verbose)
{
    printf (" * zsync_msg: ");

    //  @selftest
    //  Simple create/destroy test
    zsync_msg_t *self = zsync_msg_new (0);
    assert (self);
    zsync_msg_destroy (&self);

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
    zsync_msg_t *copy;
    self = zsync_msg_new (ZSYNC_MSG_REQ_STATE);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_RES_STATE);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_state (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (zsync_msg_state (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_REQ_UPDATE);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_state (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (zsync_msg_state (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_UPDATE);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_sender (self, "Life is short but Now lasts for ever");
    zsync_msg_set_update_msg (self, zmsg_new ());
//    zmsg_addstr (zsync_msg_update_msg (self), "Hello, World");
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (zsync_msg_update_msg (self)) == 0);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_REQ_FILES);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zsync_msg_files_append (self, "Name: %s", "Brutus");
    zsync_msg_files_append (self, "Age: %d", 43);
    zsync_msg_set_size (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_msg_receiver (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_files_size (self) == 2);
        assert (streq (zsync_msg_files_first (self), "Name: Brutus"));
        assert (streq (zsync_msg_files_next (self), "Age: 43"));
        assert (zsync_msg_size (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_REQ_CHUNK);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    zsync_msg_set_chunk_size (self, 123);
    zsync_msg_set_offset (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_chunk_size (self) == 123);
        assert (zsync_msg_offset (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_RES_CHUNK);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_chunk (self, zchunk_new ("Captcha Diem", 12));
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (memcmp (zchunk_data (zsync_msg_chunk (self)), "Captcha Diem", 12) == 0);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_CHUNK);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_chunk (self, zchunk_new ("Captcha Diem", 12));
    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    zsync_msg_set_sequence (self, 123);
    zsync_msg_set_offset (self, 123);
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (memcmp (zchunk_data (zsync_msg_chunk (self)), "Captcha Diem", 12) == 0);
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        assert (zsync_msg_sequence (self) == 123);
        assert (zsync_msg_offset (self) == 123);
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_ABORT);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    zsync_msg_set_receiver (self, "Life is short but Now lasts for ever");
    zsync_msg_set_path (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        assert (streq (zsync_msg_receiver (self), "Life is short but Now lasts for ever"));
        assert (streq (zsync_msg_path (self), "Life is short but Now lasts for ever"));
        zsync_msg_destroy (&self);
    }
    self = zsync_msg_new (ZSYNC_MSG_TERMINATE);
    
    //  Check that _dup works on empty message
    copy = zsync_msg_dup (self);
    assert (copy);
    zsync_msg_destroy (&copy);

    //  Send twice from same object
    zsync_msg_send_again (self, output);
    zsync_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        if (instance == 0) {
            self = zsync_msg_recv (input);
        }
        if (instance == 1) {
            zclock_sleep (250);     // give time for message to arrive
            self = zsync_msg_recv_nowait (input);
        }
        assert (self);
        
        zsync_msg_destroy (&self);
    }

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
