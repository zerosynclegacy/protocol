/* =========================================================================
    zsync_peer - peer representation of ZeroSync

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
    ZeroSync peer

@discuss
@end
*/

#include "zsync_classes.h"

struct _zsync_peer_t {
    char *uuid;
    uint64_t state;
    bool connected;
    bool ready;
};


// --------------------------------------------------------------------------
// Contructs, a new peer representation
    
zsync_peer_t *
zsync_peer_new (char *uuid, uint64_t state)
{
    zsync_peer_t *self = (zsync_peer_t *) zmalloc (sizeof (zsync_peer_t));
    
    self->uuid = (char *) malloc (sizeof (char) * 32 + 1);
    strcpy (self->uuid, uuid);
    self->connected = false;
    self->ready = false;
    self->state = state;
    return self;
}


// --------------------------------------------------------------------------
// Destroys, a peer representation

void
zsync_peer_destroy (zsync_peer_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_peer_t *self = *self_p;
        free (self->uuid);
        
        free (self);
        self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Returns the permanent uuid of this peer

char *
zsync_peer_uuid (zsync_peer_t *self)
{
    assert (self);
    return self->uuid;
}

// --------------------------------------------------------------------------
// Returns the last known upd state

uint64_t
zsync_peer_state (zsync_peer_t *self)
{
    assert (self);
    return self->state;
}

// --------------------------------------------------------------------------
// Sets the upd state value

void
zsync_peer_set_state (zsync_peer_t *self, uint64_t state)
{
    assert (self);
    self->state = state;
}

// --------------------------------------------------------------------------
// Sets the connected flag

void
zsync_peer_set_connected (zsync_peer_t *self, bool connected)
{
    assert (self);
    self->connected = connected;
}

// --------------------------------------------------------------------------
// Sets the ready flag

void
zsync_peer_set_ready (zsync_peer_t *self, bool ready)
{
    assert (self);
    self->ready = ready;
}


