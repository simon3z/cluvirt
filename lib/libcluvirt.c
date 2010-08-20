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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <cluvirt.h>
#include <utils.h>


int clv_make_socket(const char *filename, int mode)
{
    int sock;
    struct sockaddr_un name;

    if ((sock = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    memset(&name, 0, sizeof(name));
    
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, filename, sizeof(name.sun_path) - 1);
    
    if (mode == CLV_SOCKET_SERVER) {
        unlink(filename);
        
        if (bind(sock, (struct sockaddr *) &name, sizeof (name)) != 0)  {
            goto exit_fail;
        }
        
        if (listen(sock, 1) != 0) {
            goto exit_fail;
        }
    }
    else {
        if (connect(sock, (struct sockaddr *) &name, sizeof (name)) != 0) {
             goto exit_fail;
        }
    }

    return sock;

exit_fail:
    close(sock);
    return -1;
}

int clv_init(clv_handle_t *clvh, const char *filename)
{
    if ((clvh->fd = clv_make_socket(filename, CLV_SOCKET_CLIENT)) < 0) {
        return -1;
    }

    clvh->to_sec    = 5; /* default timeout: 5 seconds */
    clvh->to_usec   = 0;
    
    return 0;
}

void clv_finish(clv_handle_t *clvh)
{
    close(clvh->fd);
}

int clv_get_fd(clv_handle_t *clvh)
{
    return clvh->fd;
}

ssize_t clv_cmd_read(int fd, clv_cmd_msg_t *msg, size_t maxlen)
{
    ssize_t read_len;

    read_len = read(fd, msg, maxlen);

    /* checking message length */
    if (read_len == 0) return 0;
    else if (read_len < sizeof(clv_cmd_msg_t)) {
        return -1;
    }

    /* checking data length */
    if (msg->data_size != ((size_t) read_len - sizeof(clv_cmd_msg_t))) {
        return -1;
    }

    /* clv_cmd_endian_convert(msg); */

    return read_len;
}

ssize_t clv_cmd_write(int fd, clv_cmd_msg_t *msg)
{
    size_t cmd_len;
    ssize_t write_len;

    cmd_len = sizeof(clv_cmd_msg_t) + msg->data_size;

    /* clv_cmd_endian_convert(msg); */

    write_len = write(fd, msg, cmd_len); /* sending request */
    
    if (write_len != cmd_len) {
        return -1; /* error sending request */
    };

    return write_len;
}

ssize_t clv_cmd_wait(clv_handle_t *clvh, clv_cmd_msg_t *msg, size_t maxlen)
{
    int fd_max;
    fd_set fds_read;
    struct timeval read_to;
    
    fd_max  = clvh->fd + 1;
    
    read_to.tv_sec  = clvh->to_sec;
    read_to.tv_usec = clvh->to_usec;

    FD_ZERO(&fds_read);
    FD_SET(clvh->fd, &fds_read);

    if (select(fd_max, &fds_read, 0, 0, &read_to) < 0) {
        return -1;
    }

    if (FD_ISSET(clvh->fd, &fds_read)) {
        return clv_cmd_read(clvh->fd, msg, maxlen);
    }

    return -1;
}

int clv_fetch_vminfo(
        clv_handle_t *clvh, uint32_t nodeid, clv_vminfo_head_t *di_head)
{
    ssize_t msg_len;
    clv_cmd_msg_t req_cmd, *asw_cmd;
    
    asw_cmd = alloca(CLV_CMD_MSG_MAXSIZE);

    memset(&req_cmd, 0, sizeof(clv_cmd_msg_t));

    req_cmd.cmd     = CLV_CMD_VMINFO;
    req_cmd.nodeid  = nodeid;
    
    if ((clv_cmd_write(clvh->fd, &req_cmd)) < 0) {
        return -1; /* error sending request */
    };
    
    if ((msg_len = clv_cmd_wait(clvh, asw_cmd, CLV_CMD_MSG_MAXSIZE)) < 0) {
        return -1; /* error waiting response */
    }

    if (msg_len > 0) {
        if (asw_cmd->cmd != (CLV_CMD_VMINFO | CLV_CMD_REPLY)
                || asw_cmd->nodeid != nodeid) {
            return -1; /* FIXME: improve error handling */
        }
        
        if (clv_vminfo_from_msg(
                di_head, asw_cmd->data, asw_cmd->data_size) < 0) {
            return -1;
        }
    }
    
    return 0;
}

