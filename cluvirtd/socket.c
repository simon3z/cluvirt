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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "socket.h"


int socket_unix(char *path)
{
    int s;
    socklen_t len_l;
    struct sockaddr_un local;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);

    unlink(local.sun_path);

    len_l = sizeof(local.sun_family) + strlen(local.sun_path);

    if (bind(s, (struct sockaddr *)&local, len_l) < 0) {
        goto exit_fail;
    }

    if (listen(s, 5) < 0) {
        goto exit_fail;
    }

    return s;

exit_fail:

    close(s);
    return -1;
}

