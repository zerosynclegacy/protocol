/*  =========================================================================
    zsync_msg - the API which is used to comminicate with user interface clients
    
    Codec header for zsync_msg.

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

#ifndef __ZSYNC_MSG_H_INCLUDED__
#define __ZSYNC_MSG_H_INCLUDED__

/*  These are the zsync_msg messages:

    REQ_STATE - Requests the current state.

    RES_STATE - Responds to REQ_STATE with current state.
        state               number 8    

    REQ_UPDATE - Requests an update for all changes with a newer state then 'state'.
        state               number 8    

    UPDATE - Sends a list of updated files to the client.
        sender              string      UUID that identifies the sender
        update_msg          msg         List of updated files and their metadata

    REQ_FILES - Requests a list of files from receiver.
        receiver            string      UUID that identifies the receiver
        files               strings     List of file names
        size                number 8    Total size of all files in bytes

    REQ_CHUNK - Requests a chunk of 'chunk_size' data from 'path' at 'offset'.
        path                string      Path of file that the 'chunk' belongs to 
        chunk_size          number 8    Size of the requested chunk in bytes
        offset              number 8    File offset for for the chunk in bytes

    RES_CHUNK - Responds with the requested chunk.
        chunk               chunk       Requested chunk

    CHUNK - Sends one 'chunk' of data of a file at the 'path'.
        chunk               chunk       This chunk is part of the file at 'path'
        path                string      Path of file that the 'chunk' belongs to 
        sequence            number 8    Defines which chunk of the file at 'path' this is!
        offset              number 8    Offset for this 'chunk' in bytes

    ABORT - Sends an abort for one file at path.
        receiver            string      UUID that identifies the receiver
        path                string      

    TERMINATE - Terminate all worker threads.
*/

#define ZSYNC_MSG_VERSION                   1

#define ZSYNC_MSG_REQ_STATE                 1
#define ZSYNC_MSG_RES_STATE                 2
#define ZSYNC_MSG_REQ_UPDATE                3
#define ZSYNC_MSG_UPDATE                    4
#define ZSYNC_MSG_REQ_FILES                 5
#define ZSYNC_MSG_REQ_CHUNK                 6
#define ZSYNC_MSG_RES_CHUNK                 7
#define ZSYNC_MSG_CHUNK                     8
#define ZSYNC_MSG_ABORT                     9
#define ZSYNC_MSG_TERMINATE                 10

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zsync_msg_t zsync_msg_t;

//  @interface
//  Create a new zsync_msg
zsync_msg_t *
    zsync_msg_new (int id);

//  Destroy the zsync_msg
void
    zsync_msg_destroy (zsync_msg_t **self_p);

//  Parse a zsync_msg from zmsg_t. Returns new object or NULL if error.
//  Use when not in control of receiving the message.
zsync_msg_t *
    zsync_msg_decode (zmsg_t *msg);

//  Encode zsync_msg into zmsg and destroy it. 
//  Returns a newly created object or NULL if error. 
//  Use when not in control of sending the message.
zmsg_t *
    zsync_msg_encode (zsync_msg_t *self);

//  Receive and parse a zsync_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
zsync_msg_t *
    zsync_msg_recv (void *input);

//  Receive and parse a zsync_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
zsync_msg_t *
    zsync_msg_recv_nowait (void *input);

//  Send the zsync_msg to the output, and destroy it
int
    zsync_msg_send (zsync_msg_t **self_p, void *output);

//  Send the zsync_msg to the output, and do not destroy it
int
    zsync_msg_send_again (zsync_msg_t *self, void *output);

//  Send the REQ_STATE to the output in one step
int
    zsync_msg_send_req_state (void *output);
    
//  Send the RES_STATE to the output in one step
int
    zsync_msg_send_res_state (void *output,
        uint64_t state);
    
//  Send the REQ_UPDATE to the output in one step
int
    zsync_msg_send_req_update (void *output,
        uint64_t state);
    
//  Send the UPDATE to the output in one step
int
    zsync_msg_send_update (void *output,
        char *sender,
        zmsg_t *update_msg);
    
//  Send the REQ_FILES to the output in one step
int
    zsync_msg_send_req_files (void *output,
        char *receiver,
        zlist_t *files,
        uint64_t size);
    
//  Send the REQ_CHUNK to the output in one step
int
    zsync_msg_send_req_chunk (void *output,
        char *path,
        uint64_t chunk_size,
        uint64_t offset);
    
//  Send the RES_CHUNK to the output in one step
int
    zsync_msg_send_res_chunk (void *output,
        zchunk_t *chunk);
    
//  Send the CHUNK to the output in one step
int
    zsync_msg_send_chunk (void *output,
        zchunk_t *chunk,
        char *path,
        uint64_t sequence,
        uint64_t offset);
    
//  Send the ABORT to the output in one step
int
    zsync_msg_send_abort (void *output,
        char *receiver,
        char *path);
    
//  Send the TERMINATE to the output in one step
int
    zsync_msg_send_terminate (void *output);
    
//  Duplicate the zsync_msg message
zsync_msg_t *
    zsync_msg_dup (zsync_msg_t *self);

//  Print contents of message to stdout
void
    zsync_msg_dump (zsync_msg_t *self);

//  Get/set the message routing id
zframe_t *
    zsync_msg_routing_id (zsync_msg_t *self);
void
    zsync_msg_set_routing_id (zsync_msg_t *self, zframe_t *routing_id);

//  Get the zsync_msg id and printable command
int
    zsync_msg_id (zsync_msg_t *self);
void
    zsync_msg_set_id (zsync_msg_t *self, int id);
char *
    zsync_msg_command (zsync_msg_t *self);

//  Get/set the state field
uint64_t
    zsync_msg_state (zsync_msg_t *self);
void
    zsync_msg_set_state (zsync_msg_t *self, uint64_t state);

//  Get/set the sender field
char *
    zsync_msg_sender (zsync_msg_t *self);
void
    zsync_msg_set_sender (zsync_msg_t *self, char *format, ...);

//  Get/set the update_msg field
zmsg_t *
    zsync_msg_update_msg (zsync_msg_t *self);
void
    zsync_msg_set_update_msg (zsync_msg_t *self, zmsg_t *msg);

//  Get/set the receiver field
char *
    zsync_msg_receiver (zsync_msg_t *self);
void
    zsync_msg_set_receiver (zsync_msg_t *self, char *format, ...);

//  Get/set the files field
zlist_t *
    zsync_msg_files (zsync_msg_t *self);
void
    zsync_msg_set_files (zsync_msg_t *self, zlist_t *files);

//  Iterate through the files field, and append a files value
char *
    zsync_msg_files_first (zsync_msg_t *self);
char *
    zsync_msg_files_next (zsync_msg_t *self);
void
    zsync_msg_files_append (zsync_msg_t *self, char *format, ...);
size_t
    zsync_msg_files_size (zsync_msg_t *self);

//  Get/set the size field
uint64_t
    zsync_msg_size (zsync_msg_t *self);
void
    zsync_msg_set_size (zsync_msg_t *self, uint64_t size);

//  Get/set the path field
char *
    zsync_msg_path (zsync_msg_t *self);
void
    zsync_msg_set_path (zsync_msg_t *self, char *format, ...);

//  Get/set the chunk_size field
uint64_t
    zsync_msg_chunk_size (zsync_msg_t *self);
void
    zsync_msg_set_chunk_size (zsync_msg_t *self, uint64_t chunk_size);

//  Get/set the offset field
uint64_t
    zsync_msg_offset (zsync_msg_t *self);
void
    zsync_msg_set_offset (zsync_msg_t *self, uint64_t offset);

//  Get/set the chunk field
zchunk_t *
    zsync_msg_chunk (zsync_msg_t *self);
void
    zsync_msg_set_chunk (zsync_msg_t *self, zchunk_t *chunk);

//  Get/set the sequence field
uint64_t
    zsync_msg_sequence (zsync_msg_t *self);
void
    zsync_msg_set_sequence (zsync_msg_t *self, uint64_t sequence);

//  Self test of this class
int
    zsync_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
