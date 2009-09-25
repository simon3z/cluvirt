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
#include "status.h"
#include "cluster.h"


#define SELECT_FD_BUFFER_SIZE   32
#define CONTROL_SOCKET_PATH     "/var/run/cluvirtd.sock"


void main_loop()
{
    int             fd_csync, fd_max;
    fd_set          readfds;
    struct timeval  select_timeout;
 
    if ((fd_csync = setup_cpg()) < 0) {
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

        domain_status_update();
        
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

