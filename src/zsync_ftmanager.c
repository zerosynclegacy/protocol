/* =========================================================================
    zsync_ftmanager - File Transfer manager

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
    ZeroSync file transfer manager
    Holds a list of outstanding file transfers and credit for each peer.
    Automatically sends chunks of data according the open file transfers.

    A node is one instance on the ZeroSync network
@discuss
    LOGGER into zyre_event.
    LOG message to LOG group on whisper and shout
@end
*/

#include "zsync_classes.h"

struct _zsync_ftrequest_t {
    zlist_t *requested_files;
    uint64_t credit;
};

typedef struct _zsync_ftrequest_t zsync_ftrequest_t;

zsync_ftrequest_t *
zsync_ftrequest_new ()
{
    zsync_ftrequest_t *self = (zsync_ftrequest_t *) zmalloc (sizeof (zsync_ftrequest_t));
    return self;
}

void
zsync_ftrequest_destroy (zsync_ftrequest_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_ftrequest_t *self = *self_p;
        zlist_destroy (&self->requested_files);

        free (self);
        self_p = NULL;
    }
}

void
zsync_ftmanager_engine (void *args, zctx_t *ctx, void *pipe)
{
    zhash_t *peer_requests = zhash_new ();

    while (true) {
        // TODO
        // Poll from pipe, until empty then proceed
        //   Append to files to requested files
        //   Manage Credit
        //   Abort requested files
        // Get requested file's chunks from Qt-Client
        // Send chunks according if there's credit else skip
        //   In case of no credit at all wait even on empty pipe
        // repeat after sending one chunk, so abort messages can be taken into account
    }

    zhash_destroy (&peer_requests);
}

