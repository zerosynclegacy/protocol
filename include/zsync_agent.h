/* =========================================================================
    zsync_agent.h - public api for clients

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

#ifndef __ZSYNC_AGENT_H_INCLUDED__
#define __ZSYNC_AGENT_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

// Opaque class structure
typedef struct _zsync_agent_t zsync_agent_t;

// Create a new zsync_agent
zsync_agent_t *
    zsync_agent_new ();

// Destroys a zsync_agent
void
    zsync_agent_destroy (zsync_agent_t **agent);

// Starts the protocol execution, returns -1 if a function ptr isn´t set else 0
int 
    zsync_agent_start (zsync_agent_t *self);

// zsync_agent stop, failed if a function ptr isn´t set
void
    zsync_agent_stop (zsync_agent_t *self);

void 
    zsync_agent_send_request_files (zsync_agent_t *agent, char *sender, zlist_t *list);

void 
    zsync_agent_send_update (zsync_agent_t *agent, uint64_t state, zlist_t *list);

void 
    zsync_agent_send_abort (zsync_agent_t *agent, char *sender, char *fileToAbort);

// Sets the pass_update callback method
void 
    zsync_agent_set_pass_update (zsync_agent_t *self, void *ptr);

// Sets the pass_chunk callback method
void 
    zsync_agent_set_pass_chunk (zsync_agent_t *self, void *ptr);

// Sets the get_update callback method
void 
    zsync_agent_set_get_update (zsync_agent_t *self, void *ptr);

// Sets the get_current_state callback method
void 
    zsync_agent_set_get_current_state (zsync_agent_t *self, void *ptr);

// Sets the get_chunk callback method
void 
    zsync_agent_set_get_chunk (zsync_agent_t *self, void *ptr);

// Gets the zeromq context
zctx_t *
    zsync_agent_ctx (zsync_agent_t *self);

// Gets the Zyre object
zyre_t *
    zsync_agent_zyre (zsync_agent_t *self);

// Passes a list with updated files
void 
    zsync_agent_pass_update (zsync_agent_t *self, char *sender, zlist_t* fmetadata);

// Passes chunk of a file
void
    zsync_agent_pass_chunk (zsync_agent_t *self, byte *chunk, char *path, uint64_t sequence, uint64_t offset);

// Gets the current state
uint64_t
    zsync_agent_current_state (zsync_agent_t *self);

// Gets a list of updates starting by the from state to the current state
zlist_t *
    zsync_agent_update (zsync_agent_t *self, uint64_t from_state);

// Gets a list of updates starting by the from state to the current state
byte *
    zsync_agent_chunk (zsync_agent_t *self, char *path, uint64_t chunk_size, uint64_t offset);

// Returns whether the agents has been started or not
bool
    zsync_agent_running (zsync_agent_t *self);

// Returns the channel the agent joins
char *
    zsync_agent_channel (zsync_agent_t *self);

// Selftest this class
void
    zsync_agent_test();

#ifdef __cplusplus
}
#endif

#endif
