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

/*
@header
    ZeroSync messages module

    When sending messages the ownership of passed arguments is transfered 
    to the message module. Any attempt to free a passed argument may result
    in an severe error.

    When receiving messages the zs_msg_t struct must be destroy by it's destroy
    method zs_msg_destroy. Any attempt to free parts of the zs_msg_t struct may
    result in an severe error.
@discuss
@end
*/

#include "zsync_classes.h"

struct _zs_msg_t {
    uint16_t signature;     // zs_msg signature
    int cmd;                // zs_msg command
    byte *needle;           // read/write pointer for serialization
    byte *ceiling;          // valid upper limit for needle
    byte *uuid;             // own 16-byte uuid
    uint64_t state;
    uint64_t sequence;
    uint64_t offset;
    char *file_path;
    zframe_t *chunk;
    zlist_t *fmetadata;     // zlist of file meta data list
    zlist_t *fpaths;        // zlist of file paths
    uint64_t credit;        // given credit for RP 
};

// ZeroSync Sigature
#define SIGNATURE 0x5A53

// --------------------------------------------------------------------------
// Network data encoding macros

// Put a block to the frame
#define PUT_BLOCK(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

// Get a block from the frame
#define GET_BLOCK(host,size) { \
    if (self->needle + size > self->ceiling) \
      goto malformed; \
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
    if (self->needle + 1 > self->ceiling) \
      goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

// Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
      goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
    + (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

// Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
      goto malformed; \
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
    if (self->needle + string_size > (self->ceiling)) \
      goto malformed; \
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
    self->uuid = (byte *) zmalloc (sizeof (byte) * 16);
    self->file_path = NULL;
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zs_msg

void
zs_msg_fmetadata_destroy (zs_msg_t **self_p) 
{
    assert (self_p);
    
    if (*self_p) {
        zs_msg_t *self = *self_p;
        if (self->fmetadata) {
            zs_fmetadata_t *fmetadata_item = zs_msg_fmetadata_first (self);
            while (fmetadata_item) {
                zs_fmetadata_destroy (&fmetadata_item);
                // next list entry
                fmetadata_item = zs_msg_fmetadata_next (self);
            }
            zlist_destroy (&self->fmetadata);
        }
    }
}
    
void 
zs_msg_destroy (zs_msg_t **self_p) 
{
    assert (self_p);
    
    if (*self_p) {
        // Free struct properties
        zs_msg_t *self = *self_p;
        zs_msg_fmetadata_destroy (&self);    
        if (self->fpaths) {
            zlist_destroy (&self->fpaths); 
        }
        zframe_destroy(&self->chunk);
    
        // Free object itself
        free (self);
        *self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Receive and parse a zs_msg from the socket. Returns new object or
// NULL if error. Will block if there's no message waiting.

zs_msg_t *
zs_msg_unpack (zmsg_t *input) 
{
    assert (input);
    zs_msg_t *self = zs_msg_new (0);
    zframe_t *frame = NULL;
    uint64_t list_size;
    string_size_t string_size;

    // Receive data frame
    frame = zmsg_pop (input);
    if (!frame) {
        goto empty;
    }
    // Set pointer to data begin and end
    self->needle = zframe_data(frame);
    self->ceiling = self->needle + zframe_size (frame);
    
    // Get and check protocol signature
    // silently ignore everything with wrong signature     
    GET_NUMBER2(self->signature);
    if (self->signature == SIGNATURE) {

        // Get the message command and parse accordingly
        GET_NUMBER1(self->cmd);
        switch (self->cmd) {
            case ZS_CMD_GREET:
                GET_BLOCK (self->uuid, 16);
                GET_NUMBER8(self->state);
                break;
            case ZS_CMD_LAST_STATE:
                GET_NUMBER8 (self->state);
                break;
            case ZS_CMD_UPDATE:
                GET_NUMBER8(self->state);
                char *path, *path_renamed;
                int operation;
                uint64_t timestamp, size, checksum;
                // file meta data count
                GET_NUMBER8(list_size);
                while (list_size--) {
                    zs_fmetadata_t *fmetadata_item = zs_fmetadata_new ();
                    GET_STRING (path);
                    zs_fmetadata_set_path (fmetadata_item, "%s", path);
                    GET_NUMBER1 (operation);
                    zs_fmetadata_set_operation (fmetadata_item, operation);
                    GET_NUMBER8 (timestamp);
                    zs_fmetadata_set_timestamp (fmetadata_item, timestamp);
                    switch (zs_fmetadata_operation (fmetadata_item)) {
                        case ZS_FILE_OP_UPD:
                            GET_NUMBER8 (size);
                            zs_fmetadata_set_size (fmetadata_item, size);
                            GET_NUMBER8 (checksum);
                            zs_fmetadata_set_checksum (fmetadata_item, checksum);
                            break;
                        case ZS_FILE_OP_DEL:
                            // noting to do here
                            break;
                        case ZS_FILE_OP_REN:
                            //
                            GET_STRING (path_renamed);
                            zs_fmetadata_set_renamed_path (fmetadata_item, "%s", path_renamed);
                            break;
                        default:
                            goto malformed;
                    }
                    zs_msg_fmetadata_append (self, fmetadata_item);
                }
                break;
            case ZS_CMD_REQUEST_FILES:
                GET_NUMBER8(list_size);
                while (list_size--) {
                    char *path;
                    GET_STRING(path);
                    zs_msg_fpaths_append (self, "%s", path); 
                }
                break;
            case ZS_CMD_GIVE_CREDIT:
                GET_NUMBER8(self->credit);      
                break;
            case ZS_CMD_SEND_CHUNK:
                GET_NUMBER8 (self->sequence);
                GET_STRING (self->file_path);
                GET_NUMBER8 (self->offset);
                self->chunk = zmsg_pop (input);
                break;
            case ZS_CMD_ABORT:
                // noting to get
                break;
            default:
                goto malformed;
        }
    }

    // cleanup
    zframe_destroy (&frame);
    return self;
    
    // Error handling
    malformed: 
        printf ("[ERROR] malformed message '%d'\n", self->cmd);            
    empty:
        zframe_destroy (&frame);
        zs_msg_destroy (&self);
        return (NULL);
}


// --------------------------------------------------------------------------
// Send the zs_msg to the socket, and destroy it
// Returns 0 if OK, else -1

int 
zs_msg_pack (zs_msg_t **self_p, zmsg_t *output, size_t frame_size)
{
    assert (output);
    assert (self_p);
    assert (*self_p);
   
    zs_msg_t *self = *self_p; 
    string_size_t string_size;
    int frame_flags = 0;
   
    // frame size appendix
    frame_size += frame_size + 2 + 1;          //  Signature and command ID
    zframe_t *data_frame = zframe_new (NULL, frame_size);
    
    self->needle = zframe_data (data_frame);  // Address of frame data
    self->ceiling = self->needle + frame_size;
    
    /* Add data to frame */
    PUT_NUMBER2 (SIGNATURE);
    PUT_NUMBER1 (self->cmd);

    switch(self->cmd) {
        case ZS_CMD_GREET:
            PUT_BLOCK (self->uuid, 16);
            PUT_NUMBER8 (self->state);
            break;
        case ZS_CMD_LAST_STATE:
            PUT_NUMBER8 (self->state);
            break;
        case ZS_CMD_UPDATE:
            PUT_NUMBER8 (self->state);
            // put trailing size of list
            PUT_NUMBER8 (zlist_size (self->fmetadata));
            // get first element from list
            zs_fmetadata_t *fmetadata_item = zs_msg_fmetadata_first (self);
            while (fmetadata_item) {
                PUT_STRING (zs_fmetadata_path (fmetadata_item));
                PUT_NUMBER1 (zs_fmetadata_operation (fmetadata_item));
                PUT_NUMBER8 (zs_fmetadata_timestamp (fmetadata_item));
                switch (zs_fmetadata_operation (fmetadata_item)) {
                    case ZS_FILE_OP_UPD:
                        PUT_NUMBER8 (zs_fmetadata_size (fmetadata_item));
                        PUT_NUMBER8 (zs_fmetadata_checksum (fmetadata_item));
                        break;
                    case ZS_FILE_OP_DEL:
                        // noting to do here
                        break;
                    case ZS_FILE_OP_REN:
                        PUT_STRING (zs_fmetadata_renamed_path (fmetadata_item));
                        break;
                    default:
                        goto malformed;
                }
                // cleanup & next list entry
                fmetadata_item = zs_msg_fmetadata_next (self);
            }
            break;
        case ZS_CMD_REQUEST_FILES:
            // put trailing size of list
            PUT_NUMBER8 (zlist_size (self->fpaths));
            // get first element from list
            char *path = zs_msg_fpaths_first (self);
            while (path) {
                PUT_STRING (path);
                // next element
                path = zs_msg_fpaths_next (self);
            }
            break;
        case ZS_CMD_GIVE_CREDIT:
            PUT_NUMBER8 (self->credit);
            break;
        case ZS_CMD_SEND_CHUNK:
            PUT_NUMBER8 (self->sequence);
            PUT_STRING (self->file_path);
            PUT_NUMBER8 (self->offset);
            frame_flags = ZFRAME_MORE;
        case ZS_CMD_ABORT:
            // no data to put
            break;
        default:
            goto malformed;
    }
    
    /* Append data frame */
    if (zmsg_append (output, &data_frame)) {
           goto cleanup;
    }

    /* Send frames */
    if (self->cmd == ZS_CMD_SEND_CHUNK) {
        
        /* Append the chunk frame */
        if (zmsg_append (output, &self->chunk)) {
            goto cleanup;    
        }
        
    }

    /* Cleanup */
    zs_msg_destroy (&self);

    return 0;

    malformed:
        printf ("[ERROR] malformed message '%d'\n", self->cmd);            
    cleanup:
        zframe_destroy (&data_frame);
        zs_msg_destroy (&self);
        return 1;
}

// --------------------------------------------------------------------------
// Send the GREET to the RP in one step

int 
zs_msg_pack_greet (zmsg_t *output, byte *uuid, uint64_t state) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_GREET);
    zs_msg_set_uuid (self, uuid);
    zs_msg_set_state (self, state);
    size_t frame_size = 16; // 16-byte uuid
    frame_size += 8;        // 8-byte state
    return zs_msg_pack (&self, output, frame_size);
}

// --------------------------------------------------------------------------
// Send the GREET to the RP in one step

int 
zs_msg_pack_last_state (zmsg_t *output, uint64_t last_state) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_LAST_STATE);
    zs_msg_set_state (self, last_state);
    size_t frame_size = 8;  // 8-byte state
    return zs_msg_pack (&self, output, frame_size);
}

// --------------------------------------------------------------------------
// Send the FILE_LIST to the RP in one step

int
zs_msg_pack_update (zmsg_t *output, uint64_t state, zlist_t *fmetadata) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_UPDATE);   
    zs_msg_set_state (self, state);
    zs_msg_set_fmetadata (self, fmetadata);
   
    // calculate frame size
    size_t frame_size = 8; // 8-byte state
    frame_size += 8;       // 8-byte list size
    zs_fmetadata_t *filemeta_data = zs_msg_fmetadata_first (self);
    while (filemeta_data) {
        frame_size += sizeof(string_size_t); // string size
        frame_size += strlen(zs_fmetadata_path (filemeta_data)); // string length
        frame_size += 8;   // 8-byte time stamp
        frame_size += 1;   // 1-byte file operation
        switch (zs_fmetadata_operation (filemeta_data)) {
            case ZS_FILE_OP_UPD:
                frame_size += 8; // 8-byte file size
                frame_size += 8; // 8-byte checksum
                break;
            case ZS_FILE_OP_REN:
                frame_size += sizeof(string_size_t); // string size
                frame_size += strlen(zs_fmetadata_renamed_path (filemeta_data)); // string len
                break;
            default:
                break;
        }
        // next list entry
        filemeta_data = zs_msg_fmetadata_next (self);
    }

    return zs_msg_pack (&self, output, frame_size); 
}

// -------------------------------------------------------------------------
// Send the REQUEST FILES to the RP in one step 

int
zs_msg_pack_request_files (zmsg_t *output, zlist_t *fpaths)
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_REQUEST_FILES);
    
    zs_msg_set_fpaths (self, fpaths);

    size_t frame_size = 8; // 8-byte list size
    char* path = zs_msg_fpaths_first (self);
    while (path) {
        frame_size += sizeof (string_size_t);
        frame_size += strlen (path);
        // next
        path = zs_msg_fpaths_next (self);
    }

    return zs_msg_pack (&self, output, frame_size);
}

// -------------------------------------------------------------------------
// Send the GIVE_CREDIT to a RP (receiving peer)

int 
zs_msg_pack_give_credit (zmsg_t *output, uint64_t credit)
{ 
    assert(output);

    zs_msg_t *msg = zs_msg_new (ZS_CMD_GIVE_CREDIT);
    zs_msg_set_credit (msg, credit); 

    size_t frame_size = 8; // 8-byte credit
    return zs_msg_pack (&msg, output, frame_size); 
}

// -------------------------------------------------------------------------
// Send CHUNK to a SP (sending peer)

int
zs_msg_pack_chunk (zmsg_t *output, uint64_t sequence, char *file_path, uint64_t offset, zframe_t *chunk)
{
    assert(output);
    assert(chunk);

    zs_msg_t *msg = zs_msg_new (ZS_CMD_SEND_CHUNK);
    zs_msg_set_chunk (msg, chunk);
    zs_msg_set_sequence (msg, sequence);
    zs_msg_set_file_path (msg, file_path);
    zs_msg_set_offset (msg, offset);

    size_t frame_size = 0;
    frame_size += sizeof(string_size_t);    // size of string 
    frame_size += strlen(file_path);    // length of string
    frame_size += 8;    // 8-byte sequence
    frame_size += 8;    // 8-byte offset

    return zs_msg_pack (&msg, output, frame_size);
}

// -------------------------------------------------------------------------
// Send ABORT to the RP in one step 

int
zs_msg_pack_abort (zmsg_t *output) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_ABORT);
    size_t frame_size = 0; // there're no data to send
    
    return zs_msg_pack (&self, output, frame_size);
}


// --------------------------------------------------------------------------
// Get the message command

int
zs_msg_get_cmd (zs_msg_t *self) 
{
    assert (self);
    return self->cmd;    
}


// --------------------------------------------------------------------------
// Get/Set the uuid field

void 
zs_msg_set_uuid (zs_msg_t *self, byte *uuid) 
{
    assert (self);
    free (self->uuid);
    self->uuid = uuid;
}

byte * 
zs_msg_uuid (zs_msg_t *self) 
{
    assert (self);
    return self->uuid;
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

// Greedy method takes ownership of fmetadata.
void 
zs_msg_set_fmetadata (zs_msg_t *self, zlist_t *fmetadata) 
{
    assert (self);
   
    // cleanup existing 
    zs_msg_fmetadata_destroy (&self);

    self->fmetadata = fmetadata;
}

zlist_t *
zs_msg_get_fmetadata (zs_msg_t *self) 
{
    assert (self);
    return self->fmetadata;
}

// Iterate through fmetadata zlist

zs_fmetadata_t *
zs_msg_fmetadata_first (zs_msg_t *self)
{
    assert (self);
    if (self->fmetadata)
        return (zs_fmetadata_t *) (zlist_first (self->fmetadata));
    else
        return NULL;
}

zs_fmetadata_t *
zs_msg_fmetadata_next (zs_msg_t *self)
{
    assert (self);
    if (self->fmetadata)
        return (zs_fmetadata_t *) (zlist_next (self->fmetadata));
    else
        return NULL;
}

void
zs_msg_fmetadata_append (zs_msg_t *self, zs_fmetadata_t *fmetadata_item)
{
    assert (self);
    // Format into newly allocated string
    
    if (!self->fmetadata) {
        self->fmetadata = zlist_new ();
    }
    zlist_append (self->fmetadata, fmetadata_item);
}

// --------------------------------------------------------------------------
// Get/Set the fpaths field

zlist_t *
zs_msg_fpaths (zs_msg_t *self)
{
    assert (self);
    return self->fpaths;
}

// Greedy function, takes ownership of fpaths; if you don't want that
// then use zlist_dup() to pass a copy of fpaths

void
zs_msg_set_fpaths (zs_msg_t *self, zlist_t *fpaths)
{
    assert (self);
    zlist_destroy (&self->fpaths);
    self->fpaths = fpaths;
}

// --------------------------------------------------------------------------
// Iterate through the fpaths field, and append a fpaths value

char *
zs_msg_fpaths_first (zs_msg_t *self)
{
    assert (self);
    if (self->fpaths)
        return (char *) (zlist_first (self->fpaths));
    else
        return NULL;
}

char *
zs_msg_fpaths_next (zs_msg_t *self)
{
    assert (self);
    if (self->fpaths)
        return (char *) (zlist_next (self->fpaths));
    else
        return NULL;
}

void
zs_msg_fpaths_append (zs_msg_t *self, char *format, ...)
{
    // Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = (char *) malloc (STRING_MAX + 1);
    assert (string);
    vsnprintf (string, STRING_MAX, format, argptr);
    va_end (argptr);
    
    // Attach string to list
    if (!self->fpaths) {
        self->fpaths = zlist_new ();
        zlist_autofree (self->fpaths);
    }
    zlist_append (self->fpaths, string);
    free (string);
}

// --------------------------------------------------------------------------
// Get/Set the credit

void 
zs_msg_set_credit (zs_msg_t *self, uint64_t credit)
{
    assert (self);
    self->credit = credit;
}

uint64_t 
zs_msg_get_credit (zs_msg_t *self)
{
    assert (self);
    return self->credit;
}

// --------------------------------------------------------------------------
// Get/Set the chunk

void 
zs_msg_set_chunk (zs_msg_t *self, zframe_t *chunk)
{
    assert(self);
    self->chunk = chunk; 
} 

zframe_t*
zs_msg_get_chunk (zs_msg_t *self)
{
    assert(self);
    return self->chunk;     
}

// --------------------------------------------------------------------------
// Get/Set the msg sequence

void 
zs_msg_set_sequence (zs_msg_t *self, uint64_t sequence)
{
   assert(self);
   self->sequence = sequence; 
}

uint64_t
zs_msg_get_sequence (zs_msg_t *self)
{
    assert(self);
    return self->sequence;
}

void
zs_msg_set_file_path (zs_msg_t *self, char *format, ...)
{
    // Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->file_path);
    self->file_path = (char *) malloc (STRING_MAX + 1);
    assert (self->file_path);
    vsnprintf (self->file_path, STRING_MAX, format, argptr);
    va_end (argptr);
}

char*
zs_msg_get_file_path (zs_msg_t *self)
{
    assert(self);
    if(!self->file_path)
        return NULL;

    // copy string from struct 
    char *path = malloc(strlen(self->file_path) * sizeof(char));
    strcpy(path, self->file_path);
    return path;
}

// --------------------------------------------------------------------------
// Get/Set the msg offset

void
zs_msg_set_offset (zs_msg_t *self, uint64_t offset)
{
    assert(self);
    self->offset = offset;
}

uint64_t
zs_msg_get_offset (zs_msg_t *self)
{
    assert(self);
    return self->offset;
}

// --------------------------------------------------------------------------
// Self test this class

int 
zs_msg_test () 
{
    printf("selftest zs_msg* \n");

    //@selftest
    /* 1. create zmq context */
    void *context = zmq_ctx_new ();
    /* 2. create sockets */
    void *sink = zmq_socket (context, ZMQ_DEALER);
    void *sender = zmq_socket (context, ZMQ_DEALER);
    zmsg_t *msg;
    /* 3. bind/connect sockets */
    int rc = zmq_bind (sink, "inproc://zframe.test");
    assert(rc == 0);
    zmq_connect (sender, "inproc://zframe.test");
    
    /* [SEND] GREET */
    msg = zmsg_new ();
    zuuid_t *s_uuid = zuuid_new ();
    zs_msg_pack_greet (msg, zuuid_data (s_uuid), 0xFF);
    zmsg_send (&msg, sender);
    zmsg_destroy (&msg);    // delete zmsg
    
    /* [RECV] GREET */
    msg = zmsg_recv (sink);
    zs_msg_t *self = zs_msg_unpack (msg);
    uint64_t state = zs_msg_get_state (self);
    zuuid_t *r_uuid = zuuid_new ();
    zuuid_set (r_uuid, zs_msg_uuid (self));
    assert ( zuuid_eq (s_uuid, zuuid_data (r_uuid)));
    printf("Command %d\n", zs_msg_get_cmd (self));
    printf ("uuid %s\n", zuuid_str (r_uuid));
    printf("state %"PRIx64"\n", state);
    zmsg_destroy (&msg);    // destroy zmsg
    zs_msg_destroy (&self); // destry zs_msg

    /* [SEND LAST_STATE */
    msg = zmsg_new ();
    zs_msg_pack_last_state (msg, 0x55);
    zmsg_send (&msg, sender);
    zmsg_destroy (&msg);

    /* [RECV] LAST_STATE */
    msg = zmsg_recv (sink);
    self = zs_msg_unpack (msg);
    uint64_t last_state = zs_msg_get_state (self);
    printf("last state %"PRIx64"\n", last_state);
    zmsg_destroy (&msg);    // destroy zmsg
    zs_msg_destroy (&self); // destry zs_msg
    
    /* [SEND] UPDATE */
    msg = zmsg_new ();
    zlist_t *filemeta_list = zlist_new ();
    zs_fmetadata_t *fmetadata = zs_fmetadata_new ();
    zs_fmetadata_set_path (fmetadata, "%s", "a.txt");
    zs_fmetadata_set_operation (fmetadata, ZS_FILE_OP_UPD);
    zs_fmetadata_set_size (fmetadata, 0x1533);
    zs_fmetadata_set_timestamp (fmetadata, 0x1dfa533);
    zs_fmetadata_set_checksum (fmetadata, 0x3312AFFDE12);
    zlist_append(filemeta_list, fmetadata);
    zs_fmetadata_t *fmetadata2 = zs_fmetadata_new ();
    zs_fmetadata_set_path (fmetadata2, "%s", "b.txt");
    zs_fmetadata_set_renamed_path (fmetadata2, "%s", "c.txt");
    zs_fmetadata_set_operation (fmetadata2, ZS_FILE_OP_REN);
    zs_fmetadata_set_timestamp (fmetadata2, 0x1dfa544);
    zlist_append(filemeta_list, fmetadata2);

    zs_msg_pack_update (msg, 0xAB, filemeta_list);
    zmsg_send (&msg, sender);
    zmsg_destroy (&msg);    // delete zmsg
    
    /* [RECV] UPDATE */
    msg = zmsg_recv (sink);
    self = zs_msg_unpack (msg);
    fmetadata = zs_msg_fmetadata_first (self);
    printf("Command %d\n", zs_msg_get_cmd (self));
    printf("State %"PRIx64"\n", zs_msg_get_state (self));
    while (fmetadata) {
        char *path = zs_fmetadata_path (fmetadata);
        char *path_ren = NULL;
        if (zs_fmetadata_renamed_path (fmetadata)) 
            path_ren = zs_fmetadata_renamed_path (fmetadata);
        int operation = zs_fmetadata_operation (fmetadata);
        uint64_t size = zs_fmetadata_size (fmetadata);
        uint64_t timestamp = zs_fmetadata_timestamp (fmetadata);
        uint64_t checksum = zs_fmetadata_checksum (fmetadata);
        printf("%s %s %x %"PRIx64" %"PRIx64" %"PRIx64"\n", path, path_ren, operation, size, timestamp, checksum);
        
        free (path);
        fmetadata = zs_msg_fmetadata_next (self);
    }
    zmsg_destroy (&msg);    // destroy zmsg
    zs_msg_destroy (&self);
   
    /* [SEND] REQUEST FILES */
    msg = zmsg_new ();
    zlist_t *paths = zlist_new ();
    zlist_append(paths, "test1.txt");
    zlist_append(paths, "test2.txt");
    zlist_append(paths, "test3.txt");

    zs_msg_pack_request_files (msg, paths);
    zmsg_send (&msg, sender);
    zmsg_destroy (&msg);    // destroy zmsg


    /* [RECV] REQUEST FILES */
    msg = zmsg_recv (sink);
    self = zs_msg_unpack (msg);
    printf("Command %d\n", zs_msg_get_cmd (self));
    zs_msg_fpaths (self);
    char *path = zs_msg_fpaths_first (self);
    while (path) {
        printf("%s\n", path);
        // next
        path = zs_msg_fpaths_next (self);
    }
    // cleanup
    zmsg_destroy (&msg);    // destroy zmsg
    zs_msg_destroy (&self);

    /* [SEND] ABORT */
    msg = zmsg_new ();
    zs_msg_pack_abort(msg);
    zmsg_send (&msg, sender);
    zmsg_destroy (&msg);    // destroy zmsg

    /* [RECV] NO UPDATE */
    msg = zmsg_recv (sink);
    self = zs_msg_unpack (msg);
    printf("Command %d\n", zs_msg_get_cmd (self));
    // cleanup
    zmsg_destroy (&msg);    // destroy zmsg
    zs_msg_destroy (&self);

    /* 4. close & destroy */
    zmq_close (sink);
    zmq_close (sender);
    zmq_ctx_destroy (context);
    // @end

    printf("OK\n");
    return 0;
}

