/* =========================================================================
    zs_msg - work with ZeroSync messages

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

#ifndef __ZS_MSG_H_INCLUDED__
#define __ZS_MSG_H_INCLUDED__
 
#ifdef __cplusplus
extern "C" {
#endif

// Message commands 
#define ZS_CMD_GREET 0x1
#define ZS_CMD_LAST_STATE 0x2
#define ZS_CMD_UPDATE 0x3
#define ZS_CMD_REQUEST_FILES 0x4
#define ZS_CMD_GIVE_CREDIT 0x5
#define ZS_CMD_SEND_CHUNK 0x6 
#define ZS_CMD_ABORT 0x7

// Opaque class structure
typedef struct _zs_msg_t zs_msg_t;

// Strings might change due to UTF encoding
typedef uint16_t string_size_t;

// @interface
// --------------------------------------------------------------------------
// Construct & Destroy

// Contructor, creates new zs message
zs_msg_t *
    zs_msg_new (int cmd);

// Destructor, destroys zs message
void
    zs_msg_destroy (zs_msg_t **self_p);

// receive messages
zs_msg_t *
    zs_msg_unpack (zmsg_t *input);

// pack GREET
int 
    zs_msg_pack_greet (zmsg_t *output, byte *uuid,  uint64_t state);
 
// pack LAST_STATE
int 
    zs_msg_pack_last_state (zmsg_t *output, uint64_t last_state);
 
// pack FILE_LIST
int
    zs_msg_pack_update (zmsg_t *output, uint64_t state, zlist_t *filemeta_list);
 
// pack REQUEST_FILES
int
    zs_msg_pack_request_files (zmsg_t *output, zlist_t *fpaths);

// pack GIVE CREDIT
int
    zs_msg_pack_give_credit (zmsg_t *output, uint64_t credit);

// pack SEND CHUNK
int
    zs_msg_pack_chunk (zmsg_t *output, uint64_t sequence, char *file_path, uint64_t offset, zframe_t *chunk);

// pack NO_UPDATE
int
    zs_msg_pack_abort (zmsg_t *output);
 

// --------------------------------------------------------------------------
// zs_msg_t get & set

// getter/setter message command    
int
    zs_msg_get_cmd (zs_msg_t *self);

// getter/setter message uuid
void
    zs_msg_set_uuid (zs_msg_t *self, byte *uuid);

byte *
    zs_msg_uuid (zs_msg_t *self);

// getter/setter message state    
void
    zs_msg_set_state (zs_msg_t *self, uint64_t state);

uint64_t
    zs_msg_get_state (zs_msg_t *self);

// getter/setter message file meta list
void
    zs_msg_set_fmetadata (zs_msg_t *self, zlist_t *filemeta_list);

zlist_t *
    zs_msg_get_fmetadata (zs_msg_t *self);

// Iterate file meta data

zs_fmetadata_t *
    zs_msg_fmetadata_first (zs_msg_t *self);

zs_fmetadata_t *
    zs_msg_fmetadata_next (zs_msg_t *self);

void
    zs_msg_fmetadata_append (zs_msg_t *self, zs_fmetadata_t *fmetadata_item);

// getter/setter message file paths

void
    zs_msg_set_fpaths (zs_msg_t *self, zlist_t *fpaths);

zlist_t *
    zs_msg_fpaths (zs_msg_t *self);

// Iterate fpaths

char *
    zs_msg_fpaths_first (zs_msg_t *self);

char *
    zs_msg_fpaths_next (zs_msg_t *self);

void
    zs_msg_fpaths_append (zs_msg_t *self, char *format, ...);

// getter/setter message credit
void
    zs_msg_set_credit (zs_msg_t *self, uint64_t credit);

uint64_t
    zs_msg_get_credit (zs_msg_t *self);

// getter/setter message chunk
void
    zs_msg_set_chunk (zs_msg_t *self, zframe_t *chunk);

zframe_t *
    zs_msg_get_chunk (zs_msg_t *self);

// getter/setter message sequence
void
    zs_msg_set_sequence (zs_msg_t *self, uint64_t seq);

uint64_t
    zs_msg_get_sequence (zs_msg_t *self);

// getter/setter message file-path
void
    zs_msg_set_file_path (zs_msg_t *self, char *format, ...);

char *
    zs_msg_get_file_path (zs_msg_t *self);

// getter/setter message offset
void
    zs_msg_set_offset (zs_msg_t *self, uint64_t offset);

uint64_t
    zs_msg_get_offset (zs_msg_t *self);

int
    zs_msg_test ();
// @end

#ifdef __cplusplus
}
#endif

#endif
