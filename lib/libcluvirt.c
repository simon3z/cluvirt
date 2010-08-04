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
#include <utils.h>


#define CLV_BUFFER_DEFSIZE      8192


int clv_init(clv_handle_t *clvh, const char *filename, int mode)
{
    int sock;
    struct sockaddr_un name;

    if ((sock = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    memset(&name, 0, sizeof(name));
    
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, filename, sizeof(name.sun_path) - 1);
    
    if (mode == CLV_INIT_SERVER) {
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
    
    clvh->fd        = sock;
    clvh->to_sec    = 5;
    clvh->to_usec   = 0;
    clvh->reply     = malloc(CLV_BUFFER_DEFSIZE);
    clvh->reply_len = CLV_BUFFER_DEFSIZE;
    
    return 0;
    
exit_fail:
    close(sock);
    return -1;
}

void clv_finish(clv_handle_t *clvh)
{
    close(clvh->fd);
    free(clvh->reply);
    clvh->reply_len = 0;
}

int clv_get_fd(clv_handle_t *clvh)
{
    return clvh->fd;
}

void *clv_rcv_command(clv_cmd_msg_t *rcv_cmd, void *msg, size_t msg_len)
{
    clv_cmd_msg_t *tmp_cmd = msg;
    
    if (msg_len < sizeof(clv_cmd_msg_t)) {
        return 0;
    }
    
    rcv_cmd->cmd            = be_swap32(tmp_cmd->cmd);
    rcv_cmd->token          = be_swap32(tmp_cmd->token);
    rcv_cmd->nodeid         = be_swap32(tmp_cmd->nodeid);
    rcv_cmd->pid            = be_swap32(tmp_cmd->pid);
    rcv_cmd->payload_size   = be_swap32(tmp_cmd->payload_size);
    
    return msg + sizeof(clv_cmd_msg_t);
}

ssize_t clv_wait_reply(clv_handle_t *clvh)
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
        return read(clvh->fd, clvh->reply, clvh->reply_len);
    }
    
    return 0;
}

int clv_req_domains(
        clv_handle_t *clvh, uint32_t nodeid, clv_vminfo_head_t *di_head)
{
    ssize_t msg_len;
    clv_cmd_msg_t req_cmd;
    
    req_cmd.cmd     = be_swap32(CLV_CMD_REQUEST);
    req_cmd.token   = be_swap32(0); /* FIXME: unused */
    req_cmd.nodeid  = be_swap32(nodeid);
    req_cmd.pid     = be_swap32(0); /* FIXME: unused */
    
    msg_len = write(clvh->fd, &req_cmd, sizeof(req_cmd)); /* sending request */
    
    if (msg_len != sizeof(req_cmd)) {
        return -1; /* error sending request */
    };
    
    if ((msg_len = clv_wait_reply(clvh)) > 0) { /* waiting response */
        clv_cmd_msg_t   asw_cmd;
        
        if (clv_rcv_command(&asw_cmd, clvh->reply, (size_t) msg_len) == 0) {
            return -1;
        }
        
        if (asw_cmd.cmd != CLV_CMD_ANSWER || asw_cmd.nodeid != nodeid) {
            return -1; /* FIXME: improve error handling */
        }
        
        if (clv_vminfo_from_msg(di_head,
                (char *) clvh->reply + sizeof(clv_cmd_msg_t),
                (size_t) msg_len - sizeof(clv_cmd_msg_t)) < 0) {
            return -1;
        }
    }
    
    return 0;
}

