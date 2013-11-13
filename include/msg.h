#ifndef __MSG_H_INCLUDED__
#define __MSG_H_INCLUDED__


#ifdef __cplusplus
extern "C" {
#endif

// Message commands
#define ZS_CMD_GIVE_CREDIT 0x5 


// Opaque class structure
typedef struct _zs_msg_t zs_msg_t;

// receive messages
zs_msg_t *
zs_msg_recv (void *input);

// send LAST_STATE

int 
zs_msg_send_last_state (void *output, uint64_t state);

int
zs_msg_send_give_credit (void *output, uint64_t credit);

// getter/setter message state    
void 
zs_msg_set_state (zs_msg_t *self, uint64_t state);

uint64_t 
zs_msg_get_state (zs_msg_t *self);

void
zs_msg_set_credit (zs_msg_t *self, uint64_t credit);

uint64_t
zs_msg_get_credit (zs_msg_t *self);


#ifdef __cplusplus
}
#endif

#endif
