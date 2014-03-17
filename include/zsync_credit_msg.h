/*  =========================================================================
    zsync_credit_msg - credit manager api
    
    Codec header for zsync_credit_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

    * The XML model used for this code generation: zsync_credit_msg
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

#ifndef __ZSYNC_CREDIT_MSG_H_INCLUDED__
#define __ZSYNC_CREDIT_MSG_H_INCLUDED__

/*  These are the zsync_credit_msg messages:

    REQUEST - Bytes requested from other peer
        sender              string      
        req_bytes           number 8    

    UPDATE - Bytes received from other peer
        sender              string      
        recv_bytes          number 8    

    GIVE_CREDIT - Sends an encoded credit to be forwarded to the receiver.
        receiver            string      
        credit              msg         

    ABORT - Abort sending credit to other peer

    TERMINATE - Terminate the worker thread
*/

#define ZSYNC_CREDIT_MSG_VERSION            1

#define ZSYNC_CREDIT_MSG_REQUEST            1
#define ZSYNC_CREDIT_MSG_UPDATE             2
#define ZSYNC_CREDIT_MSG_GIVE_CREDIT        3
#define ZSYNC_CREDIT_MSG_ABORT              4
#define ZSYNC_CREDIT_MSG_TERMINATE          5

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zsync_credit_msg_t zsync_credit_msg_t;

//  @interface
//  Create a new zsync_credit_msg
zsync_credit_msg_t *
    zsync_credit_msg_new (int id);

//  Destroy the zsync_credit_msg
void
    zsync_credit_msg_destroy (zsync_credit_msg_t **self_p);

//  Parse a zsync_credit_msg from zmsg_t. Returns new object or NULL if error.
//  Use when not in control of receiving the message.
zsync_credit_msg_t *
    zsync_credit_msg_decode (zmsg_t *msg);

//  Encode zsync_credit_msg into zmsg and destroy it. 
//  Returns a newly created object or NULL if error. 
//  Use when not in control of sending the message.
zmsg_t *
    zsync_credit_msg_encode (zsync_credit_msg_t *self);

//  Receive and parse a zsync_credit_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
zsync_credit_msg_t *
    zsync_credit_msg_recv (void *input);

//  Receive and parse a zsync_credit_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
zsync_credit_msg_t *
    zsync_credit_msg_recv_nowait (void *input);

//  Send the zsync_credit_msg to the output, and destroy it
int
    zsync_credit_msg_send (zsync_credit_msg_t **self_p, void *output);

//  Send the zsync_credit_msg to the output, and do not destroy it
int
    zsync_credit_msg_send_again (zsync_credit_msg_t *self, void *output);

//  Send the REQUEST to the output in one step
int
    zsync_credit_msg_send_request (void *output,
        char *sender,
        uint64_t req_bytes);
    
//  Send the UPDATE to the output in one step
int
    zsync_credit_msg_send_update (void *output,
        char *sender,
        uint64_t recv_bytes);
    
//  Send the GIVE_CREDIT to the output in one step
int
    zsync_credit_msg_send_give_credit (void *output,
        char *receiver,
        zmsg_t *credit);
    
//  Send the ABORT to the output in one step
int
    zsync_credit_msg_send_abort (void *output);
    
//  Send the TERMINATE to the output in one step
int
    zsync_credit_msg_send_terminate (void *output);
    
//  Duplicate the zsync_credit_msg message
zsync_credit_msg_t *
    zsync_credit_msg_dup (zsync_credit_msg_t *self);

//  Print contents of message to stdout
void
    zsync_credit_msg_dump (zsync_credit_msg_t *self);

//  Get/set the message routing id
zframe_t *
    zsync_credit_msg_routing_id (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_routing_id (zsync_credit_msg_t *self, zframe_t *routing_id);

//  Get the zsync_credit_msg id and printable command
int
    zsync_credit_msg_id (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_id (zsync_credit_msg_t *self, int id);
char *
    zsync_credit_msg_command (zsync_credit_msg_t *self);

//  Get/set the sender field
char *
    zsync_credit_msg_sender (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_sender (zsync_credit_msg_t *self, char *format, ...);

//  Get/set the req_bytes field
uint64_t
    zsync_credit_msg_req_bytes (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_req_bytes (zsync_credit_msg_t *self, uint64_t req_bytes);

//  Get/set the recv_bytes field
uint64_t
    zsync_credit_msg_recv_bytes (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_recv_bytes (zsync_credit_msg_t *self, uint64_t recv_bytes);

//  Get/set the receiver field
char *
    zsync_credit_msg_receiver (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_receiver (zsync_credit_msg_t *self, char *format, ...);

//  Get/set the credit field
zmsg_t *
    zsync_credit_msg_credit (zsync_credit_msg_t *self);
void
    zsync_credit_msg_set_credit (zsync_credit_msg_t *self, zmsg_t *msg);

//  Self test of this class
int
    zsync_credit_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
