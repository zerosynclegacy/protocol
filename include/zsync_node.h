/* =========================================================================
    zsync_node.h - node on ZeroSync network

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

#ifndef __ZSYNC_NODE_H_INCLUDED__
#define __ZSYNC_NODE_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

// Opaque class structure
typedef struct _zsync_node_t zsync_node_t;

// @interface
static zsync_node_t *
    zsync_node_new ();

void *
    zsync_node_engine (void *args);

int
    zsync_node_test ();
// @end

#ifdef __cplusplus
}
#endif

#endif
