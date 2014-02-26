/* =========================================================================
    zsync_credit - Credit Manager 

   -------------------------------------------------------------------------
   Copyright (c) 2013 Kevin Sapper
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
    ZeroSync credit manager

    Handles credit apprvals for outgoing file requests.
    By observing outgoing file requests and incoming chunks it manages to
    total credit that is feasible.   
@discuss
    LOG message to LOG group on whisper and shout
@end
*/

#include "zsync_classes.h"

#define TOTAL_CREDIT 1024 * 1024 * 100; // -> 100 MB

struct _zsync_credit_t {
    uint64_t reqested_bytes;  // Bytes requests for all files
    uint64_t credited_bytes;  // Bytes credited to other peer 
    uint64_t received_bytes;  // Bytes received from other peer 
};

typedef struct _zsync_credit_t zsync_credit_t;

zsync_credit_t *
zsync_credit_new ()
{
    zsync_credit_t *self = (zsync_credit_t *) zmalloc (sizeof (zsync_credit_t));
    self->reqested_bytes = 0;
    self->credited_bytes = 0;
    self->received_bytes = 0;
    return self;
}

void
zsync_credit_destroy (zsync_credit_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_credit_t *self = *self_p;
        free (self);
        self_p = NULL;
    }
}

void
s_destroy_credit_item (void *data) 
{
    zsync_credit_t * credit = (zsync_credit_t *) data;
    zsync_credit_destroy (&credit);        
}

void
zsync_credit_manager_engine (void *args, zctx_t *ctx, void *pipe)
{
    uint64_t total_credit = TOTAL_CREDIT;
    zhash_t *peer_credit = zhash_new ();
    zsync_agent_t *agent = (zsync_agent_t *) args;
    zmsg_t *msg;
    int rc;

    printf("[CR] started with %"PRId64" bytes credit\n", total_credit);
    while (zsync_agent_running (agent)) {
        msg = zmsg_recv (pipe);
        assert (msg);
         
        // First frame is sender
        char *sender = zmsg_popstr (msg);
        // Get credit information for sender
        zsync_credit_t *credit = zhash_lookup (peer_credit, sender);
        if (!credit) {
           credit = zsync_credit_new ();
           zhash_insert (peer_credit, sender, credit);
           zhash_freefn (peer_credit, sender, s_destroy_credit_item);
        }
        // Second frame is command
        char *command = zmsg_popstr (msg);
        if (streq (command, "REQUEST")) {
            uint64_t req_bytes;
            sscanf (zmsg_popstr (msg), "%"SCNd64, &req_bytes);
            printf("[CR] [RECV] request %"PRId64"\n", req_bytes);
            credit->reqested_bytes += req_bytes;
        }
        else
        if (streq (command, "UPDATE")) {
            uint64_t recv_bytes; 
            sscanf (zmsg_popstr (msg), "%"SCNd64, &recv_bytes);
            credit->received_bytes += recv_bytes;
            printf("[CR] [RECV] update %"PRId64"\n", recv_bytes);
            total_credit += recv_bytes;
        }
        else 
        if (streq (command, "ABORT")) {
            zhash_delete (peer_credit, sender);
            printf("[CR] [RECV] abort\n");
        }
        else
        if (streq (command, "SHUTDOWN")) {
            break;
        }
        zmsg_destroy (&msg);
        
        if (total_credit >= CHUNK_SIZE) {
            uint64_t credit_left = credit->credited_bytes - credit->received_bytes;
            if (credit_left <= CHUNK_SIZE * 2) {
                uint64_t new_credit = 0;
                uint64_t requested_bytes_left = credit->reqested_bytes - credit->received_bytes;
                // send more credit
                int chunks_left = (requested_bytes_left % CHUNK_SIZE) + 1;
                if (chunks_left > 10) {
                    new_credit = CHUNK_SIZE * 10;                  
                } else {
                    new_credit = requested_bytes_left;
                }
                zmsg_t *cmsg = zmsg_new ();
                rc = zs_msg_pack_give_credit (cmsg, new_credit);
                assert (rc == 0);
                zmsg_pushstr (cmsg, sender);
                zmsg_send (&cmsg, pipe);
                total_credit -= new_credit;
                printf("[CR] [SEND] credit: %"PRId64", to %s\n", new_credit, sender);
            }
        }
    }
    printf("[CR] stopped\n");
    zhash_destroy (&peer_credit);
}

void
zsync_credit_test ()
{
    printf(" * zsync_credit: ");
    
    printf("OK\n");
}

