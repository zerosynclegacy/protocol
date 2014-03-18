/*  =========================================================================
    zsync_ftm_msg - file transfer manager api
    
    Codec header for zsync_ftm_msg.

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

#ifndef __ZSYNC_FTM_MSG_H_INCLUDED__
#define __ZSYNC_FTM_MSG_H_INCLUDED__

/*  These are the zsync_ftm_msg messages:

    REQUEST - Sends a list of files requested by sender
        sender              string      UUID that identifies the sender
        paths               strings     

    CREDIT - Sends an approved credit amount by sender
        sender              string      UUID that identifies the sender
        credit              number 8    

    CHUNK - Requests a chunk of 'chunk_size' data from 'path' at 'offset'.
        receiver            string      UUID that identifies the receiver
        path                string      Path of file that the 'chunk' belongs to
        sequence            number 8    
        chunk_size          number 8    Size of the requested chunk in bytes
        offset              number 8    File offset for for the chunk in bytes

    ABORT - Sends a list of files to abort file transfer for sender
        sender              string      UUID that identifies the sender
        path                string      

    TERMINATE - Terminate the worker thread
*/

#define ZSYNC_FTM_MSG_VERSION               1

#define ZSYNC_FTM_MSG_REQUEST               1
#define ZSYNC_FTM_MSG_CREDIT                2
#define ZSYNC_FTM_MSG_CHUNK                 3
#define ZSYNC_FTM_MSG_ABORT                 4
#define ZSYNC_FTM_MSG_TERMINATE             5

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zsync_ftm_msg_t zsync_ftm_msg_t;

//  @interface
//  Create a new zsync_ftm_msg
zsync_ftm_msg_t *
    zsync_ftm_msg_new (int id);

//  Destroy the zsync_ftm_msg
void
    zsync_ftm_msg_destroy (zsync_ftm_msg_t **self_p);

//  Parse a zsync_ftm_msg from zmsg_t. Returns new object or NULL if error.
//  Use when not in control of receiving the message.
zsync_ftm_msg_t *
    zsync_ftm_msg_decode (zmsg_t *msg);

//  Encode zsync_ftm_msg into zmsg and destroy it. 
//  Returns a newly created object or NULL if error. 
//  Use when not in control of sending the message.
zmsg_t *
    zsync_ftm_msg_encode (zsync_ftm_msg_t *self);

//  Receive and parse a zsync_ftm_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
zsync_ftm_msg_t *
    zsync_ftm_msg_recv (void *input);

//  Receive and parse a zsync_ftm_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
zsync_ftm_msg_t *
    zsync_ftm_msg_recv_nowait (void *input);

//  Send the zsync_ftm_msg to the output, and destroy it
int
    zsync_ftm_msg_send (zsync_ftm_msg_t **self_p, void *output);

//  Send the zsync_ftm_msg to the output, and do not destroy it
int
    zsync_ftm_msg_send_again (zsync_ftm_msg_t *self, void *output);

//  Send the REQUEST to the output in one step
int
    zsync_ftm_msg_send_request (void *output,
        char *sender,
        zlist_t *paths);
    
//  Send the CREDIT to the output in one step
int
    zsync_ftm_msg_send_credit (void *output,
        char *sender,
        uint64_t credit);
    
//  Send the CHUNK to the output in one step
int
    zsync_ftm_msg_send_chunk (void *output,
        char *receiver,
        char *path,
        uint64_t sequence,
        uint64_t chunk_size,
        uint64_t offset);
    
//  Send the ABORT to the output in one step
int
    zsync_ftm_msg_send_abort (void *output,
        char *sender,
        char *path);
    
//  Send the TERMINATE to the output in one step
int
    zsync_ftm_msg_send_terminate (void *output);
    
//  Duplicate the zsync_ftm_msg message
zsync_ftm_msg_t *
    zsync_ftm_msg_dup (zsync_ftm_msg_t *self);

//  Print contents of message to stdout
void
    zsync_ftm_msg_dump (zsync_ftm_msg_t *self);

//  Get/set the message routing id
zframe_t *
    zsync_ftm_msg_routing_id (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_routing_id (zsync_ftm_msg_t *self, zframe_t *routing_id);

//  Get the zsync_ftm_msg id and printable command
int
    zsync_ftm_msg_id (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_id (zsync_ftm_msg_t *self, int id);
char *
    zsync_ftm_msg_command (zsync_ftm_msg_t *self);

//  Get/set the sender field
char *
    zsync_ftm_msg_sender (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_sender (zsync_ftm_msg_t *self, char *format, ...);

//  Get/set the paths field
zlist_t *
    zsync_ftm_msg_paths (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_paths (zsync_ftm_msg_t *self, zlist_t *paths);

//  Iterate through the paths field, and append a paths value
char *
    zsync_ftm_msg_paths_first (zsync_ftm_msg_t *self);
char *
    zsync_ftm_msg_paths_next (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_paths_append (zsync_ftm_msg_t *self, char *format, ...);
size_t
    zsync_ftm_msg_paths_size (zsync_ftm_msg_t *self);

//  Get/set the credit field
uint64_t
    zsync_ftm_msg_credit (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_credit (zsync_ftm_msg_t *self, uint64_t credit);

//  Get/set the receiver field
char *
    zsync_ftm_msg_receiver (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_receiver (zsync_ftm_msg_t *self, char *format, ...);

//  Get/set the path field
char *
    zsync_ftm_msg_path (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_path (zsync_ftm_msg_t *self, char *format, ...);

//  Get/set the sequence field
uint64_t
    zsync_ftm_msg_sequence (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_sequence (zsync_ftm_msg_t *self, uint64_t sequence);

//  Get/set the chunk_size field
uint64_t
    zsync_ftm_msg_chunk_size (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_chunk_size (zsync_ftm_msg_t *self, uint64_t chunk_size);

//  Get/set the offset field
uint64_t
    zsync_ftm_msg_offset (zsync_ftm_msg_t *self);
void
    zsync_ftm_msg_set_offset (zsync_ftm_msg_t *self, uint64_t offset);

//  Self test of this class
int
    zsync_ftm_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
