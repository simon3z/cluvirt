/*
cluvirtadm - client for cluvirtd daemons.
Copyright (C) 2009  Federico Simoncelli <federico.simoncelli@nethesis.it>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>

#include <libcman.h>

#include "status.h"
#include "cluster.h"
#include "member.h"
#include "utils.h"


static cpg_handle_t         daemon_handle;
static cluster_node_head_t  cn_head;

static int group_members, group_answers;


void cpg_deliver(cpg_handle_t handle,
        struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, int msg_len)
{
    domain_info_t       *d;
    cluster_node_t      *n;
    
    log_debug("receiving a message: %i", msg_len);
    
    if (((char*)msg)[0] != 0x00) return; /* FIXME: improve message type */
    
    LIST_FOREACH(n, &cn_head, next) {
        if (n->id == nodeid) break;
    }
    
    if (n == 0) {
        log_error("cluster node %i not found", nodeid);
        return;
    }
    
    domain_status_from_msg(&n->domain, msg + 1, msg_len);

    group_answers++;
}

void cpg_confchg(cpg_handle_t handle,
        struct cpg_name *group_name,
        struct cpg_address *member_list, int member_list_entries,
        struct cpg_address *left_list, int left_list_entries,
        struct cpg_address *joined_list, int joined_list_entries)
{
    group_members = member_list_entries - 1;
}

void request_domain_info()
{
    char request_msg[] = { 0x01 };
    send_message(&daemon_handle, request_msg, sizeof(request_msg));
}


static cpg_callbacks_t cpg_callbacks = {
    .cpg_deliver_fn = cpg_deliver, .cpg_confchg_fn = cpg_confchg
};


int print_all()
{
    domain_info_t   *d;
    cluster_node_t  *n;
    
    LIST_FOREACH(n, &cn_head, next) {
        printf("node id: %i, host: %s\n", n->id, n->host);
        LIST_FOREACH(d, &n->domain, next) {
            printf("  vm id: %i, name: %s, vnc: %i, cpu: %i%%\n",
                    d->id, d->name, d->status.vncport, d->status.usage);
        }
    }
}

int main(int argc, char *argv[])
{
    int             fd_csync, fd_max;
    fd_set          readfds;
    struct timeval  select_timeout;

 
    log_debug("starting cluvirtadm: %i", argc);
 
    if ((fd_csync = setup_cpg(&daemon_handle, &cpg_callbacks)) < 0) {
        log_error("unable to initialize openais: %i", errno);
        exit(EXIT_FAILURE);
    }

    member_init_list(&cn_head);

    fd_max = fd_csync + 1;
    FD_ZERO(&readfds);
    
    request_domain_info();

    while (1) {
        select_timeout.tv_sec   = 5;
        select_timeout.tv_usec  = 0;
        
        FD_SET(fd_csync, &readfds);
        
        select(fd_max, &readfds, 0, 0, &select_timeout);
        
        if (FD_ISSET(fd_csync, &readfds)) {
            if (cpg_dispatch(daemon_handle, CPG_DISPATCH_ALL) != CPG_OK) {
                error(1, errno, "Unable to dispatch");
            }
        }
        
        if (group_answers >= group_members) {
            break;
        }
    }
    
    print_all();
    
    return 0;
}

