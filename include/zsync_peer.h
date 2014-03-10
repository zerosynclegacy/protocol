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

#ifndef __ZSYNC_PEER_H_INCLUDED__
#define __ZSYNC_PEER_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

// Opaque class structure
typedef struct _zsync_peer_t zsync_peer_t;


// @interface

// Contructs, a new peer representation
zsync_peer_t *
    zsync_peer_new ();

// Destroys, a peer representation
void
    zsync_peer_destroy (zsync_peer_t **self);

// Returns the permanent uuid of this peer
char *
    zsync_peer_uuid (zsync_peer_t *self);

// Returns the last known upd state
uint64_t
    zsync_peer_state (zsync_peer_t *self);

// Sets the last known upd state
void
    zsync_peer_set_state (zsync_peer_t *self, uint64_t state);

// Sets the zyre connection state
void
    zsync_peer_set_zyre_state (zsync_peer_t *self, int zyre_state);

// Gets the zyre connection state
int
    zsync_peer_zyre_state (zsync_peer_t *self);
// @end

#ifdef __cplusplus
}
#endif

#endif
