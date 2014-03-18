#ifndef __ZSYNC_CLASSES_H_INCLUDED_
#define __ZSYNC_CLASSES_H_INCLUDED_

#include <czmq.h>
#include <zyre.h>
#include <zyre_event.h>
#include "../include/zs_fmetadata.h"
#include "../include/zs_msg.h"
#include "../include/zsync_msg.h"
#include "../include/zsync_peer.h"
#include "../include/zsync_ftmanager.h"
#include "../include/zsync_credit.h"
#include "../include/zsync_node.h"
#include "../include/zsync_agent.h"
#include "../include/zsync_credit_msg.h"
#include "../include/zsync_ftm_msg.h"

// Strings are encoded with 2-byte length
#define STRING_MAX 65535 // 2-byte - 1-bit for trailing \0

#endif
