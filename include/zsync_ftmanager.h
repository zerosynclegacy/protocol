/* =========================================================================
    zsync_ftmanager.h - File transfer manager

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

#ifndef __ZSYNC_FTMANAGER_H_INCLUDED__
#define __ZSYNC_FTMANAGER_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

#define CHUNK_SIZE 30000

// Start the file transfer manager as thread and provide a pipe to comminicate
// back and forth. The file transfer manager expects a zsync_agent as arguments.
void
    zsync_ftmanager_engine (void *args, zctx_t *ctx, void *pipe);

// Selftest
void
    zsync_ftmanager_test ();

#ifdef __cplusplus
}
#endif

#endif

