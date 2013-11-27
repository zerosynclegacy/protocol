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

struct _zsync_agent_t {
    
};

zsync_agent_t *
zsync_agent_new () 
{
    zsync_agent_t *self = (zsync_agent_t *) zmalloc (sizeof (zsync_agent_t));
    return self;
}

void 
zsync_agent_start () {
    
}

int
zsync_agent_test (char *id)
{
    printf("selftest zsync_agent* \n");

    // create contex & node
    zctx_t *ctx = zctx_new ();
    zyre_t *node = zyre_new (ctx);
   
    // set header, start node and join group
    zyre_set_header (node, "PROTOCOL", "%s", "1.0");
    zyre_set_header (node, "PEER_NAME", "%s", id);
    zyre_start (node);
    zyre_join (node, "zerosync");

    zmsg_t *msg = NULL;
    char *peer_name = NULL;
    char *peer = NULL;
    while (true) {
        msg = zyre_recv (node);
        char *cmd = zmsg_popstr (msg);
        peer = zmsg_popstr (msg);
        printf("Command: %s\n", cmd);
        printf("Peer: %s\n", peer);
        if (streq (cmd, "ENTER")) {
            zframe_t *headers_packed = zmsg_pop (msg);
            zhash_t *headers = zhash_unpack (headers_packed);
            printf("Protocol verison: %s\n", (char *) zhash_lookup (headers, "PROTOCOL"));
            peer_name = (char *) zhash_lookup (headers, "PEER_NAME");
            printf("Peer Name: %s\n", peer_name);
            zhash_destroy (&headers);
        }
        if (streq (cmd, "WHISPER")) {
            printf("Whisper: %s\n", zmsg_popstr (msg));
        }
        zmsg_destroy (&msg);
        if (streq (cmd, "JOIN")) {
            printf("joooooooiin\n");
            msg = zmsg_new ();
            zmsg_addstr (msg, peer);
            zmsg_addstr (msg, "%s", "Hello friend!");
            zyre_whisper (node, &msg);
            zmsg_destroy (&msg);
        }
        printf("-----------------------------------\n");
    }

    printf("OK\n");
    return 0;
}
