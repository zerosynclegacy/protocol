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

    printf("OK\n");
    return 0;
}
