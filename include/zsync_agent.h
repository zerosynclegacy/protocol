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

int 
    zsync_agent_start (zsync_agent_t *self);

void 
    zsync_agent_send_request_files (zsync_agent_t *agent, zlist_t *list);

void 
    zsync_agent_send_update (zsync_agent_t *agent, uint64_t state, zlist_t *list);

void 
    zsync_agent_send_abort (zsync_agent_t *agent, char *fileToAbort);

void 
    zsync_agent_set_pass_lstate (zsync_agent_t *self, void *ptr);

void 
    zsync_agent_set_pass_chunk (zsync_agent_t *self, void *ptr);

void 
    zsync_agent_set_get_chunk (zsync_agent_t *self, void *ptr);

void 
    zsync_agent_set_get_lstate (zsync_agent_t *self, void *ptr);


#ifdef __cplusplus
}
#endif

#endif
