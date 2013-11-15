/* =========================================================================
   zs_msg - work with zerosync messages

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

#ifndef __MSG_H_INCLUDED__
#define __MSG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

// Message commands 
#define ZS_CMD_LAST_STATE 0x1
#define ZS_CMD_FILE_LIST 0x2
#define ZS_CMD_GIVE_CREDIT 0x5
#define ZS_CMD_SEND_CHUNK 0x6 

// Opaque class structure
typedef struct _zs_msg_t zs_msg_t;
typedef struct _zs_filemeta_data_t zs_filemeta_data_t;

// Strings might change due to UTF encoding
typedef uint16_t string_size_t;

// --------------------------------------------------------------------------
// Create & Destroy

// create new zs message
zs_msg_t *
zs_msg_new (int cmd);

// destroy zs message
void
zs_msg_destroy (zs_msg_t **self_p);

// create new zs file meta data
zs_filemeta_data_t *
zs_filemeta_data_new ();

// destroy file meta data
void
zs_filemeta_data_destroy (zs_filemeta_data_t **self_p);

// --------------------------------------------------------------------------
// Receive & Send

// receive messages
zs_msg_t *
zs_msg_recv (void *input);

// send LAST_STATE
int 
zs_msg_send_last_state (void *output, uint64_t state);

 
// send FILE_LIST
int
zs_msg_send_file_list (void *output, zlist_t *filemeta_list);

// send GIVE CREDIT
int
zs_msg_send_give_credit (void *output, uint64_t credit);

// send SEND CHUNK
int
zs_msg_send_chunk (void *output, zframe_t chunk);

// --------------------------------------------------------------------------
// zs_msg_t get & set

// getter/setter message state    
void 
zs_msg_set_state (zs_msg_t *self, uint64_t state);

uint64_t 
zs_msg_get_state (zs_msg_t *self);

// getter/setter message file meta list
void 
zs_msg_set_file_meta (zs_msg_t *self, zlist_t *filemeta_list);

zlist_t *
zs_msg_get_file_meta (zs_msg_t *self);

// getter/setter message credit
void
zs_msg_set_credit (zs_msg_t *self, uint64_t credit);

uint64_t
zs_msg_get_credit (zs_msg_t *self);

// getter/setter message chunk
void
zs_msg_set_chunk (zs_msg_t *self, zframe_t *chunk);

zframe_t* 
zs_msg_get_chunk (zs_msg_t *self);


// --------------------------------------------------------------------------
// zs_filemeta_data_t get & set


// getter/setter file path
void
zs_filemeta_data_set_path (zs_filemeta_data_t *self, char* path);

char *
zs_filemeta_data_get_path (zs_filemeta_data_t *self);

// getter/setter file size
void
zs_filemeta_data_set_size (zs_filemeta_data_t *self, uint64_t size);

uint64_t 
zs_filemeta_data_get_size (zs_filemeta_data_t *self);

// getter/setter timestamp
void
zs_filemeta_data_set_timestamp (zs_filemeta_data_t *self, uint64_t timestamp);

uint64_t
zs_filemeta_data_get_timestamp (zs_filemeta_data_t *self);

#ifdef __cplusplus
}
#endif

#endif
