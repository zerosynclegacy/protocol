/* =========================================================================
    zsync_selftest - run all self tests

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

#include "zsync_classes.h"

zsync_agent_t *agent;

void
pass_update (char *sender, zlist_t *fmetadata) 
{
    printf ("[ST] PASS_UPDATE from %s: %"PRId64"\n", sender, zlist_size (fmetadata));
    uint64_t size = 0;
    zlist_t *paths = zlist_new ();
    zs_fmetadata_t *meta = zlist_first (fmetadata);
    while (meta) {
        zlist_append (paths, zs_fmetadata_path (meta));
        size += zs_fmetadata_size (meta);
        meta = zlist_next (fmetadata);
    }
    zsync_agent_send_request_files (agent, sender, paths, size);
}


void 
pass_chunk (zchunk_t *chunk, char *path, uint64_t sequence, uint64_t offset)
{
    // save chunk
    printf ("[ST] PASS_CHUNK %s, %"PRId64", %"PRId64", %"PRId64"\n", path, sequence, zchunk_size (chunk), offset);
    zfile_t *file = zfile_new("./syncfolder", path);
    zfile_output(file);
    zfile_write(file, chunk, offset);
    zfile_close(file);
    zfile_destroy(&file);
}

zlist_t *
get_update (uint64_t from_state)
{ 
    printf("[ST] GET_UPDATE\n");
    zlist_t *filemeta_list = zlist_new ();
    
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("./syncfolder")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            if (strcmp (ent->d_name, ".") != 0 && strcmp (ent->d_name, "..") != 0) {
                struct stat st;
                stat(ent->d_name, &st);

                zs_fmetadata_t *fmetadata = zs_fmetadata_new ();
                zs_fmetadata_set_path (fmetadata, "%s", ent->d_name);
                zs_fmetadata_set_size (fmetadata, st.st_size);
                zs_fmetadata_set_operation (fmetadata, ZS_FILE_OP_UPD);
                zs_fmetadata_set_timestamp (fmetadata, st.st_ctime);
                zs_fmetadata_set_checksum (fmetadata, 0x3312AFFDE12);
                zlist_append(filemeta_list, fmetadata);
            }
        }
        closedir (dir);
    } 
    if (zlist_size (filemeta_list) > 0) {
        return filemeta_list;
    }
    else {
        return NULL;
    }
}

// Gets the current state

zchunk_t *
get_chunk (char *path, uint64_t chunk_size, uint64_t offset)
{
    printf("[ST] GET CHUNK\n");
    char *path_new = malloc(strlen("./syncfolder/") + strlen(path) + 1);
    path_new[0] = '\0';
    strcat(path_new, "./syncfolder/");
    strcat(path_new, path); 

    if (zsys_file_exists (path_new)) {
        printf("[ST] File exist\n");
        zfile_t *file = zfile_new (".", path_new);
        if (zfile_is_readable (file)) {
            printf("[ST] File read\n");
            zfile_input (file); 
            if (zfile_size (path_new) > offset) {
                zchunk_t *chunk = zfile_read (file, chunk_size, offset);
                zfile_destroy (&file);
                return chunk;
            }
            else {
                return NULL;
            }
        }
    } else {
        printf("[ST] File %s not exist\n", path_new);
    }
    return NULL;
}

uint64_t
get_current_state () 
{
    return 0x0;
}

void test_integrate_components ()
{
    printf ("Integration Test: ");

    agent = zsync_agent_new ();
    zsync_agent_set_pass_update (agent, pass_update);
    zsync_agent_set_pass_chunk (agent, pass_chunk);
    zsync_agent_set_get_update (agent, get_update);
    zsync_agent_set_get_current_state (agent, get_current_state);
    zsync_agent_set_get_chunk (agent, get_chunk);

    zsync_agent_start (agent);

    zlist_t *files;

    while (zsync_agent_running (agent)) {
        zclock_sleep (4000);
        DIR *dir;
        struct dirent *ent;
        int count = -2;
        if ((dir = opendir ("./syncfolder")) != NULL) {
            while ((ent = readdir (dir)) != NULL) {
                count++; 
            }
        }
        if (count == 2)
            break;
    }
    zsync_agent_stop (agent);
        
    zclock_sleep (500);

    zsync_agent_destroy (&agent);
    
    printf ("OK\n");
}

int 
main (int argc, char *argv [])
{
    printf("Running self tests...\n");
    zs_msg_test ();
    zsync_credit_test ();
    zsync_ftmanager_test ();
    zsync_node_test ();
    zsync_agent_test ();
    if (argc > 1) {
        test_integrate_components ();
    }
    printf("Tests passed OK\n");
}

