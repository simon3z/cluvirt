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

void *clv_rcv_command(clv_cmd_msg_t *rcv_cmd, void *msg, size_t msg_len)
{
    clv_cmd_msg_t *tmp_cmd = msg;
    
    if (msg_len < sizeof(clv_cmd_msg_t)) {
        log_error("malformed message, size: %lu", msg_len);
        return 0;
    }
    
    rcv_cmd->cmd            = be_swap32(tmp_cmd->cmd);
    rcv_cmd->token          = be_swap32(tmp_cmd->token);
    rcv_cmd->nodeid         = be_swap32(tmp_cmd->nodeid);
    rcv_cmd->pid            = be_swap32(tmp_cmd->pid);
    rcv_cmd->payload_size   = be_swap32(tmp_cmd->payload_size);
    
    return msg + sizeof(clv_cmd_msg_t);
}

int clv_req_domains(int fd, uint32_t nodeid)
{
    ssize_t msg_len;
    clv_cmd_msg_t req_cmd;
    
    req_cmd.cmd     = be_swap32(CLV_CMD_REQUEST);
    req_cmd.token   = be_swap32(0); /* FIXME: unused */
    req_cmd.nodeid  = be_swap32(nodeid);
    req_cmd.pid     = be_swap32(0); /* FIXME: unused */
    
    msg_len = write(fd, &req_cmd, sizeof(req_cmd));
    
    return (msg_len == 1) ? 0 : -1;
}

