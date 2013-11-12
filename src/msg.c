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

#include <zmq.h>
#include <czmq.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "../include/msg.h"

struct _zs_msg_t {
    uint16_t signature;     // zs_msg signature
    int cmd;                // zs_msg command
    byte *needle;           // read/write pointer for serialization
    byte *ceiling;          // valid upper limit for needle
    uint64_t state;
    zlist_t *filemeta_list;  // zlist of file meta data list
};

struct _zs_filemeta_data_t {
    char * path;            // file path + file name
    uint64_t size;          // file size in bytes
    uint64_t timestamp;     // UNIX timestamp
    uint64_t checksum;      // SHA-3 512
};

// --------------------------------------------------------------------------
// Network data encoding macros

// Strings are encoded with 2-byte length
#define STRING_MAX 65535 // 2-byte - 1-bit for trailing \0

// Put a block to the frame
#define PUT_BLOCK(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

// Get a block from the frame
#define GET_BLOCK(host,size) { \
    /*if (self->needle + size > self->ceiling) \
    goto malformed; */ \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

// Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

// Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8) & 255); \
    self->needle [1] = (byte) (((host)) & 255); \
    self->needle += 2; \
}

// Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8) & 255); \
    self->needle [7] = (byte) (((host)) & 255); \
    self->needle += 8; \
}

// Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    /*if (self->needle + 1 > self->ceiling) \
    goto malformed;*/ \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

// Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    /*if (self->needle + 2 > self->ceiling) \
    goto malformed;*/ \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
    + (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

// Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    /*if (self->needle + 8 > self->ceiling) \
    goto malformed;*/ \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
    + ((uint64_t) (self->needle [1]) << 48) \
    + ((uint64_t) (self->needle [2]) << 40) \
    + ((uint64_t) (self->needle [3]) << 32) \
    + ((uint64_t) (self->needle [4]) << 24) \
    + ((uint64_t) (self->needle [5]) << 16) \
    + ((uint64_t) (self->needle [6]) << 8) \
    + (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

// Put a string to the frame
#define PUT_STRING(host) { \
    string_size = strlen (host); \
    PUT_NUMBER2 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

// Get a string from the frame
#define GET_STRING(host) { \
    GET_NUMBER2 (string_size); \
    /*if (self->needle + string_size > (self->ceiling)) \
    goto malformed; */ \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

// --------------------------------------------------------------------------
// Create a new zs_msg

zs_msg_t * 
zs_msg_new (int cmd) 
{
    zs_msg_t *self = (zs_msg_t *) zmalloc (sizeof (zs_msg_t));
    self->cmd = cmd;
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zs_msg

void 
zs_msg_destroy (zs_msg_t **self_p) 
{
    assert (self_p);
    if (*self_p) {
        zs_msg_t *self = *self_p;

        // Free object itself
        free (self);
        *self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Create a new zs_filemeta_data

zs_filemeta_data_t * 
zs_filemeta_data_new () 
{
    zs_filemeta_data_t *self = (zs_filemeta_data_t *) zmalloc (sizeof (zs_filemeta_data_t));
    self->path = (char *) malloc (STRING_MAX + 1);
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zs_msg

void 
zs_filemeta_data_destroy (zs_filemeta_data_t **self_p) 
{
    assert (self_p);
    if (*self_p) {
        zs_filemeta_data_t *self = *self_p;
        
        free(self->path);
        // Free object itself
        free (self);
        *self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Receive and parse a fmq_msg from the socket. Returns new object or
// NULL if error. Will block if there's no message waiting.

zs_msg_t *
zs_msg_recv (void *input) 
{
    zs_msg_t *self = zs_msg_new (0);
    uint64_t list_size;
    string_size_t string_size;

    // receive data
    zframe_t *rframe = NULL;
    rframe = zframe_recv (input);
    self->needle = zframe_data(rframe);
    
    // parse message
    GET_NUMBER2(self->signature);
    GET_NUMBER1(self->cmd);
  
    // silently ignore everything with wrong signature     
    if (self->signature == SIGNATURE) { 
        switch (self->cmd) {
            case ZS_CMD_LAST_STATE:
                self->ceiling = self->needle + 11;
                GET_NUMBER8(self->state);
                break;
            case ZS_CMD_FILE_LIST:
                self->ceiling = self->needle + 24 + 8;
                // file meta data count
                GET_NUMBER8(list_size);

                self->filemeta_list = zlist_new ();
                while (list_size--) {
                    zs_filemeta_data_t *filemeta_data = zs_filemeta_data_new ();
                    GET_STRING (filemeta_data->path);
                    GET_NUMBER8 (filemeta_data->size);
                    GET_NUMBER8 (filemeta_data->timestamp);

                    zlist_append (self->filemeta_list, filemeta_data); 
                }
                break;
            default:
                break;
        }
    }

    // cleanup
    zframe_destroy (&rframe);
    return self;
}


// --------------------------------------------------------------------------
// Send the zs_msg to the socket, and destroy it
// Returns 0 if OK, else -1

int 
zs_msg_send (zs_msg_t **self_p, void *output, size_t frame_size)
{
    assert (output);
    assert (self_p);
    assert (*self_p);

    zs_msg_t *self = *self_p; 
    string_size_t string_size;
   
    // calculate frame size
    frame_size = frame_size + 2 + 1;          //  Signature and command ID
    zframe_t *data_frame = zframe_new (NULL, frame_size);
    
    self->needle = zframe_data (data_frame);  // Address of frame data
    self->ceiling = self->needle + frame_size;
    
    /* Add data to frame */
    PUT_NUMBER2 (SIGNATURE);
    PUT_NUMBER1 (self->cmd);

    switch(self->cmd) {
        case ZS_CMD_LAST_STATE:
            PUT_NUMBER8 (self->state);
            break;
        case ZS_CMD_FILE_LIST:
            // put trailing size of list
            PUT_NUMBER8 (zlist_size (self->filemeta_list));
            // get first element from list
            zs_filemeta_data_t *filemeta_data = (zs_filemeta_data_t *) zlist_first (self->filemeta_list);
            while (filemeta_data) {
                PUT_STRING (filemeta_data->path);
                PUT_NUMBER8 (filemeta_data->size);
                PUT_NUMBER8 (filemeta_data->timestamp);
                // next list entry
                filemeta_data = (zs_filemeta_data_t *) zlist_next (self->filemeta_list);
            }
            break;
        default:
            break;
    }
    
    /* Send data frame */
    if (zframe_send (&data_frame, output, 0)) {
        zframe_destroy (&data_frame);
    }
    
    /* Cleanup */
    zs_msg_destroy (&self);

    return 0;
}

// --------------------------------------------------------------------------
// Send the LAST_STATE to the socket in one step

int 
zs_msg_send_last_state (void *output, uint64_t state) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_LAST_STATE);
    zs_msg_set_state (self, state);
    size_t frame_size = 8; // 8-byte state
    return zs_msg_send (&self, output, 8);
}

// --------------------------------------------------------------------------
// Send the FILE-LIST to the socket in one step

int
zs_msg_send_file_list (void *output, zlist_t *filemeta_list) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_FILE_LIST);   
    zs_msg_set_file_meta (self, filemeta_list);

    size_t frame_size = 8 + // 8-byte list size
                        (zlist_size(filemeta_list) *
                          (sizeof(string_size_t) + // string size
                           5 + // string length @TODO make variable
                           8 + // 8-byte file size
                           8)  // 8-byte time stamp
                        );
    return zs_msg_send (&self, output, frame_size); 
}


// --------------------------------------------------------------------------
// Get/Set the state field

void 
zs_msg_set_state (zs_msg_t *self, uint64_t state) 
{
    assert (self);
    self->state = state;
}

uint64_t 
zs_msg_get_state (zs_msg_t *self) 
{
    assert (self);
    return self->state;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data list

void 
zs_msg_set_file_meta (zs_msg_t *self, zlist_t *filemeta_list) 
{
    assert (self);
    self->filemeta_list = filemeta_list;
}

zlist_t *
zs_msg_get_file_meta (zs_msg_t *self) 
{
    assert (self);
    return self->filemeta_list;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data path

void
zs_filemeta_data_set_path (zs_filemeta_data_t *self, char* path) 
{
    assert (self);    
    
    self->path = malloc(strlen(path) * sizeof(char));
    strcpy(self->path, path);
}

char *
zs_filemeta_data_get_path (zs_filemeta_data_t *self)
{
    assert (self);
    
    char *path = malloc(strlen(self->path) * sizeof(char));
    strcpy(path, self->path);
    return path;
}    


// --------------------------------------------------------------------------
// Get/Set the file meta data size

void
zs_filemeta_data_set_size (zs_filemeta_data_t *self, uint64_t size) 
{
    assert (self);
    self->size = size;
}    

uint64_t 
zs_filemeta_data_get_size (zs_filemeta_data_t *self) 
{
    assert (self);
    return self->size;
}    


// --------------------------------------------------------------------------
// Get/Set the file meta data timestamp

void
zs_filemeta_data_set_timestamp (zs_filemeta_data_t *self, uint64_t timestamp)
{
    assert (self);
    self->timestamp = timestamp;
}

uint64_t 
zs_filemeta_data_get_timestamp (zs_filemeta_data_t *self) {
    assert (self);
    return self->timestamp;
}

