#pragma once

#include <slash/slash.h>
#include <slash/optparse.h>


/* Global default arguments */
extern unsigned int slash_dfl_node;
extern unsigned int slash_dfl_timeout;

extern unsigned int rdp_dfl_window;
extern unsigned int rdp_dfl_conn_timeout;
extern unsigned int rdp_dfl_packet_timeout;
extern unsigned int rdp_dfl_ack_timeout;
extern unsigned int rdp_dfl_ack_count;

void rdp_opt_add(optparse_t * parser);
void rdp_opt_set();
void rdp_opt_reset();
