/* =========================================================================
    zsync_node - node on ZeroSync network

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
    ZeroSync node

    A node is one instance on the ZeroSync network
@discuss
@end
*/

#include "zsync_classes.h"

#define UUID_FILE ".zsync_uuid"
#define UUID_BIN_SIZE 16

struct _zsync_node_t {
    zctx_t *ctx;
    zyre_t *zyre;       // zyre main struct
    zuuid_t *uuid;      // uuid of this node
};

static zsync_node_t *
zsync_node_new ()
{
    int rc;
    zsync_node_t *self = (zsync_node_t *) zmalloc (sizeof (zsync_node_t));
   
    self->ctx = zctx_new ();
    self->zyre = zyre_new (self->ctx);  

    self->uuid = zuuid_new ();
    if (zsys_file_exists (UUID_FILE)) {
        // Read uuid from file
        zfile_t *uuid_file = zfile_new (".", UUID_FILE);
        int rc = zfile_input (uuid_file);    // open file for reading        
        assert (rc == 0); 

        zchunk_t *uuid_chunk = zfile_read (uuid_file, 16, 0);
        assert (zchunk_size (uuid_chunk) == 16);    // make sure read succeeded
        zuuid_set (self->uuid, zchunk_data (uuid_chunk));
    } else {
        // Write uuid to file
        zfile_t *uuid_file = zfile_new (".", ".zsync_uuid");
        rc = zfile_output (uuid_file); // open file for writing
        assert (rc == 0);
        zchunk_t *uuid_bin = zchunk_new ( zuuid_data (self->uuid), 16);
        rc = zfile_write (uuid_file, uuid_bin, 0);
        assert (rc == 0);
    }

    return self;
}

static void
zsync_node_destroy (zsync_node_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        zsync_node_t *self = *self_p;
        
        zuuid_destroy (&self->uuid);
        zyre_destroy (&self->zyre);
        zctx_destroy (&self->ctx);
    }
}

void
zsync_node_engine ()
{
    zsync_node_t *self = zsync_node_new ();

    // SET uuid the be send with header
    zyre_set_header (self->zyre, "UUID", "%s", zuuid_str (self->uuid));

    // Start Zyre
    zyre_start (self->zyre);

    // start recving messages
    while (true) {
        zyre_msg_t *msg = zyre_msg_recv (self->zyre);
        
        switch (zyre_msg_cmd (msg)) {
            case ZYRE_MSG_ENTER:
                printf ("ENTER: %s\n", zyre_msg_get_header (msg, "UUID"));
                break;        
            case ZYRE_MSG_JOIN:
                printf ("JOIN\n");
                break;
            default:
                break;
        }
        break;
    }

    zsync_node_destroy (&self);
}

int
zsync_node_test () 
{
    zsync_node_engine ();
}

