/* =========================================================================
    zs_fmetadata - work with ZeroSync file meta data

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

#ifndef __ZSYNC_FMETADATA_H_INCLUDED__
#define __ZSYNC_FMETADATA_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

#define ZS_FILE_OP_UPD 0x1
#define ZS_FILE_OP_DEL 0x2
#define ZS_FILE_OP_REN 0x3

// Opaque class structure
typedef struct _zs_fmetadata_t zs_fmetadata_t;

// @interface
// Constructor, creates new zs file meta data
zs_fmetadata_t *
    zs_fmetadata_new ();

// Destructor, destroys file meta data
void
    zs_fmetadata_destroy (zs_fmetadata_t **self_p);

// getter/setter file path
void
    zs_fmetadata_set_path (zs_fmetadata_t *self, char* format, ...);

char *
    zs_fmetadata_path (zs_fmetadata_t *self);

// getter/setter renamed file path
void
    zs_fmetadata_set_renamed_path (zs_fmetadata_t *self, char* format, ...);

char *
    zs_fmetadata_renamed_path (zs_fmetadata_t *self);


// getter/setter file operation
void
    zs_fmetadata_set_operation(zs_fmetadata_t *self, int operation);
     
int
    zs_fmetadata_operation(zs_fmetadata_t *self);

// getter/setter file size
void
    zs_fmetadata_set_size (zs_fmetadata_t *self, uint64_t size);

uint64_t 
    zs_fmetadata_size (zs_fmetadata_t *self);

// getter/setter timestamp
void
    zs_fmetadata_set_timestamp (zs_fmetadata_t *self, uint64_t timestamp);

uint64_t
    zs_fmetadata_timestamp (zs_fmetadata_t *self);

// getter/setter checksum
void
    zs_fmetadata_set_checksum (zs_fmetadata_t *self, uint64_t checksum);

uint64_t
    zs_fmetadata_checksum (zs_fmetadata_t *self);

// Self test this class
int
    zs_fmetadata_test ();
// @end


#endif
