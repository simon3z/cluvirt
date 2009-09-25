/*
cluvirtd - daemon to multicast libvirt info to openais members.
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
#include <string.h>
#include <time.h>
#include <errno.h>
#include <error.h>

#include <sys/un.h>

#include "status.h"
#include "cluster.h"
#include "utils.h"


#define MESSAGE_BUFFER_SIZE     1024
#define CONTROL_SOCKET_PATH     "/var/run/cluvirtd.sock"


cpg_handle_t        daemon_handle;
domain_info_head_t  di_head = LIST_HEAD_INITIALIZER();


void cpg_deliver(
        cpg_handle_t, struct cpg_name *, uint32_t, uint32_t, void *, int);

void cpg_confchg(
        cpg_handle_t, struct cpg_name *, struct cpg_address *, int,
        struct cpg_address *, int, struct cpg_address *, int);


static cpg_callbacks_t cpg_callbacks = {
    .cpg_deliver_fn = cpg_deliver,
    .cpg_confchg_fn = cpg_confchg
};


void cpg_deliver(cpg_handle_t handle,
        struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, int msg_len)
{
    size_t      msg_size;
    char        reply_msg[MESSAGE_BUFFER_SIZE];
    uint32_t    cmd;
    
    cmd = *(uint32_t*) msg;
    
    if (cmd == 0x01) {
        msg_size = domain_status_to_msg(
                        &di_head, reply_msg, MESSAGE_BUFFER_SIZE);
        log_debug("sending domain info: %lu", msg_size);
        send_message(&daemon_handle, reply_msg, msg_size);
    }
}

void cpg_confchg(cpg_handle_t handle,
        struct cpg_name *group_name,
        struct cpg_address *member_list, int member_list_entries,
        struct cpg_address *left_list, int left_list_entries,
        struct cpg_address *joined_list, int joined_list_entries)
{
    return;
}

void main_loop()
{
    int             fd_csync, fd_max;
    fd_set          readfds;
    struct timeval  select_timeout;
 
    if ((fd_csync = setup_cpg(&daemon_handle, &cpg_callbacks)) < 0) {
        log_error("unable to initialize openais: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    fd_max = fd_csync + 1;
    FD_ZERO(&readfds);
    
    while (1) {
        select_timeout.tv_sec   = 5;
        select_timeout.tv_usec  = 0;
        
        FD_SET(fd_csync, &readfds);
        
        select(fd_max, &readfds, 0, 0, &select_timeout);

        domain_status_update(&di_head);
        
        if (FD_ISSET(fd_csync, &readfds)) {
            if (cpg_dispatch(daemon_handle, CPG_DISPATCH_ALL) != CPG_OK) {
                error(1, errno, "Unable to dispatch");
            }
        }
    }
}

int main(int argc, char *argv[])
{
    main_loop();

    return EXIT_SUCCESS; 
}

