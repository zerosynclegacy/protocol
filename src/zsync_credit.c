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

#define TOTAL_CREDIT CHUNK_SIZE * 100;

struct _zsync_credit_t {
    uint64_t req_bytes;
    uint64_t approved_bytes;
    uint64_t recv_bytes;
};

typedef struct _zsync_credit_t zsync_credit_t;

zsync_credit_t *
zsync_credit_new ()
{
    zsync_credit_t *self = (zsync_credit_t *) zmalloc (sizeof (zsync_credit_t));
    self->req_bytes = 0;
    self->approved_bytes = 0;
    self->recv_bytes= 0;
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
zsync_credit_destroy_item (void *data) 
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

    printf("[CR] started\n");
    while (zsync_agent_running (agent)) {
        msg = zmsg_recv (pipe);
        assert (msg);
         
        printf("[CR] recv\n");
        // First frame is sender
        char *sender = zmsg_popstr (msg);
        // Get credit information for sender
        zsync_credit_t *credit = zhash_lookup (peer_credit, sender);
        if (!credit) {
           credit = zsync_credit_new ();
           zhash_insert (peer_credit, sender, credit);
           zhash_freefn (peer_credit, sender, zsync_credit_destroy_item);
        }
        // Second frame is command
        char *command = zmsg_popstr (msg);
        if (streq (command, "REQUEST")) {
            uint64_t req_bytes;
            sscanf (zmsg_popstr (msg), "%"SCNd64, &req_bytes);
            printf("[CR] request %"PRId64"\n", req_bytes);
            credit->req_bytes += req_bytes;
        }
        else
        if (streq (command, "UPDATE")) {
            uint64_t recv_bytes; 
            sscanf (zmsg_popstr (msg), "%"SCNd64, &recv_bytes);
            credit->recv_bytes -= recv_bytes;
            printf("[CR] update %"PRId64"\n", recv_bytes);
            total_credit += recv_bytes;
        }
        zmsg_destroy (&msg);
        
        if (total_credit >= CHUNK_SIZE) {
            uint64_t diff = credit->approved_bytes - credit->recv_bytes;
            if (diff <= (CHUNK_SIZE * 2)) {
                // send more credit
                int chunks_left = (credit->req_bytes % CHUNK_SIZE) + 1;
                if (chunks_left > 10) {
                    chunks_left = 10;                   
                }
                zmsg_t *cmsg = zmsg_new ();
                rc = zs_msg_pack_give_credit (cmsg,  (uint16_t) chunks_left * CHUNK_SIZE);
                assert (rc == 0);
                zmsg_pushstr (cmsg, "%s", sender);
                zmsg_send (&cmsg, pipe);
                total_credit -= (chunks_left * CHUNK_SIZE);
            }
        }
    }
    printf("[CR] stopped\n");
    zhash_destroy (&peer_credit);
}

