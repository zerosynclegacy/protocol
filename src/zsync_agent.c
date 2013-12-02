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

/* TODO: header ausimplementieren / setter fertig machen
*/
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

void 
test_pass_last_state(uint64_t lstate, zlist_t *list, uint64_t *cstate)
{
    printf("%"PRId64"\n", lstate);     
}
    
int
zsync_agent_test (char *id)
{
    printf("selftest zsync_agent* \n");
    printf("testing pass_last_state\n...");
    
    zsync_agent_t *agent = zsync_agent_new();
    
    zsync_agent_set_pass_lstate(agent, test_pass_last_state); 
    (*agent->pass_last_state)(2, NULL, NULL);

    zsync_agent_destroy (&agent);
    printf("OK\n");
    return 0;
}
