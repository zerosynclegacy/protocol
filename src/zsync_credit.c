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
    uint64_t requested_bytes;  // Bytes requests for all files
    uint64_t credited_bytes;  // Bytes credited to other peer 
    uint64_t received_bytes;  // Bytes received from other peer 
};

typedef struct _zsync_credit_t zsync_credit_t;

zsync_credit_t *
zsync_credit_new ()
{
    zsync_credit_t *self = (zsync_credit_t *) zmalloc (sizeof (zsync_credit_t));
    self->requested_bytes = 0;
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
s_credit_dump (zsync_credit_t *self) {
    assert (self);
    printf ("[CR] Credit Dump:\n");
    printf ("     Requested %"PRId64"\n", self->requested_bytes);
    printf ("     Credited %"PRId64"\n", self->credited_bytes);
    printf ("     Received %"PRId64"\n", self->received_bytes);
}

void
zsync_credit_manager_engine (void *args, zctx_t *ctx, void *pipe)
{
    uint64_t total_credit = TOTAL_CREDIT;
    zhash_t *peer_credit = zhash_new ();
    zsync_credit_msg_t *msg;
    int rc;

    zpoller_t *poller = zpoller_new (pipe, NULL);

    printf("[CR] started with %"PRId64" bytes credit\n", total_credit);
    while (!zpoller_terminated (poller)) {
        void *which = zpoller_wait (poller, -1);
        
        if (which == pipe) {
            msg = zsync_credit_msg_recv (pipe);
            assert (msg);
        } 
        else 
        if (which == NULL) {
            // stop thread if being interupted
            break;
        }
        
        char *sender = zsync_credit_msg_sender (msg);
       // Get credit information for sender
        zsync_credit_t *credit = zhash_lookup (peer_credit, sender);
        if (!credit) {
           credit = zsync_credit_new ();
           zhash_insert (peer_credit, sender, credit);
           zhash_freefn (peer_credit, sender, s_destroy_credit_item);
        }
        
        switch (zsync_credit_msg_id (msg)) {
            case ZSYNC_CREDIT_MSG_REQUEST:
                credit->requested_bytes += zsync_credit_msg_req_bytes (msg);
                printf("[CR] [RECV] request %"PRId64"\n", credit->requested_bytes);
                break;
            case ZSYNC_CREDIT_MSG_UPDATE:
                credit->received_bytes += zsync_credit_msg_recv_bytes (msg);
                total_credit += zsync_credit_msg_recv_bytes (msg);
                printf("[CR] [RECV] update %"PRId64"\n", credit->received_bytes);
                break;
            case ZSYNC_CREDIT_MSG_ABORT:
                zhash_delete (peer_credit, sender);
                printf("[CR] [RECV] abort\n");
                break;
            case ZSYNC_CREDIT_MSG_TERMINATE: {
                zmsg_t *tmsg = zmsg_new ();
                zmsg_pushstr (tmsg, "OK");
                zmsg_send (&tmsg, pipe);
                break;
             }
        }
        zsync_credit_msg_destroy (&msg);
        
        if (total_credit >= CHUNK_SIZE) {

            uint64_t credit_left = credit->credited_bytes - credit->received_bytes;
            if (credit_left <= CHUNK_SIZE * 2) {
                uint64_t new_credit = 0;
                uint64_t uncredited_left = credit->requested_bytes - credit->credited_bytes;
                // send more credit
                int chunks_left = (uncredited_left / CHUNK_SIZE);
                // s_credit_dump (credit);
                if (chunks_left >= 10) {
                    new_credit = CHUNK_SIZE * 10;                  
                } 
                else if (uncredited_left > 0 ) {
                    new_credit = uncredited_left;
                } 
                else {
                    continue;
                }
                zmsg_t *cmsg = zmsg_new ();
                rc = zs_msg_pack_give_credit (cmsg, new_credit);
                assert (rc == 0);
                zmsg_pushstr (cmsg, sender);
                zmsg_send (&cmsg, pipe);
                total_credit -= new_credit;
                credit->credited_bytes += new_credit;
                printf("[CR] [SEND] credit: %"PRId64", to %s\n", new_credit, sender);
            }
        }
    }
    zhash_destroy (&peer_credit);
    printf("[CR] stopped\n");
}

static void
s_test_send_request (void *pipe, char *peer, uint64_t amount) {
    zsync_credit_msg_send_request (pipe, peer, amount);    
}

static void
s_test_send_update (void *pipe, char *peer, uint64_t amount) {
    zsync_credit_msg_send_update (pipe, peer, amount);    
}

static void
s_test_expect_credit (void *pipe, char *peer, uint64_t amount) 
{
    zmsg_t *msg = zmsg_recv (pipe);
    char *uuid = zmsg_popstr (msg);
    zs_msg_t *zs_msg = zs_msg_unpack (msg);
    printf ("[%s] expect: %"PRId64"; actual: %"PRId64"\n", uuid, amount, zs_msg_get_credit (zs_msg));
    assert (streq (peer, uuid));
    assert (amount == zs_msg_get_credit (zs_msg)); 
}
 
void
zsync_credit_test ()
{
    printf(" * zsync_credit: ");

    zctx_t *ctx = zctx_new ();
    void *pipe = zthread_fork (ctx, zsync_credit_manager_engine, NULL);

    char *peer1 = "0001";
    char *peer2 = "0002";

    // Request from Peer 1
    s_test_send_request (pipe, peer1, 310000);
    s_test_expect_credit (pipe, peer1, 300000);
    s_test_send_update (pipe, peer1, 239000);
    zclock_sleep (100);                             // Give time for a message to arrive
    assert (zmsg_recv_nowait (pipe) == NULL);
    s_test_send_update (pipe, peer1, 1000);
    s_test_expect_credit (pipe, peer1, 10000);
    s_test_send_update (pipe, peer1, 70000);
    zclock_sleep (100);
    assert (zmsg_recv_nowait (pipe) == NULL);

    // Request from Peer 2
    s_test_send_request (pipe, peer2, 650000);
    s_test_expect_credit (pipe, peer2, 300000);
    s_test_send_update (pipe, peer2, 300000);
    s_test_expect_credit (pipe, peer2, 300000);
    s_test_send_update (pipe, peer2, 300000);
    s_test_expect_credit (pipe, peer2, 50000);
    s_test_send_update (pipe, peer2, 50000);
    zclock_sleep (100);
    assert (zmsg_recv_nowait (pipe) == NULL);

    // Terminate
    zmsg_t *tmsg = zmsg_new ();
    zmsg_addstr (tmsg, "TERMINATE");
    zmsg_addstr (tmsg, "TERMINATE");
    zmsg_send (&tmsg, pipe);
    tmsg = zmsg_recv (pipe);
    zmsg_destroy (&tmsg);

    // Cleanup
    zctx_destroy (&ctx);

    printf("OK\n");
}

