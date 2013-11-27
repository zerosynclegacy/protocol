/* =========================================================================
    zsync_node - node on ZeroSync network

   -------------------------------------------------------------------------
   Copyright (c) 2013 Kevin Sapper, Bernhard Finger
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
    ZeroSync node

    A node is one instance on the ZeroSync network
@discuss
@end
*/

#include "zsync_classes.h"

#define UUID_FILE ".zsync_uuid"
#define UUIDS_FILE ".zsync_uuids"
#define UUID_BIN_SIZE 16

struct _zsync_node_t {
    zctx_t *ctx;
    zyre_t *zyre;               // zyre main struct
    zuuid_t *own_uuid;          // uuid of this node
    // TODO move into own peer class
    zhash_t *peer_uuids;        // mapping of our peers temporary and permanent uuid
    zhash_t *peer_fstates;      // current flow state of our peers
    zhash_t *peer_ustates;      // last known states of out peers
    // TODO peer -> connected and ready
};

static zsync_node_t *
zsync_node_new ()
{
    int rc;
    zsync_node_t *self = (zsync_node_t *) zmalloc (sizeof (zsync_node_t));
   
    self->ctx = zctx_new ();
    self->zyre = zyre_new (self->ctx);  

    self->own_uuid = zuuid_new ();
    if (zsys_file_exists (UUID_FILE)) {
        // Read uuid from file
        zfile_t *uuid_file = zfile_new (".", UUID_FILE);
        int rc = zfile_input (uuid_file);    // open file for reading        
        assert (rc == 0); 

        zchunk_t *uuid_chunk = zfile_read (uuid_file, 16, 0);
        assert (zchunk_size (uuid_chunk) == 16);    // make sure read succeeded
        zuuid_set (self->own_uuid, zchunk_data (uuid_chunk));
    } else {
        // Write uuid to file
        zfile_t *uuid_file = zfile_new (".", UUID_FILE);
        rc = zfile_output (uuid_file); // open file for writing
        assert (rc == 0);
        zchunk_t *uuid_bin = zchunk_new ( zuuid_data (self->own_uuid), 16);
        rc = zfile_write (uuid_file, uuid_bin, 0);
        assert (rc == 0);
    }


    self->peer_uuids = zhash_new ();
    self->peer_fstates = zhash_new ();
    self->peer_ustates = zhash_new ();
    zhash_load (self->peer_ustates, UUIDS_FILE);

    return self;
}

static void
zsync_node_destroy (zsync_node_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_node_t *self = *self_p;
        
        zuuid_destroy (&self->own_uuid);
        zyre_destroy (&self->zyre);
        zctx_destroy (&self->ctx);
       
        // save foreign uuids before destroying 
        int rc = zhash_save (self->peer_ustates, UUIDS_FILE);
        assert (rc == 0);
        zhash_destroy (&self->peer_uuids);
        zhash_destroy (&self->peer_fstates);
        zhash_destroy (&self->peer_ustates);
    }
}

int peer_last_state (zsync_node_t *self, zyre_event_t *event)
{
    assert (self);
    // Get permanent peer uuid
    char *perm_peerid = zhash_lookup (self->peer_uuids, zyre_event_peerid (event));
    char *last_state_str = zhash_lookup (self->peer_ustates, perm_peerid);
    if (last_state_str) {
        int last_state;
        sscanf (last_state_str, "%d", &last_state);
        return last_state;
    } else {
        return 5;
    }
}

void
zsync_node_engine ()
{
    zsync_node_t *self = zsync_node_new ();
    zhash_t *uuid_hash;
    char *s_peer_id;

    // SET uuid the be send with header
    zyre_set_header (self->zyre, "UUID", "%s", zuuid_str (self->own_uuid));

    // Start Zyre
    zyre_start (self->zyre);
    zyre_join (self->zyre, "TEST");

    // Give time to interconnect
    zclock_sleep (250);

    // start recving messages
    int count = 3;
    while (count--) {
        zyre_event_t *event = zyre_event_recv (self->zyre);
        // TODO introdues state flow engine
        switch (zyre_event_cmd (event)) {
            case ZYRE_EVENT_ENTER:
                // get uuid of sending peer
                s_peer_id = zyre_event_get_header (event, "UUID");
                printf("ENTER: %s\n", s_peer_id);
                zhash_update (self->peer_uuids, zyre_event_peerid (event), s_peer_id);
                break;        
            case ZYRE_EVENT_JOIN:
                printf ("JOIN\n");
                int last_state = peer_last_state (self, event); 
                zmsg_t *msg = zmsg_new ();
                zmsg_addstr (msg, zyre_event_peerid (event));
                zs_msg_pack_last_state (msg, last_state);
                zyre_whisper (self->zyre, &msg);
                break;
            case ZYRE_EVENT_WHISPER:
                printf ("WHISPER\n");
                zs_msg_t *zs_msg = zs_msg_unpack (zyre_event_data (event));
                printf ("LAST_STATE: %"PRId64"\n", zs_msg_get_state (zs_msg));
                zs_msg_destroy (&zs_msg);
                break;
            default:
                break;
        }

        zyre_event_destroy (&event);
    }

    zsync_node_destroy (&self);
}

int
zsync_node_test () 
{
    printf ("zsync_node: \n");
    zsync_node_engine ();
    printf("OK\n");
}

