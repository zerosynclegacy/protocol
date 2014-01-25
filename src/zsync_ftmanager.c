/* =========================================================================
    zsync_ftmanager - File Transfer manager

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
    ZeroSync file transfer manager

    Handles incoming file requests from peers and their credit approval.
    Automatically sends chunks of data once a credit approval has been send.
    Outstanding files are only transfered if peer has enough credit.
@discuss
    LOG message to LOG group on whisper and shout
@end
*/

#include "zsync_classes.h"

struct _zsync_ftfile_t {
    char *path;
    uint64_t sequence;
    uint64_t offset;
};

struct _zsync_ftrequest_t {
    zlist_t *requested_files;
    uint64_t credit;
};

typedef struct _zsync_ftfile_t zsync_ftfile_t;
typedef struct _zsync_ftrequest_t zsync_ftrequest_t;

zsync_ftfile_t *
zsync_ftfile_new (char *path)
{
    zsync_ftfile_t *self = (zsync_ftfile_t *) zmalloc (sizeof (zsync_ftfile_t));
    self->path = path;
    self->sequence = 0;
    self->offset = 0;
    return self;
}

void
zsync_ftfile_destroy (zsync_ftfile_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_ftfile_t *self = *self_p;
        free (self->path);
        free (self);
        self_p = NULL;
    }
}

zsync_ftrequest_t *
zsync_ftrequest_new ()
{
    zsync_ftrequest_t *self = (zsync_ftrequest_t *) zmalloc (sizeof (zsync_ftrequest_t));
    self->requested_files = zlist_new ();
    self->credit = 0;
    return self;
}

void
zsync_ftrequest_destroy (zsync_ftrequest_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_ftrequest_t *self = *self_p;
        zsync_ftfile_t *file = zlist_first (self->requested_files);
        while (file) {
            zsync_ftfile_destroy (&file);
            file = zlist_next (self->requested_files);
        }
        zlist_destroy (&self->requested_files);

        free (self);
        self_p = NULL;
    }
}

void
zsync_ftmanager_destroy_item (void *data) 
{
    zsync_ftrequest_t * request = (zsync_ftrequest_t *) data;
    zsync_ftrequest_destroy (&request);        
}

// Helper method that returns true is there is still work left, otherwise false
static bool
s_work_left (zhash_t *self) 
{
    assert (self);
    if (zhash_size (self) == 0)
       return false; 

    zlist_t *keys = zhash_keys (self);
    char *key = zlist_first (keys);
    while (key) {
        zsync_ftrequest_t *request =  zhash_lookup (self, key);
        int request_count = zlist_size (request->requested_files);
        printf("[FT] requests (%d), credit(%"PRId64")\n", request_count, request->credit);
        if (request_count > 0 && request->credit > CHUNK_SIZE) {
            return true; 
        }
        key = zlist_next (keys);
    }
    return false;
}

void
zsync_ftmanager_engine (void *args, zctx_t *ctx, void *pipe)
{
    zhash_t *peer_requests = zhash_new ();
    zsync_agent_t *agent = (zsync_agent_t *) args;
    zmsg_t *msg;
    int rc;

    printf("[FT] started\n");
    while (zsync_agent_running (agent)) {
        // Proceed if there is still work to do and no message are in queue,
        // Otherwise wait for work
        if (s_work_left (peer_requests)) {
            printf("[FT] go into recv nowait\n");
            msg = zmsg_recv_nowait (pipe);
        } else {
            printf("[FT] go into recv\n");
            msg = zmsg_recv (pipe);
        }
        
        if (msg) {
            printf("[FT] recv\n");
            // First frame is sender 
            char *sender = zmsg_popstr (msg);
            // Get file transfer request object
            zsync_ftrequest_t *ftrequest = zhash_lookup (peer_requests, sender);
            if (!ftrequest) {
                ftrequest = zsync_ftrequest_new ();
                zhash_insert (peer_requests, sender, ftrequest);
                zhash_freefn (peer_requests, sender, zsync_ftmanager_destroy_item);
            }

            // Second frame is command
            char *cmd = zmsg_popstr (msg);
            if (streq (cmd, "REQUEST")) {
                char *fpath = zmsg_popstr (msg);
                while (fpath) {
                    // TODO check for duplicates
                    zlist_append (ftrequest->requested_files, zsync_ftfile_new (fpath));
                    printf("[FT] added %s\n", fpath);
                    fpath = zmsg_popstr (msg);
                }
            }
            else 
            if (streq (cmd, "CREDIT")) {
                uint64_t credit;
                sscanf (zmsg_popstr (msg), "%"SCNd64, &credit);
                ftrequest->credit += credit;
                printf("[FT] credit %"PRId64"\n", credit);
            }
            else 
            if (streq (cmd, "ABORT")) {
                printf("[FT] FT_ABORT");
            }
            zmsg_destroy (&msg);
        }
       
        printf("[FT] LOOKUP\n");
        // Search for file transfer request
        // Transfer one chunk then proceed 
        zlist_t *keys = zhash_keys (peer_requests);
        char *key = zlist_first (keys);
        while (key) {
            zsync_ftrequest_t *request =  zhash_lookup (peer_requests, key);
            int request_count = zlist_size (request->requested_files);
            printf("[FT] requests (%d), credit(%"PRId64")\n", request_count, request->credit);
            if (request_count > 0 && request->credit > CHUNK_SIZE) {
                zsync_ftfile_t *file = zlist_first (request->requested_files);
                byte *chunk = zsync_agent_chunk (agent, file->path, CHUNK_SIZE, file->offset);
                if (chunk) {
                    zframe_t *data = zframe_new (chunk, CHUNK_SIZE);
                    zmsg_t *msg = zmsg_new ();
                    rc = zs_msg_pack_chunk (msg, file->sequence, file->path, file->offset, data);
                    assert (rc == 0);
                    // First frame sender uuid
                    zmsg_pushstr (msg, key);
                    zmsg_send (&msg, pipe); // Forward chunk to node
                    // Increment for next chunk
                    file->sequence++;
                    file->offset += CHUNK_SIZE;
                    printf("[FT] chunk send\n");
                } 
                else {
                    printf("[FT] file completed\n");
                    (void *) zlist_pop (request->requested_files); 
                }
                // Advance after sending one chunk, in order to catch abort 
                break;            
            }
            key = zlist_next (keys);
        }
    }
    printf("[FT] stopped\n");
    zhash_destroy (&peer_requests);
}

