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

/*
@header
    ZeroSync messages module

    When sending messages the ownership of passed arguments are transfered 
    to the message module.

    When receiving messages the ownership of the of the passed message struct
    stays with the message module.
@discuss
@end
*/

#include "zsync_classes.h"

struct _zs_msg_t {
    uint16_t signature;     // zs_msg signature
    int cmd;                // zs_msg command
    byte *needle;           // read/write pointer for serialization
    byte *ceiling;          // valid upper limit for needle
    uint64_t state;
    uint64_t sequence;
    uint64_t offset;
    char *file_path;
    zframe_t *chunk;
    zlist_t *fmetadata;     // zlist of file meta data list
    zlist_t *fpaths;        // zlist of file paths
    uint64_t credit;        // given credit for RP 
};

struct _zs_fmetadata_t {
    char *path;             // file path + file name
    int operation;
    uint64_t size;          // file size in bytes
    uint64_t timestamp;     // UNIX timestamp
    uint64_t checksum;      // SHA-3 512
};

// ZeroSync Sigature
#define SIGNATURE 0x5A53

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
// Create a new zs_fmetadata

zs_fmetadata_t * 
zs_fmetadata_new () 
{
    zs_fmetadata_t *self = (zs_fmetadata_t *) zmalloc (sizeof (zs_fmetadata_t));
    return self;
}

// --------------------------------------------------------------------------
// Destroy the zs_fmetadata

void 
zs_fmetadata_destroy (zs_fmetadata_t **self_p) 
{
    assert (self_p);

    if (*self_p) {
        zs_fmetadata_t *self = *self_p;
        
        free (self->path);
        // Free object itself
        free (self);
        *self_p = NULL;
    }
}

// --------------------------------------------------------------------------
// Duplicate the zs_fmetadata

zs_fmetadata_t *
zs_fmetadata_dup (zs_fmetadata_t *self)
{
    assert (self);

    zs_fmetadata_t *self_dup = zs_fmetadata_new ();
    
    zs_fmetadata_set_path (self_dup, "%s", self->path);
    zs_fmetadata_set_operation (self_dup, self->operation);
    zs_fmetadata_set_size (self_dup, self->size);
    zs_fmetadata_set_timestamp (self_dup, self->timestamp);
    zs_fmetadata_set_checksum (self_dup, self->checksum);

    return self_dup;
}

// --------------------------------------------------------------------------
// Receive and parse a zs_msg from the socket. Returns new object or
// NULL if error. Will block if there's no message waiting.

zs_msg_t *
zs_msg_recv (void *input) 
{
    assert (input);
    zs_msg_t *self = zs_msg_new (0);
    zframe_t *frame = NULL;
    uint64_t list_size;
    string_size_t string_size;

    // Receive data frame
    frame = zframe_recv (input);
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
            case ZS_CMD_LAST_STATE:
                GET_NUMBER8(self->state);
                break;
            case ZS_CMD_UPDATE:
                GET_NUMBER8(self->state);
                // file meta data count
                GET_NUMBER8(list_size);
                while (list_size--) {
                    zs_fmetadata_t *fmetadata_item = zs_fmetadata_new ();
                    GET_STRING (fmetadata_item->path);
                    GET_NUMBER1 (fmetadata_item->operation);
                    GET_NUMBER8 (fmetadata_item->size);
                    GET_NUMBER8 (fmetadata_item->timestamp);
                    GET_NUMBER8 (fmetadata_item->checksum);
                    // TODO support checksum 
                    zs_msg_fmetadata_append (self, fmetadata_item); 
                }
                break;
            case ZS_CMD_NO_UPDATE:
                // nothing to get
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
                
                self->chunk = zframe_recv (input);
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
zs_msg_send (zs_msg_t **self_p, void *output, size_t frame_size)
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
                PUT_STRING (fmetadata_item->path);
                PUT_NUMBER1 (fmetadata_item->operation);
                PUT_NUMBER8 (fmetadata_item->size);
                PUT_NUMBER8 (fmetadata_item->timestamp);
                PUT_NUMBER8 (fmetadata_item->checksum);
                // cleanup & next list entry
                fmetadata_item = zs_msg_fmetadata_next (self);
            }
        case ZS_CMD_NO_UPDATE:
            // No data to put
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
        default:
            break;
    }
    
       /* Send data frame */
    if (zframe_send (&data_frame, output, frame_flags)) {
               zframe_destroy (&data_frame);
    }

    /* Send frames */
    if (self->cmd == ZS_CMD_SEND_CHUNK) {
        
        /* followed by sending the chunk frame */
        if (zframe_send (&self->chunk, output, 0)) {
            zframe_destroy (&self->chunk);
        }
    }

    /* Cleanup */
    zs_msg_destroy (&self);

    return 0;
}

// --------------------------------------------------------------------------
// Send the LAST_STATE to the RP in one step

int 
zs_msg_send_last_state (void *output, uint64_t state) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_LAST_STATE);
    zs_msg_set_state (self, state);
    size_t frame_size = 8; // 8-byte state
    return zs_msg_send (&self, output, frame_size);
}

// --------------------------------------------------------------------------
// Send the FILE_LIST to the RP in one step

int
zs_msg_send_update (void *output, uint64_t state, zlist_t *fmetadata) 
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
        frame_size += strlen(filemeta_data->path); // string length
        frame_size += 1; // 1-byte file operation
        frame_size += 8; // 8-byte file size
        frame_size += 8; // 8-byte time stamp
        frame_size += 8; // 8-byte checksum
        // TODO support checksum
        // next list entry
        filemeta_data = zs_msg_fmetadata_next (self);
    }

    return zs_msg_send (&self, output, frame_size); 
}

// -------------------------------------------------------------------------
// Send the NO UPDATE to the RP in one step 

int
zs_msg_send_no_update (void *output) 
{
    zs_msg_t *self = zs_msg_new (ZS_CMD_NO_UPDATE);
    size_t frame_size = 0; // there're no data to send
    
    return zs_msg_send (&self, output, frame_size);
}

// -------------------------------------------------------------------------
// Send the REQUEST FILES to the RP in one step 

int
zs_msg_send_request_files (void *output, zlist_t *fpaths)
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

    return zs_msg_send (&self, output, frame_size);
}

// -------------------------------------------------------------------------
// Send the GIVE_CREDIT to a RP (receiving peer)

int 
zs_msg_send_give_credit (void *output, uint64_t credit)
{ 
    assert(output);

    zs_msg_t *msg = zs_msg_new (ZS_CMD_GIVE_CREDIT);
    zs_msg_set_credit (msg, credit); 

    size_t frame_size = 8; // 8-byte credit
    return zs_msg_send (&msg, output, frame_size); 
}

// -------------------------------------------------------------------------
// Send CHUNK to a SP (sending peer)

int
zs_msg_send_chunk (void *output, uint64_t sequence, char *file_path, uint64_t offset, zframe_t *chunk)
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

    return zs_msg_send (&msg, output, frame_size);
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
    return self->file_path;
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
// Get/Set the file meta data path

void
zs_fmetadata_set_path (zs_fmetadata_t *self, char *format, ...) 
{
    assert (self);
    // Format into newly allocated string
    va_list argptr;
    va_start (argptr, format);
    free (self->path);
    self->path = (char *) malloc (STRING_MAX + 1);
    assert (self->path);
    vsnprintf (self->path, STRING_MAX, format, argptr);
    va_end (argptr);
}

char *
zs_fmetadata_get_path (zs_fmetadata_t *self)
{
    assert (self);
    // copy string from struct 
    char *path = malloc(strlen(self->path) * sizeof(char));
    strcpy(path, self->path);
    return path;
}    


// --------------------------------------------------------------------------
// Get/Set the file operation

void
zs_fmetadata_set_operation(zs_fmetadata_t *self, int operation)
{
    assert (self);
    self->operation = operation;
}

int
zs_fmetadata_get_operation(zs_fmetadata_t *self)
{
    assert (self);
    return self->operation;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data size

void
zs_fmetadata_set_size (zs_fmetadata_t *self, uint64_t size) 
{
    assert (self);
    self->size = size;
}    

uint64_t 
zs_fmetadata_get_size (zs_fmetadata_t *self) 
{
    assert (self);
    return self->size;
}    

// --------------------------------------------------------------------------
// Get/Set the file meta data timestamp

void
zs_fmetadata_set_timestamp (zs_fmetadata_t *self, uint64_t timestamp)
{
    assert (self);
    self->timestamp = timestamp;
}

uint64_t 
zs_fmetadata_get_timestamp (zs_fmetadata_t *self) {
    assert (self);
    return self->timestamp;
}

// --------------------------------------------------------------------------
// Get/Set the file meta data checksum

void
zs_fmetadata_set_checksum (zs_fmetadata_t *self, uint64_t checksum)
{
    assert (self);
    self->checksum = checksum;
}

uint64_t 
zs_fmetadata_get_checksum (zs_fmetadata_t *self) {
    assert (self);
    return self->checksum;
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
    /* 3. bind/connect sockets */
    int rc = zmq_bind (sink, "inproc://zframe.test");
    assert(rc == 0);
    zmq_connect (sender, "inproc://zframe.test");
    
    /* [SEND] LAST STATE */
    zs_msg_send_last_state (sender, 0xFF);
    
    /* [RECV] LAST STATE */
    zs_msg_t *self = zs_msg_recv (sink);
    uint64_t last_state = zs_msg_get_state (self);
    printf("Command %d\n", zs_msg_get_cmd (self));
    printf("last state %"PRIx64"\n", last_state);
    
    // cleanup 
    zs_msg_destroy (&self);

    /* [SEND] UPDATE */
    zlist_t *filemeta_list = zlist_new ();
    zs_fmetadata_t *fmetadata = zs_fmetadata_new ();
    zs_fmetadata_set_path (fmetadata, "%s", "a.txt");
    zs_fmetadata_set_operation (fmetadata, ZS_FILE_OP_DEL);
    zs_fmetadata_set_size (fmetadata, 0x1533);
    zs_fmetadata_set_timestamp (fmetadata, 0x1dfa533);
    zs_fmetadata_set_checksum (fmetadata, 0x3312AFFDE12);
    zlist_append(filemeta_list, fmetadata);
    zs_fmetadata_t *fmetadata2 = zs_fmetadata_new ();
    zs_fmetadata_set_path (fmetadata2, "%s", "b.txt");
    zs_fmetadata_set_operation (fmetadata2, ZS_FILE_OP_UPD);
    zs_fmetadata_set_size (fmetadata2, 0x1544);
    zs_fmetadata_set_timestamp (fmetadata2, 0x1dfa544);
    zlist_append(filemeta_list, fmetadata2);

    zs_msg_send_update (sender, 0xAB, filemeta_list);
    
    /* [RECV] FILE LIST */
    self = zs_msg_recv (sink);
    fmetadata = zs_msg_fmetadata_first (self);
    printf("Command %d\n", zs_msg_get_cmd (self));
    printf("State %"PRIx64"\n", zs_msg_get_state (self));
    while (fmetadata) {
        char *path = zs_fmetadata_get_path (fmetadata);
        int operation = zs_fmetadata_get_operation (fmetadata);
        uint64_t size = zs_fmetadata_get_size (fmetadata);
        uint64_t timestamp = zs_fmetadata_get_timestamp (fmetadata);
        uint64_t checksum = zs_fmetadata_get_checksum (fmetadata);
        printf("%s %x %"PRIx64" %"PRIx64" %"PRIx64"\n", path, operation, size, timestamp, checksum);
        
        free (path);
        fmetadata = zs_msg_fmetadata_next (self);
    }

    // cleanup
    zs_msg_destroy (&self);
   
    /* [SEND] NO UPDATE */
    zs_msg_send_no_update(sender);

    /* [RECV] NO UPDATE */
    self = zs_msg_recv (sink);
    printf("Command %d\n", zs_msg_get_cmd (self));
    
    // cleanup
    zs_msg_destroy (&self);
   

    /* [SEND] REQUEST FILES */
    zlist_t *paths = zlist_new ();
    zlist_append(paths, "test1.txt");
    zlist_append(paths, "test2.txt");
    zlist_append(paths, "test3.txt");

    zs_msg_send_request_files (sender, paths);

    /* [RECV] REQUEST FILES */
    self = zs_msg_recv (sink);
    printf("Command %d\n", zs_msg_get_cmd (self));
    char *path = zs_msg_fpaths_first (self);
    while (path) {
        printf("%s\n", path);
        // next
        path = zs_msg_fpaths_next (self);
    }
    // cleanup
    zs_msg_destroy (&self);

    /* 4. close & destroy */
    zmq_close (sink);
    zmq_close (sender);
    zmq_ctx_destroy (context);
    // @end

    printf("OK\n");
    return 0;
}

