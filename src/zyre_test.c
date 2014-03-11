#include "zsync_classes.h"

// Zsync Peer
// -------------------------------------------------

struct _zsync_peer_t {
    char *uuid;
    int state;
};

typedef struct _zsync_peer_t zsync_peer_tt;

zsync_peer_tt *
zsync_peer_new (char *uuid, int state)
{
    zsync_peer_tt *self = (zsync_peer_tt *) malloc (sizeof (zsync_peer_tt));
    assert (self);
    self->uuid = uuid;
    self->state = state;
    return self;
}

char *
zsync_peer_uuid (zsync_peer_tt *self) 
{
    assert (self);
    return self->uuid;
}

void
zsync_peer_set_status (zsync_peer_tt *self, int state)
{
    assert (self);
    self->state = state;
}

int
zsync_peer_status (zsync_peer_tt *self)
{
    assert (self);
    return self->state;
}

// Zsync Manager
// -------------------------------------------------

struct _zsync_manager_t {
    zlist_t *peers;
    char *name;
};

typedef struct _zsync_manager_t zsync_manager_t;

zsync_manager_t *
zsync_manager_new (char *name)
{
    zsync_manager_t *self = (zsync_manager_t *) malloc (sizeof (zsync_manager_t));
    assert (self);
    self->peers = zlist_new ();
    self->name = name;
    return self;
}

void
zsync_manager_destroy (zsync_manager_t **self_p)
{
    assert (self_p);
    if (*self_p) { 
        zsync_manager_t *self = *self_p;
	zlist_destroy (&self->peers);
	free (self->name);
    }
}

void
zsync_manager_update_peer (zsync_manager_t *self, char *uuid, int state)
{
    assert (self);
    bool hit = false;
    zsync_peer_tt *peer = zlist_first (self->peers);
    while (peer) {
	char *peer_uuid = zsync_peer_uuid (peer);
    	if (streq (peer_uuid, uuid)) {
	    zsync_peer_set_status (peer, state);
	    hit = true;
	    break;
	}
        peer = zlist_next (self->peers);
    }   
    if (!hit) {
       zsync_peer_tt *peer_new = zsync_peer_new (uuid, state);
       zlist_append (self->peers, peer_new);
    }
}

void 
zsync_manager_dump (zsync_manager_t *self) 
{
    printf("[%s] Dump of %d peer\n", self->name, zlist_size (self->peers));
    zsync_peer_t *peer = zlist_first (self->peers);
    while (peer) {
        char *uuid = zsync_peer_uuid (peer);
	int state = zsync_peer_status (peer);
	printf ("[%s] UUID(%s) \tState(%d)\n", self->name, uuid, state);
	peer = zlist_next (self->peers);
    }
}

static bool 
s_zthread_handle_shutdown (void *pipe) 
{
    (void *) zmsg_recv (pipe);
    zmsg_t *msg = zmsg_new ();
    zmsg_pushstr (msg, "shutdown");
    zmsg_send (&msg, pipe);
    
    return true;
}

void 
zyre_worker (void *args, zctx_t *ctx, void *pipe) {
    bool active = true;
    zsync_manager_t *manager = zsync_manager_new (args);
    printf("[%s] Worker started\n", (char *) args);
    zyre_t *node = zyre_new (ctx);
    
    zyre_start (node);
    zyre_join (node, "GLOBAL");
    
    zclock_sleep (250);
   
    zpoller_t *poller = zpoller_new (zyre_socket (node), pipe, NULL);
    while (active) {
        void *sender = zpoller_wait (poller, -1);

	if (sender == zyre_socket (node)) {
	    zyre_event_t *event = zyre_event_recv (node);
	    char *uuid = zyre_event_sender (event);
	    zsync_manager_update_peer (manager, uuid, zyre_event_type (event));
	}
	if (sender == pipe) {
	    printf("[%s] pipe\n", (char *) args);
	    s_zthread_handle_shutdown (pipe);
	    active = false;
	}
    }

    printf("[%s] exit\n", (char *) args);
    zpoller_destroy (&poller);
    zyre_stop (node);
    zyre_destroy (&node);
    zsync_manager_dump (manager);
    zsync_manager_destroy (&manager);
}

int
main (int argc, char **args)
{
    int active;
    zctx_t * ctx = zctx_new ();
    zpoller_t *poller = zpoller_new (NULL);

    int max_worker = 3;
    if (argc == 2) {
    	sscanf(args[1], "%d", &max_worker);
    }
    zlist_t *pipelist = zlist_new ();
    for (int i = 0; i < max_worker; i++) {
     	char *name = malloc (sizeof (char) * 3);
	name[0] = 'w';
	name[1] = (65 + i);
	name[2] = '\0';
	void *pipe = zthread_fork (ctx, zyre_worker, name);
	zlist_append (pipelist, pipe);
	zpoller_add (poller, pipe);
    } 

    bool shutdown = false;
    active = zlist_size (pipelist);
    while (active > 0) {
	if (!shutdown) {
            zclock_sleep (15000);
	    printf ("send shutdown\n");
            void *pipe = zlist_first (pipelist);
	    while (pipe) {
	         zmsg_t *msg = zmsg_new ();
                 zmsg_pushstr (msg, "shutdown");
	         zmsg_send (&msg, pipe);
	         pipe = zlist_next (pipelist);
	    }
	    shutdown = true;
	}
    	void *sender = zpoller_wait (poller, -1);
	if (zpoller_terminated (poller)) {
	    printf ("Interupted exit\n");
	    break;
	}

        void *pipe = zlist_first (pipelist);
	while (pipe) {
	    if (sender == pipe) {
	        zmsg_recv (pipe);
		active--;
		printf ("worker shutdown (%d)\n", active);
		break;
	    }
	    pipe = zlist_next (pipelist);
	}
    }
    
    zpoller_destroy (&poller);    
    printf("destroy context\n");
    zctx_destroy (&ctx);
}

