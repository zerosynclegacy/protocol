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
    LOGGER into zyre_event.
    LOG message to LOG group on whisper and shout
@end
*/

#include "zsync_classes.h"

#define UUID_FILE ".zsync_uuid"
#define PEER_STATES_FILE ".zsync_peer_states"

struct _zsync_node_t {
    zctx_t *ctx;
    void *pipe;                 // Pipe to communicate with agent
    zyre_t *zyre;               // Zyre
    zsync_agent_t *agent;       // Zsync agent
    void *credit_pipe;          // Pipe to credit manager
    void *file_pipe;            // Pipe to file manager
    zuuid_t *own_uuid;          // uuid of this node
    zlist_t *peers;
    zhash_t *zyre_peers;        // mapping of zyre id to zsync peers
    bool terminated;
};

static zsync_node_t *
zsync_node_new ()
{
    int rc;
    zsync_node_t *self = (zsync_node_t *) zmalloc (sizeof (zsync_node_t));
   
    self->ctx = zctx_new ();
    assert (self->ctx);
    self->zyre = zyre_new (self->ctx);  
    assert (self->zyre);
    
    // Obtain permanent UUID
    self->own_uuid = zuuid_new ();
    if (zsys_file_exists (UUID_FILE)) {
        // Read uuid from file
        zfile_t *uuid_file = zfile_new (".", UUID_FILE);
        int rc = zfile_input (uuid_file);    // open file for reading        
        assert (rc == 0); 

        zchunk_t *uuid_chunk = zfile_read (uuid_file, 16, 0);
        assert (zchunk_size (uuid_chunk) == 16);    // make sure read succeeded
        zuuid_set (self->own_uuid, zchunk_data (uuid_chunk));
        zfile_destroy (&uuid_file);
    } else {
        // Write uuid to file
        zfile_t *uuid_file = zfile_new (".", UUID_FILE);
        rc = zfile_output (uuid_file); // open file for writing
        assert (rc == 0);
        zchunk_t *uuid_bin = zchunk_new ( zuuid_data (self->own_uuid), 16);
        rc = zfile_write (uuid_file, uuid_bin, 0);
        assert (rc == 0);
        zfile_destroy (&uuid_file);
    }
    
    // Obtain peers and states
    self->peers = zlist_new ();
    if (zsys_file_exists (PEER_STATES_FILE)) {
        zhash_t *peer_states = zhash_new ();
        int rc = zhash_load (peer_states, PEER_STATES_FILE);
        assert (rc == 0);
        zlist_t *uuids = zhash_keys (peer_states);
        char *uuid = zlist_first (uuids);
        while (uuid) {
            char * state_str = zhash_lookup (peer_states, uuid);
            uint64_t state;
            sscanf (state_str, "%"SCNd64, &state);
            zlist_append (self->peers, zsync_peer_new (uuid, state)); 
            uuid = zlist_next (uuids);
        }
    }
    
    self->zyre_peers = zhash_new ();
    self->terminated = false;
    return self;
}

static void
zsync_node_destroy (zsync_node_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_node_t *self = *self_p;
        
        zuuid_destroy (&self->own_uuid);
      
        // TODO destroy all zsync_peers
        zlist_destroy (&self->peers);
        zhash_destroy (&self->zyre_peers);

        free (self);
        self_p = NULL;
    }
}

static zsync_peer_t *
zsync_node_peers_lookup (zsync_node_t *self, char *uuid)
{
    assert (self);
    zsync_peer_t *peer = zlist_first (self->peers);
    while (peer) {
        if (streq (zsync_peer_uuid (peer), uuid)) {
            return peer;
        }
        peer = zlist_next (self->peers);
    }
    return NULL;
}

static int
zsync_node_save_peers (zsync_node_t *self)
{
    assert (self);
    int rc;
    
    // Obtain peer state key-values
    zhash_t *peer_states = zhash_new ();
        
    zsync_peer_t *peer = zlist_first (self->peers);
    while (peer) {
        char *uuid = zsync_peer_uuid (peer);
        char state[20];
        sprintf (state, "%"PRId64, zsync_peer_state (peer));
        zhash_insert (peer_states, uuid, state); 
        peer = zlist_next (self->peers);
    }
    rc = zhash_save (peer_states, PEER_STATES_FILE);
    return rc;
}

static char *
zsync_node_zyre_uuid (zsync_node_t *self, char *sender)
{
    assert (self);
    assert (sender);

    zlist_t *keys = zhash_keys (self->zyre_peers);
    char *key = zlist_first (keys);
    while (key) {
        zsync_peer_t *peer = zhash_lookup (self->zyre_peers, key);
        if (peer && streq (sender, zsync_peer_uuid (peer))) {
           return key;
        }
        key = zlist_next (keys);
    }
    return NULL;
}
    
static void
zsync_node_recv_from_zyre (zsync_node_t *self)
{
    zsync_peer_t *sender;
    char *zyre_sender;
    zuuid_t *sender_uuid;
    zmsg_t *zyre_in, *zyre_out, *fm_msg;
    zlist_t *fpaths, *fmetadata;
    
    zyre_event_t *event = zyre_event_recv (self->zyre);
    zyre_sender = zyre_event_sender (event); // get tmp uuid

    switch (zyre_event_type (event)) {
        case ZYRE_EVENT_ENTER:
            printf("[ND] ZS_ENTER: %s\n", zyre_sender);
            zhash_insert (self->zyre_peers, zyre_sender, NULL);
            break;        
        case ZYRE_EVENT_JOIN:
            printf ("[ND] ZS_JOIN: %s\n", zyre_sender);
            // send GREET message
            zyre_out = zmsg_new ();
            uint64_t state = zsync_agent_current_state(self->agent); 
            zs_msg_pack_greet (zyre_out, zuuid_data (self->own_uuid), state);
            zyre_whisper (self->zyre, zyre_sender, &zyre_out);
            break;
        case ZYRE_EVENT_LEAVE:
        case ZYRE_EVENT_EXIT:
            /*
            printf("[ND] ZS_EXIT %s left the house!\n", zyre_sender);
            sender = zhash_lookup (self->zyre_peers, zyre_sender);
            if (sender) {
                // Reset Managers
                zmsg_t *reset_msg = zmsg_new ();
                zmsg_addstr (reset_msg, zsync_peer_uuid (sender));
                zmsg_addstr (reset_msg, "ABORT");
                zmsg_send (&reset_msg, self->file_pipe);
                reset_msg = zmsg_new ();
                zmsg_addstr (reset_msg, zsync_peer_uuid (sender));
                zmsg_addstr (reset_msg, "ABORT");
                zmsg_send (&reset_msg, self->credit_pipe);
                // Remove Peer from active list
                zhash_delete (self->zyre_peers, zyre_sender);
            }*/
            break;
        case ZYRE_EVENT_WHISPER:
        case ZYRE_EVENT_SHOUT:
            printf ("[ND] ZS_WHISPER: %s\n", zyre_sender);
            sender = zhash_lookup (self->zyre_peers, zyre_sender);
            zyre_in = zyre_event_msg (event);
            zs_msg_t *msg = zs_msg_unpack (zyre_in);
            switch (zs_msg_get_cmd (msg)) {
                case ZS_CMD_GREET:
                    // 1. Get perm uuid
                    sender_uuid = zuuid_new ();
                    zuuid_set (sender_uuid, zs_msg_uuid (msg));
                    sender = zsync_node_peers_lookup (self, zuuid_str (sender_uuid));
                    if (!sender) {
                        sender = zsync_peer_new (zuuid_str (sender_uuid), 0x0);
                        zlist_append (self->peers, sender);
                    } 
                    assert (sender);
                    zhash_update (self->zyre_peers, zyre_sender, sender);
                    // 2. Get current state for sender
                    uint64_t remote_current_state = zs_msg_get_state (msg);
                    printf ("[ND] current state: %"PRId64"\n", remote_current_state);
                    // 3. Lookup last known state
                    uint64_t last_state_local = zsync_peer_state (sender);
                    printf ("[ND] last known state: %"PRId64"\n", zsync_peer_state (sender));
                    // 4. Update peer attributes
                    zsync_peer_set_connected (sender, true);
                    // 5. Send LAST_STATE if differs 
                    if (remote_current_state >= last_state_local) {
                        zmsg_t *lmsg = zmsg_new ();
                        zs_msg_pack_last_state (lmsg, last_state_local);
                        zyre_whisper (self->zyre, zyre_sender, &lmsg);
                    } else { 
                        zsync_peer_set_ready (sender, true);
                    }
                    break;
                case ZS_CMD_LAST_STATE:
                    assert (sender);
                    zyre_out = zmsg_new ();
                    // Gets updates from client
                    uint64_t last_state_remote = zs_msg_get_state (msg);
                    zlist_t *filemeta_list = zsync_agent_update (self->agent, last_state_remote);
                   
                    if (filemeta_list) { 
                        // Send UPDATE
                        uint64_t my_current_state = zsync_agent_current_state (self->agent);
                        zs_msg_pack_update (zyre_out, my_current_state, filemeta_list);
                        zyre_whisper (self->zyre, zyre_sender, &zyre_out);
                    }
                    break;
                case ZS_CMD_UPDATE:
                    printf ("[ND] UPDATE\n");
                    assert (sender);
                    zsync_peer_set_ready (sender, true);
                    uint64_t state = zs_msg_get_state (msg);
                    zsync_peer_set_state (sender, state); 
                    zsync_node_save_peers (self);

                    fmetadata = zs_msg_get_fmetadata (msg);
                    zsync_agent_pass_update (self->agent, zsync_peer_uuid (sender), fmetadata);
                    break;
                case ZS_CMD_REQUEST_FILES:
                    printf ("[ND] REQUEST FILES\n");
                    fpaths = zs_msg_fpaths (msg);
                    zmsg_t *fm_msg = zmsg_new ();
                    zmsg_addstr (fm_msg, zsync_peer_uuid (sender));
                    zmsg_addstr (fm_msg, "REQUEST");
                    char *fpath = zs_msg_fpaths_first (msg);
                    while (fpath) {
                        zmsg_addstr (fm_msg, fpath);
                        printf("[ND] %s\n", fpath);
                        fpath = zs_msg_fpaths_next (msg);
                    }
                    zmsg_send (&fm_msg, self->file_pipe);
                    break;
                case ZS_CMD_GIVE_CREDIT:
                    printf("[ND] GIVE CREDIT\n");
                    fm_msg = zmsg_new ();
                    zmsg_addstr (fm_msg, zsync_peer_uuid (sender));
                    zmsg_addstr (fm_msg, "CREDIT");
                    zmsg_addstrf (fm_msg, "%"PRId64, zs_msg_get_credit (msg));
                    zmsg_send (&fm_msg, self->file_pipe);
                    break;
                case ZS_CMD_SEND_CHUNK:
                    printf("[ND] SEND_CHUNK (RCV)\n");
                    // Send receival to credit manager
                    zframe_t *zframe = zs_msg_get_chunk (msg);
                    uint64_t chunk_size = zframe_size (zframe);
                    zmsg_t *cmsg = zmsg_new ();
                    zmsg_addstr (cmsg, zsync_peer_uuid (sender));
                    zmsg_addstr (cmsg, "UPDATE");
                    zmsg_addstrf (cmsg, "%"PRId64, chunk_size);
                    zmsg_send (&cmsg, self->credit_pipe);
                    // Pass chunk to client
                    byte *data = zframe_data (zframe);
                    zchunk_t *chunk = zchunk_new (data, chunk_size);
                    char *path = zs_msg_get_file_path (msg);
                    uint64_t seq = zs_msg_get_sequence (msg);
                    uint64_t off = zs_msg_get_offset (msg);
                    zsync_agent_pass_chunk (self->agent, chunk, path, seq, off);
                    break;
                case ZS_CMD_ABORT:
                    // TODO abort protocol managed file transfer
                    printf("[ND] ABORT\n");
                    break;
                default:
                    assert (false);
                    break;
            }
            
            zs_msg_destroy (&msg);
            break;
        default:
            printf("[ND] Error command not found\n");
            break;
    }
    zyre_event_destroy (&event);
}

void
zsync_node_recv_from_agent (zsync_node_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_recv (self->pipe);
    char *command = zmsg_popstr (msg);
    if (streq (command, "REQUEST")) {
        char *sender = zmsg_popstr (msg);
        char *zyre_uuid = zsync_node_zyre_uuid (self, sender);
        if (zyre_uuid) {
            char *total_bytes = zmsg_popstr (msg);
            assert (zyre_uuid);
            printf("[ND] Recv Agent WHISPER REQUEST %s ; %s\n", zyre_uuid, sender);
            zyre_whisper (self->zyre, zyre_uuid, &msg);
            
            zmsg_t *cmsg = zmsg_new ();
            zmsg_addstr (cmsg, sender);
            zmsg_addstr (cmsg, "REQUEST");
            zmsg_addstr (cmsg, total_bytes);
            zmsg_send (&cmsg, self->credit_pipe);
        }
    }
    else
    if (streq (command, "UPDATE")) {
        printf("[ND] Recv Agent SHOUT UPDATE\n");
        zyre_shout (self->zyre, zsync_agent_channel (self->agent), &msg);
    }
    else
    if (streq (command, "TERMINATE")) {
        // terminate file manager
        zmsg_t *msg = zmsg_new ();
        zmsg_addstr (msg, "TERMINATE");
        zmsg_addstr (msg, "TERMINATE");
        zmsg_send (&msg, self->file_pipe);
        // terminate credit manager
        msg = zmsg_new ();
        zmsg_addstr (msg, "TERMINATE");
        zmsg_addstr (msg, "TERMINATE");
        zmsg_send (&msg, self->credit_pipe);
        
        // receive termination confirmation
        msg = zmsg_recv (self->file_pipe);
        zmsg_destroy (&msg);
        printf("OK ft\n");
        msg = zmsg_recv (self->credit_pipe);
        zmsg_destroy (&msg);
        printf("OK cm\n");

        // send shutdown confirmation to agent
        msg = zmsg_new ();
        zmsg_addstr (msg, "OK");
        zmsg_send (&msg, self->pipe);
        printf("OK agent\n");
        
        self->terminated = true;
    }
}



void
zsync_node_engine (void *args, zctx_t *ctx, void *pipe)
{
    int rc;    
    zsync_agent_t *agent = (zsync_agent_t *) args;
    zsync_node_t *self = zsync_node_new ();
    self->ctx = zsync_agent_ctx (agent);
    self->zyre = zsync_agent_zyre (agent);
    self->pipe = pipe;
    self->agent = agent;
    
    // Join group
    rc = zyre_join (self->zyre, zsync_agent_channel (self->agent));
    assert (rc == 0);

    // Give time to interconnect
    zclock_sleep (250);

    zpoller_t *poller = zpoller_new (zyre_socket (self->zyre), self->pipe, NULL);
    
    // Create thread for file management
    self->file_pipe = zthread_fork (self->ctx, zsync_ftmanager_engine, agent);
    zpoller_add (poller, self->file_pipe);
    // Create thread for credit management
    self->credit_pipe = zthread_fork (self->ctx, zsync_credit_manager_engine, agent);
    zpoller_add (poller, self->credit_pipe);

    // Start receiving messages
    printf("[ND] started\n");
    while (!zpoller_terminated (poller)) {
        void *which = zpoller_wait (poller, -1);
        
        if (which == zyre_socket (self->zyre)) {
            zsync_node_recv_from_zyre (self);
        } 
        else
        if (which == self->pipe) {
            printf("[ND] Recv Agent\n");
            zsync_node_recv_from_agent (self);
        }
        else
        if (which == self->file_pipe) {
            printf("[ND] Recv FT Manager\n");
            zmsg_t *msg = zmsg_recv (self->file_pipe);
            char *sender = zmsg_popstr (msg);
            char *zyre_uuid = zsync_node_zyre_uuid (self, sender);
            if (zyre_uuid) {
                zmsg_t *zmsg = zmsg_dup (msg);
                zyre_whisper (self->zyre, zyre_uuid, &zmsg);
                zmsg_destroy (&msg);
            }
        }
        else
        if (which == self->credit_pipe) {
            printf("[ND] Recv Credit Manager\n");
            zmsg_t *msg = zmsg_recv (self->credit_pipe);
            char *sender = zmsg_popstr (msg);
            char *zyre_uuid = zsync_node_zyre_uuid (self, sender);
            if (zyre_uuid) {
                zmsg_t *zmsg = zmsg_dup (msg);
                zyre_whisper (self->zyre, zyre_uuid, &zmsg);
                zmsg_destroy (&msg);
            }
        }
        if (self->terminated) {
            break;
        }
    }
    zpoller_destroy (&poller);
    printf("[ND] stopped1\n");
    zsync_node_destroy (&self);
    printf("[ND] stopped\n");
}

int
zsync_node_test () 
{
    printf (" * zsync_node: ");
    printf("OK\n");
}

