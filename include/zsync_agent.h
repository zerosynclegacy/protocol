/* =========================================================================
    zsync.h - public api for clients

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

#ifndef __ZSYNC_AGENT_H_INCLUDED__
#define __ZSYNC_AGENT_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

// Opaque class structure
typedef struct _zsync_t zsync_t;

// Create a new zsync
zsync_t *
    zsync_new ();

// Destroys a zsync
void
    zsync_destroy (zsync_t **agent);

// Starts the protocol execution
int 
    zsync_start (zsync_t *self);

// zsync stop 
void
    zsync_stop (zsync_t *self);

void 
    zsync_send_request_files (zsync_t *agent, char *sender, zlist_t *list, uint64_t total_bytes);

void 
    zsync_send_update (zsync_t *agent, uint64_t state, zlist_t *list);

void 
    zsync_send_abort (zsync_t *agent, char *sender, char *fileToAbort);

bool
    zsync_running (zsync_t *self);

// Selftest this class
void
    zsync_test();

#ifdef __cplusplus
}
#endif

#endif
