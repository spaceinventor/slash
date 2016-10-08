/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Space Inventor ApS
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <csp/csp_cmp.h>
#include <csp/csp_endian.h>
#include <slash/slash.h>
#include "base16.h"

static int slash_csp_info(struct slash *slash)
{
	csp_rtable_print();
	csp_conn_print_table();
	csp_iflist_print();
	return SLASH_SUCCESS;
}

slash_command(cspinfo, slash_csp_info, NULL, "Show CSP info");

static int slash_csp_ping(struct slash *slash)
{
	if ((slash->argc <= 1) || (slash->argc > 4))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int size = 1;
	if (slash->argc >= 3)
		size = atoi(slash->argv[2]);

	unsigned int timeout = 1000;
	if (slash->argc >= 4)
		timeout = atoi(slash->argv[3]);

	slash_printf(slash, "Ping node %u size %u timeout %u: ", node, size, timeout);

	int result = csp_ping(node, timeout, size, CSP_SO_NONE);

	if (result >= 0) {
		slash_printf(slash, "Reply in %d [ms]\n", result);
	} else {
		slash_printf(slash, "No reply\n");
	}

	return SLASH_SUCCESS;
}

slash_command(ping, slash_csp_ping, "<node> [size] [timeout]", "Ping a system");

static int slash_csp_reboot(struct slash *slash)
{
	if (slash->argc != 2)
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	csp_reboot(node);

	return SLASH_SUCCESS;
}

slash_command(reboot, slash_csp_reboot, "<node>", "Reboot a node");

static int slash_csp_ps(struct slash *slash)
{
	if ((slash->argc < 2) || (slash->argc > 3))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int timeout = 1000;
	if (slash->argc >= 3)
		timeout = atoi(slash->argv[2]);

	csp_ps(node, timeout);

	return SLASH_SUCCESS;
}

slash_command(ps, slash_csp_ps, "<node> [timeout]", "Process list");

static int slash_csp_memfree(struct slash *slash)
{
	if ((slash->argc < 2) || (slash->argc > 3))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int timeout = 1000;
	if (slash->argc >= 3)
		timeout = atoi(slash->argv[2]);

	csp_memfree(node, timeout);

	return SLASH_SUCCESS;
}

slash_command(memfree, slash_csp_memfree, "<node> [timeout]", "Memfree");

static int slash_csp_buffree(struct slash *slash)
{
	if ((slash->argc < 2) || (slash->argc > 3))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int timeout = 1000;
	if (slash->argc >= 3)
		timeout = atoi(slash->argv[2]);

	csp_buf_free(node, timeout);

	return SLASH_SUCCESS;
}

slash_command(buffree, slash_csp_buffree, "<node> [timeout]", "Memfree");

static int slash_csp_uptime(struct slash *slash)
{
	if ((slash->argc < 2) || (slash->argc > 3))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int timeout = 1000;
	if (slash->argc >= 3)
		timeout = atoi(slash->argv[2]);

	csp_uptime(node, timeout);

	return SLASH_SUCCESS;
}

slash_command(uptime, slash_csp_uptime, "<node> [timeout]", "Memfree");

static int slash_csp_cmp_ident(struct slash *slash)
{
	if ((slash->argc < 2) || (slash->argc > 3))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int timeout = 1000;
	if (slash->argc >= 3)
		timeout = atoi(slash->argv[2]);

	struct csp_cmp_message message;

	if (csp_cmp_ident(node, timeout, &message) != CSP_ERR_NONE) {
		printf("No response\n");
		return SLASH_EINVAL;
	}

	printf("%s\n%s\n%s\n%s %s\n", message.ident.hostname, message.ident.model, message.ident.revision, message.ident.date, message.ident.time);

	return SLASH_SUCCESS;
}

slash_command(ident, slash_csp_cmp_ident, "<node> [timeout]", "Ident");


static int slash_csp_cmp_route_set(struct slash *slash)
{
	if ((slash->argc < 5) || (slash->argc > 6))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	unsigned int dest_node = atoi(slash->argv[2]);
	char * interface = slash->argv[3];
	unsigned int next_hop_mac = atoi(slash->argv[4]);
	unsigned int timeout = 1000;
	if (slash->argc >= 5)
		timeout = atoi(slash->argv[5]);

	struct csp_cmp_message message;

	message.route_set.dest_node = dest_node;
	message.route_set.next_hop_mac = next_hop_mac;
	strncpy(message.route_set.interface, interface, CSP_CMP_ROUTE_IFACE_LEN);

	if (csp_cmp_route_set(node, timeout, &message) != CSP_ERR_NONE) {
		printf("No response\n");
		return SLASH_EINVAL;
	}

	printf("Set route ok\n");

	return SLASH_SUCCESS;
}

slash_command(route_set, slash_csp_cmp_route_set, "<node> <dest_node> <interface> <mac> [timeout]", "Route set");


static int slash_csp_cmp_ifstat(struct slash *slash)
{
	if ((slash->argc < 3) || (slash->argc > 5))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);

	unsigned int timeout = 1000;
	if (slash->argc >= 4)
		timeout = atoi(slash->argv[3]);

	struct csp_cmp_message message;

	strncpy(message.if_stats.interface, slash->argv[2], CSP_CMP_ROUTE_IFACE_LEN);

	if (csp_cmp_if_stats(node, timeout, &message) != CSP_ERR_NONE) {
		printf("No response\n");
		return SLASH_EINVAL;
	}

	message.if_stats.tx =       csp_ntoh32(message.if_stats.tx);
	message.if_stats.rx =       csp_ntoh32(message.if_stats.rx);
	message.if_stats.tx_error = csp_ntoh32(message.if_stats.tx_error);
	message.if_stats.rx_error = csp_ntoh32(message.if_stats.rx_error);
	message.if_stats.drop =     csp_ntoh32(message.if_stats.drop);
	message.if_stats.autherr =  csp_ntoh32(message.if_stats.autherr);
	message.if_stats.frame =    csp_ntoh32(message.if_stats.frame);
	message.if_stats.txbytes =  csp_ntoh32(message.if_stats.txbytes);
	message.if_stats.rxbytes =  csp_ntoh32(message.if_stats.rxbytes);
	message.if_stats.irq = 	 csp_ntoh32(message.if_stats.irq);


	printf("%-5s   tx: %05"PRIu32" rx: %05"PRIu32" txe: %05"PRIu32" rxe: %05"PRIu32"\n"
	       "        drop: %05"PRIu32" autherr: %05"PRIu32" frame: %05"PRIu32"\n"
	       "        txb: %"PRIu32" rxb: %"PRIu32"\n\n",
		message.if_stats.interface,
		message.if_stats.tx,
		message.if_stats.rx,
		message.if_stats.tx_error,
		message.if_stats.rx_error,
		message.if_stats.drop,
		message.if_stats.autherr,
		message.if_stats.frame,
		message.if_stats.txbytes,
		message.if_stats.rxbytes);

	return SLASH_SUCCESS;
}

slash_command(ifstat, slash_csp_cmp_ifstat, "<node> <interface> [timeout]", "Ident");

static int slash_csp_cmp_peek(struct slash *slash)
{
	if ((slash->argc < 4) || (slash->argc > 5))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	unsigned int address;
	sscanf(slash->argv[2], "%x", &address);
	unsigned int len = atoi(slash->argv[3]);

	unsigned int timeout = 1000;
	if (slash->argc > 4)
		timeout = atoi(slash->argv[4]);

	struct csp_cmp_message message;

	message.peek.addr = csp_hton32(address);
	message.peek.len = len;

	if (csp_cmp_peek(node, timeout, &message) != CSP_ERR_NONE) {
		printf("No response\n");
		return SLASH_EINVAL;
	}

	printf("Peek at address %p\n", (void *) (intptr_t) address);
	csp_hex_dump(NULL, message.peek.data, message.peek.len);

	return SLASH_SUCCESS;
}

slash_command(peek, slash_csp_cmp_peek, "<node> <address> <len> [timeout]", "Peek");

static int slash_csp_cmp_poke(struct slash *slash)
{
	if ((slash->argc < 4) || (slash->argc > 5))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	unsigned int address;
	sscanf(slash->argv[2], "%x", &address);
	unsigned int timeout = 1000;
	if (slash->argc > 4)
		timeout = atoi(slash->argv[4]);

	struct csp_cmp_message message;

	message.poke.addr = csp_hton32(address);

	int outlen = base16_decode(slash->argv[3], (uint8_t *) message.poke.data);

	message.poke.len = outlen;

	printf("Poke at address %p\n", (void *) (intptr_t) address);
	csp_hex_dump(NULL, message.poke.data, outlen);

	if (csp_cmp_poke(node, timeout, &message) != CSP_ERR_NONE) {
		printf("No response\n");
		return SLASH_EINVAL;
	}

	printf("Poke ok\n");

	return SLASH_SUCCESS;
}

slash_command(poke, slash_csp_cmp_poke, "<node> <address> <data> [timeout]", "Poke");

static int slash_csp_cmp_time(struct slash *slash)
{
	if ((slash->argc < 3) || (slash->argc > 4))
		return SLASH_EUSAGE;

	unsigned int node = atoi(slash->argv[1]);
	int timestamp = atoi(slash->argv[2]);
	unsigned int timeout = 1000;
	if (slash->argc > 3)
		timeout = atoi(slash->argv[3]);

	struct csp_cmp_message message;

	if (timestamp == -1) {
		csp_timestamp_t localtime;
		clock_get_time(&localtime);
		message.clock.tv_sec = csp_hton32(localtime.tv_sec);
		message.clock.tv_nsec = csp_hton32(localtime.tv_nsec);
	} else {
		message.clock.tv_sec = csp_hton32(timestamp);
		message.clock.tv_nsec = csp_hton32(0);
	}

	if (csp_cmp_clock(node, timeout, &message) != CSP_ERR_NONE) {
		printf("No response\n");
		return SLASH_EINVAL;
	}

	message.clock.tv_sec = csp_ntoh32(message.clock.tv_sec);
	message.clock.tv_nsec = csp_ntoh32(message.clock.tv_nsec);

	printf("Remote time is %"PRIu32".%09"PRIu32"\n", message.clock.tv_sec, message.clock.tv_nsec);

	return SLASH_SUCCESS;
}

slash_command(time, slash_csp_cmp_time, "<node> <timestamp (0 GET, -1 SETLOCAL)> [timeout]", "Time");
