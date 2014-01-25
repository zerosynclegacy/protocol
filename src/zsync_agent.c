/* =========================================================================
    zsync_agent - ZeroSync public API

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
    ZeroSync agent

    The agent is responible for controlling the protocols state and workflow.
    It's splitted into message handling and information retrival.
@discuss
    * Add method to ask for current file transfer lsit
    * Add method to give update (from, to)
@end
*/

#include "zsync_classes.h"

struct _zsync_agent_t {
    // Zeromq context
    zctx_t *ctx;

    // Zyre connection
    zyre_t *zyre;

    // Pipe to node
    void *pipe;

    bool running;

    char *channel;

    // Passes a list with updated files
    void (*pass_update)(char *sender, zlist_t *file_metadata);

    // Passes a chunk of bytes to the CLIENT 
    void (*pass_chunk)(byte *chunk, char *path, uint64_t sequence, uint64_t offset);

    // Returns a list of updates starting by the from state to the current state
    zlist_t* (*get_update)(uint64_t from_state);
    
    // Returns a chunk from a file.
    byte* (*get_chunk)(char* path, uint64_t chunk_size, uint64_t offset);

    // Returns the current state of this peer
    uint64_t (*get_current_state)();
};


// ---------------------------------------------------------------------------
// Create a new zsync_agent

zsync_agent_t *
zsync_agent_new ()
{
    zsync_agent_t *self = (zsync_agent_t*) zmalloc (sizeof (zsync_agent_t));
    
    self->ctx = zctx_new ();
    assert (self->ctx);
    self->zyre = zyre_new (self->ctx);
    assert (self->zyre);
    self->channel = "ZSYNC";
    self->running = false;
    
    return self;
}

// --------------------------------------------------------------------------
// Destroys a zsync_agent

void
zsync_agent_destroy (zsync_agent_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_agent_t *self = *self_p;
        
        // stop and destroy zyre
        zyre_stop (self->zyre); 
        zyre_destroy (&self->zyre);

        free(self);
    }
}

// --------------------------------------------------------------------------
// zsync_agent start, failed if a function ptr isn´t set

int
zsync_agent_start (zsync_agent_t *self)
{
    assert (self);
    if (self->pass_update &&
        self->pass_chunk && 
        self->get_update && 
        self->get_current_state && 
        self->get_chunk) {
      
        zyre_start (self->zyre);
        self->pipe = zthread_fork (self->ctx, zsync_node_engine, self);
        self->running = true;
        return 0;
    }
    return 1;
}


// --------------------------------------------------------------------------
// zsync_agent stop, failed if a function ptr isn´t set

void
zsync_agent_stop (zsync_agent_t *self)
{
    assert (self);
    self->running = false;
    zyre_stop (self->zyre);
}

// --------------------------------------------------------------------------
// Sets the pass_update callback method

void 
zsync_agent_set_pass_update (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->pass_update = ptr;
}

// --------------------------------------------------------------------------
// Sets the pass_chunk callback method

void 
zsync_agent_set_pass_chunk (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->pass_chunk = ptr;
}


// --------------------------------------------------------------------------
// Sets the get_update callback method

void
zsync_agent_set_get_update (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->get_update = ptr;
}

// --------------------------------------------------------------------------
// Sets the get_current_state callback method

void 
zsync_agent_set_get_current_state (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->get_current_state = ptr;
}

// --------------------------------------------------------------------------
// Sets the get_chunk callback method

void
zsync_agent_set_get_chunk (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->get_chunk = ptr;
}

// --------------------------------------------------------------------------
// send_request_files for the protocol

void 
zsync_agent_send_request_files (zsync_agent_t *self, char *sender, zlist_t *list, uint64_t total_bytes)
{
    zmsg_t *msg = zmsg_new ();
    // give the Request_files_command to the protocol
    int rc = zs_msg_pack_request_files (msg, list);
    printf("[AG] S_Request_Files %d\n", rc);
    if (rc == 0) {
        zmsg_pushstrf (msg, "%"PRId64, total_bytes);
        zmsg_pushstr (msg, sender);
        zmsg_pushstr (msg, "REQUEST");
        zmsg_send (&msg, self->pipe);
    }
}

// --------------------------------------------------------------------------
// send_update for the protocol

void
zsync_agent_send_update (zsync_agent_t *self, uint64_t state, zlist_t *list)
{
    zmsg_t *msg = zmsg_new ();
    // give the send_update_command to the protocol
    if (zs_msg_pack_update (msg, state, list)) {
        zmsg_pushstr (msg, "UPDATE");
        zmsg_send (&msg, self->pipe);
        printf ("Update sent.\n");    
    }
}

// --------------------------------------------------------------------------
// Sends ABORT message to the protocol

void
zsync_agent_send_abort (zsync_agent_t *self, char *sender, char* fileToAbort)
{
    zmsg_t *msg = zmsg_new ();
    // TODO: Implement the filePath to send_abort in protocol
    // give the send_abort_command to the protocol
    if (zs_msg_pack_abort (msg)) { 
        zmsg_pushstr (msg, sender);
        zmsg_send (&msg, self->pipe);
        printf("Caution, file transfer aborted!!!\n");
    }
}

// --------------------------------------------------------------------------
// Gets the zeromq context

zctx_t *
zsync_agent_ctx (zsync_agent_t *self)
{
    assert (self);
    assert (self->ctx);
    return self->ctx;
}

// --------------------------------------------------------------------------
// Gets the Zyre object

zyre_t *
zsync_agent_zyre (zsync_agent_t *self)
{
    assert (self);
    assert (self->zyre);
    return self->zyre;
}

// --------------------------------------------------------------------------
// Returns the channel the agent joins

char *
zsync_agent_channel (zsync_agent_t *self)
{
    assert (self);
    return self->channel;
}

// --------------------------------------------------------------------------
// Returns whether the agents has been started or not

bool
zsync_agent_running (zsync_agent_t *self)
{
    assert (self);
    return self->running;
}



// --------------------------------------------------------------------------
// Gets the current state

uint64_t
zsync_agent_current_state (zsync_agent_t *self)
{
    assert (self);
    return (*self->get_current_state)();
}

// --------------------------------------------------------------------------
// Passes a list of updated files

void zsync_agent_pass_update (zsync_agent_t *self, char *sender, zlist_t* fmetadata)
{
    assert (self);
    (*self->pass_update)(sender, fmetadata);
}

// --------------------------------------------------------------------------
// Passes chunk of a file

void
zsync_agent_pass_chunk (zsync_agent_t *self, byte *chunk, char *path, uint64_t sequence, uint64_t offset)
{
    assert (self);
    (*self->pass_chunk)(chunk, path, sequence, offset);
}
// --------------------------------------------------------------------------
// Gets a list of updates starting by the from state to the current state

zlist_t *
zsync_agent_update (zsync_agent_t *self, uint64_t from_state)
{
    assert (self);
    return (*self->get_update)(from_state);
}


// --------------------------------------------------------------------------
// Gets a chunk from client

byte *
zsync_agent_chunk (zsync_agent_t *self, char *path, uint64_t chunk_size, uint64_t offset) 
{
    assert (self);
    return (*self->get_chunk) (path, chunk_size, offset);
}


// --------------------------------------------------------------------------
// Pseudo test function

uint64_t 
test_get_current_state (uint64_t from_state) 
{
    printf("%"PRId64"\n", from_state);     
}
    
void
zsync_agent_test ()
{
    printf("selftest zsync_agent* \n");
    
    zsync_agent_t *agent = zsync_agent_new();
    
    zsync_agent_set_get_current_state (agent, test_get_current_state); 
    (*agent->get_current_state)(2);

    zsync_agent_destroy (&agent);
    printf("OK\n");
}

