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

/*
@header
    ZeroSync file meta data module
    
@discuss
@end
*/

#include "zsync_classes.h"

struct _zs_fmetadata_t {
    char *path;             // file path + file name
    char *path_renamed;     // file path + file name of renamed file
    int operation;
    uint64_t size;          // file size in bytes
    uint64_t timestamp;     // UNIX timestamp
    uint64_t checksum;      // SHA-3 512
};


// --------------------------------------------------------------------------
// Create a new zs_fmetadata

zs_fmetadata_t * 
zs_fmetadata_new () 
{
    zs_fmetadata_t *self = (zs_fmetadata_t *) zmalloc (sizeof (zs_fmetadata_t));
    self->path = NULL;
    self->path_renamed = NULL;
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
        free (self->path_renamed);
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
    zs_fmetadata_set_renamed_path (self_dup, "%s", self->path_renamed);
    zs_fmetadata_set_operation (self_dup, self->operation);
    zs_fmetadata_set_size (self_dup, self->size);
    zs_fmetadata_set_timestamp (self_dup, self->timestamp);
    zs_fmetadata_set_checksum (self_dup, self->checksum);

    return self_dup;
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
zs_fmetadata_path (zs_fmetadata_t *self)
{
    assert (self);
    if (!self->path)
        return NULL;

    // copy string from struct 
    char *path = malloc(strlen(self->path) * sizeof(char));
    strcpy(path, self->path);
    return path;
}    

// --------------------------------------------------------------------------
// Get/Set the renamed file meta data path

void
zs_fmetadata_set_renamed_path (zs_fmetadata_t *self, char *format, ...) 
{
    assert (self);
    // Format into newly allocated string
    va_list argptr;
    va_start (argptr, format);
    free (self->path_renamed);
    self->path_renamed = (char *) malloc (STRING_MAX + 1);
    assert (self->path_renamed);
    vsnprintf (self->path_renamed, STRING_MAX, format, argptr);
    va_end (argptr);
}

char *
zs_fmetadata_renamed_path (zs_fmetadata_t *self)
{
    assert (self);
    if (!self->path_renamed)
        return NULL;

    // copy string from struct 
    char *path = malloc(strlen(self->path_renamed) * sizeof(char));
    strcpy(path, self->path_renamed);
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
zs_fmetadata_operation(zs_fmetadata_t *self)
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
zs_fmetadata_size (zs_fmetadata_t *self) 
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
zs_fmetadata_timestamp (zs_fmetadata_t *self) 
{
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
zs_fmetadata_checksum (zs_fmetadata_t *self) 
{
    assert (self);
    return self->checksum;
}

// --------------------------------------------------------------------------
// Self test this class

int 
zs_fmetadata_test () 
{
    return 0;
}

