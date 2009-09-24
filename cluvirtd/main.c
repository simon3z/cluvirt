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

#include "utils.h"
#include "cluster.h"
#include "status.h"
#include "group.h"
#include "socket.h"


#define MESSAGE_BUFFER_SIZE     1024
#define STATUS_BUFFER_SIZE      32

#define CONTROL_SOCKET_PATH     "/var/run/cluvirtd.sock"


void send_mcast_status()
{
    int msg_size = 0, msg_offset, vm_num, i;
    char mcast_msg[MESSAGE_BUFFER_SIZE];
    domain_status_t status_vm[STATUS_BUFFER_SIZE];
    
    vm_num = domain_status_fetch(status_vm, STATUS_BUFFER_SIZE);
    
    for (i = 0; i < vm_num; i++) {
        msg_offset = domain_status_to_msg(&status_vm[i],
                mcast_msg + msg_size, MESSAGE_BUFFER_SIZE - msg_size);
        
        if (msg_offset < 0) {
            log_error("unable to process vm '%s'", status_vm[i].name);
            goto clean_exit1;
        }
        
        msg_size += msg_offset;
    }

    send_message(mcast_msg, msg_size);

clean_exit1:
    for (i = 0; i < vm_num; i++) {
        domain_status_free(&status_vm[i]);
    }
}

void main_loop()
{
    int csy_fd, ctl_fd, max_fd;
    fd_set readfds, exceptfds;
    time_t lastmsg = 0, now;
    
    if ((csy_fd = setup_cpg()) < 0) {
        log_error("unable to initialize openais: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    if ((ctl_fd = socket_unix(CONTROL_SOCKET_PATH)) < 0) {
        log_error("unable to initialize control socket: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    max_fd = (csy_fd > ctl_fd ? csy_fd : ctl_fd) + 1;
    
    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);
    
    while (1) {
        struct timeval tv;
        
        time(&now);
        
        if ((now - lastmsg) > 5) {
            send_mcast_status();
            time(&lastmsg);
        }
        
        FD_SET(csy_fd, &readfds);
        FD_SET(ctl_fd, &readfds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        select(max_fd, &readfds, 0, &exceptfds, &tv);

        if (FD_ISSET(csy_fd, &readfds)) {
            if (cpg_dispatch(daemon_handle, CPG_DISPATCH_ALL) != CPG_OK) {
                error(1, errno, "Unable to dispatch");
            }
        }
        if (FD_ISSET(ctl_fd, &readfds)) {
            int fd = 0;
            FILE *f;
            struct sockaddr_un remote;
            socklen_t addrlen;
            
            if ((fd = accept(ctl_fd,
                    (struct sockaddr*) &remote, &addrlen)) < 0) {
                goto clean_loop1; 
            }
            
            if ((f = fdopen(fd, "w")) == 0) {
                goto clean_loop1; 
            }
            
            group_print_all(f);
            
            fclose(f);
            
            close(fd);
        }

clean_loop1:        
        continue; // FIXME: workaround for "label at end of compound statement"
    }
}

int main(int argc, char *argv[])
{
    main_loop();

    return EXIT_SUCCESS; 
}

