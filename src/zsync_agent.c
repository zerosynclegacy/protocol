/* =========================================================================
    zsync - ZeroSync public API

   -------------------------------------------------------------------------
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

/*
@header
    zsync

    This class defines to pulic API for user interface clients. 
    The bnf to communicate with the ZeroSync protocol is defined in zsync_msg.
@discuss
    * Add method to ask for current file transfer lsit
@end
*/

#include "zsync_classes.h"

struct _zsync_t {
    //  Zeromq context
    zctx_t *ctx;

    //  Pipe to node thread
    void *pipe;

    bool running;
};


// ---------------------------------------------------------------------------
// Create a new zsync_agent

zsync_t *
zsync_new ()
{
    zsync_t *self = (zsync_t*) zmalloc (sizeof (zsync_t));
    
    self->ctx = zctx_new ();
    assert (self->ctx);
    self->running = false;
    
    return self;
}

// --------------------------------------------------------------------------
// Destroys a zsync_agent

void
zsync_destroy (zsync_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zsync_t *self = *self_p;
        
        // destroy context
        zctx_destroy (&self->ctx);
    }
}

// --------------------------------------------------------------------------
// Starts the zsync protocol

int
zsync_start (zsync_t *self)
{
    assert (self);
    self->pipe = zthread_fork (self->ctx, zsync_node_engine, NULL);
    self->running = true;
    return 0;
}


// --------------------------------------------------------------------------
// zsync stop, failed if a function ptr isn´t set

void
zsync_stop (zsync_t *self)
{
    assert (self);
    // send terminate to node
    zsync_msg_send_terminate (self->pipe);
    // receive terminate confirmation
    zsync_msg_t *msg = zsync_msg_recv (self->pipe);
    zsync_msg_destroy (&msg);
    self->running = false;
}

// --------------------------------------------------------------------------
// send_request_files for the protocol

void 
zsync_send_request_files (zsync_t *self, char *receiver, zlist_t *files, uint64_t size)
{
    assert (self);
    int rc = zsync_msg_send_req_files (self->pipe, receiver, files, size);   
    assert (rc == 0);
}

// --------------------------------------------------------------------------
// send_update for the protocol

void
zsync_send_update (zsync_t *self, uint64_t state, zlist_t *list)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();
    // give the send_update_command to the protocol
    if (zs_msg_pack_update (msg, state, list) == 0) {
        int rc = zsync_msg_send_update (self->pipe, "", msg);
        assert (rc == 0);
        printf ("[AG] Update sent.\n");    
    }
}

// --------------------------------------------------------------------------
// Sends ABORT message to the protocol

void
zsync_send_abort (zsync_t *self, char *receiver, char* fileToAbort)
{
    assert (self);
    int rc = zsync_msg_send_abort (self->pipe, receiver, fileToAbort);
    assert (rc == 0);
    printf("[AG] Caution, file transfer aborted!!!\n");
}

// --------------------------------------------------------------------------
// Returns whether the agents has been started or not

bool
zsync_running (zsync_t *self)
{
    assert (self);
    return self->running;
}

void
zsync_test ()
{
    printf(" * zsync_agent: ");
    
    zsync_t *zsync = zsync_new();

    zsync_destroy (&zsync);

    printf("OK\n");
}

