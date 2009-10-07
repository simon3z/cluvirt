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

#include "status.h"
#include "cluster.h"
#include "utils.h"


static cpg_handle_t daemon_handle;
static struct cpg_name daemon_name;
static unsigned int local_nodeid;


int setup_cpg(cpg_callbacks_t *cpg_callbacks)
{
    int fd = 0;
    cpg_error_t err;

    err = cpg_initialize(&daemon_handle, cpg_callbacks);
    if (err != CPG_OK) {
            log_error("unable to initialize cpg: %d", err);
            return -1;
    }

    cpg_fd_get(daemon_handle, &fd);
    
    if (fd < 0) {
            log_error("unable to get the cpg file descriptor: %d", err);
            return -1;
    }

    memset(&daemon_name, 0, sizeof(daemon_name));
    strcpy(daemon_name.value, CLUVIRT_GROUP_NAME);
    daemon_name.length = strlen(CLUVIRT_GROUP_NAME);

retry:
    err = cpg_join(daemon_handle, &daemon_name);

    if (err == CPG_ERR_TRY_AGAIN) { 
        sleep(1);
        goto retry;
    }
    if (err != CPG_OK) {
        log_error("unable to join the cpg group %d", err);
        goto exit_fail;
    }
    
    if ((err = cpg_local_get(daemon_handle, &local_nodeid)) != CPG_OK) {
        log_error("unable to get the local nodeid %d", err);
        goto exit_fail;
    }

    log_debug("cpg file descriptor: %d", fd);
    return fd;

exit_fail:
    cpg_finalize(daemon_handle);
    return -1;
}

unsigned int get_local_nodeid(void)
{
    return local_nodeid;
}

void dispatch_message(void)
{
    if (cpg_dispatch(daemon_handle, CPG_DISPATCH_ALL) != CPG_OK) {
        log_error("unable to dispatch: %i", errno);
    }
}

int send_message(void *buf, int len)
{
    struct iovec iov;
    cpg_error_t err;
    int retries = 0;

    iov.iov_base = buf;
    iov.iov_len = len;

retry: 
    err = cpg_mcast_joined(daemon_handle, CPG_TYPE_AGREED, &iov, 1);
    
    if (err == CPG_ERR_TRY_AGAIN) {
        retries++;
        usleep(1000);
        goto retry;
    }
    if (err != CPG_OK) {
        log_error("unable to send message: %d", err);
        return -1;
    }

    if (retries) {
        log_debug("message sent after %d retries", retries);
    }

    return 0;
}

