/* =========================================================================
    zsync.h - ZSYNC wrapper

   -------------------------------------------------------------------------
   Copyright (c) 2013 Kevin Sapper
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


#ifndef __ZSYNC_H_INCLUDED__
#define __ZSYNC_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  ZSYNC version macros for compile-time API detection

#define ZSYNC_VERSION_MAJOR 1
#define ZSYNC_VERSION_MINOR 0
#define ZSYNC_VERSION_PATCH 0

#define ZSYNC_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZSYNC_VERSION \
    ZSYNC_MAKE_VERSION(ZSYNC_VERSION_MAJOR, ZSYNC_VERSION_MINOR, ZSYNC_VERSION_PATCH)

//  Classes in the API

#include "zs_fmetadata.h"
#include "zs_msg.h"
#include "zsync_peer.h"
#include "zsync_ftmanager.h"
#include "zsync_credit.h"
#include "zsync_node.h"
#include "zsync_agent.h"

#ifdef __cplusplus
}
#endif

#endif
