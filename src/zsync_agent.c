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
@end
*/

#include "zsync_classes.h"

struct _zsync_agent_t {
    // state
    // first param -> last state, that the partner has from us
    // second param -> filemetalist while initializing (if NULL -> NO-UPDATE
    // third param -> our current state 
    void (*pass_last_state)(uint64_t, zlist_t * ,uint64_t *);

    // chunk
    // first param -> chunk in bytes
    // second param -> sequence number
    // third param -> file path
    // fourth param -> offset
    void (*pass_chunk)(byte *, uint64_t, char *, uint64_t);

    // get_chunk
    // first param -> file path
    // second param -> offset
    byte* (*get_chunk)(char *, uint64_t);

    // get_last_state
    uint64_t (*get_last_state);
};


// ---------------------------------------------------------------------------
// Create a new zsync_agent

zsync_agent_t *
zsync_agent_new ()
{
    zsync_agent_t *self = (zsync_agent_t*) zmalloc (sizeof (zsync_agent_t));
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
        free(self);
    }
}

// --------------------------------------------------------------------------
// Set pass last state

void 
zsync_agent_set_pass_lstate (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->pass_last_state = ptr;
}

// --------------------------------------------------------------------------
// Set pass chunk

void
zsync_agent_set_pass_chunk (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->pass_chunk = ptr;
}

// --------------------------------------------------------------------------
// Set get chunk

void 
zsync_agent_set_get_chunk (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->get_chunk = ptr;
}

// --------------------------------------------------------------------------
// Set get last state

void
zsync_agent_set_get_lsate (zsync_agent_t *self, void *ptr)
{
    assert(self);
    self->get_last_state = ptr;
}

// --------------------------------------------------------------------------
// send_request_files for the protocol

void 
zsync_agent_send_request_files (void* out, zlist_t *list)
{
    // give the Request_files_command to the protocol
    if (zs_msg_pack_request_files (out, list) == 0) printf ("Requested files sent.\n;");

}

// --------------------------------------------------------------------------
// send_update for the protocol
void
zsync_agent_send_update (void *out, uint64_t state, zlist_t *list)
{
    // give the send_update_command to the protocol
    if (zs_msg_pack_update (out, state, list) == 0) printf ("Update sent.\n");
}

// --------------------------------------------------------------------------
// send_abort for the protocol
void
zsync_agent_send_abort(void *out, char* fileToAbort)
{
    // TODO: Implement the filePath to send_abort in protocol
    // give the send_abort_command to the protocol
    if (zs_msg_pack_abort (out) == 0) printf("Caution, file transfer aborted!!!\n");
}

// --------------------------------------------------------------------------
// Pseudo test function

void 
test_pass_last_state(uint64_t lstate, zlist_t *list, uint64_t *cstate)
{
    printf("%"PRId64"\n", lstate);     
}
    
int
zsync_agent_test (char *id)
{
    printf("selftest zsync_agent* \n");
    printf("testing pass_last_state...\n");
    
    zsync_agent_t *agent = zsync_agent_new();
    
    zsync_agent_set_pass_lstate(agent, test_pass_last_state); 
    (*agent->pass_last_state)(2, NULL, NULL);

    zsync_agent_destroy (&agent);
    printf("OK\n");
    return 0;
}
