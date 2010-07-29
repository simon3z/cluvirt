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
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <cluvirt.h>

#include "utils.h"


int clv_init(const char *filename, int mode)
{
    int sock;
    struct sockaddr_un name;

    if ((sock = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        log_error("unable to create the cluvirt socket: %i", errno);
        return -1;
    }

    memset(&name, 0, sizeof(name));
    
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, filename, sizeof(name.sun_path) - 1);
    
    if (mode == CLV_INIT_SERVER) {
        unlink(filename);
        
        if (bind(sock, (struct sockaddr *) &name, sizeof (name)) != 0)
        {
            log_error("unable to bind the cluvirt socket: %i", errno);
            goto exit_fail;
        }
        
        if (listen(sock, 1) != 0) {
            log_error("unable to listen on the cluvirt socket: %i", errno);
            goto exit_fail;
        }
    }
    else {
        if (connect(sock, (struct sockaddr *) &name, sizeof (name)) != 0) {
             log_error("unable to connect to the cluvirt socket: %i", errno);
             goto exit_fail;
        }
    }
    
    return sock;
    
exit_fail:
    close(sock);
    return -1;
}

