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


int setup_cpg(cpg_handle_t *daemon_handle, cpg_callbacks_t *cpg_callbacks)
{
    int fd = 0;
    cpg_error_t err;
    static struct cpg_name daemon_name;

    err = cpg_initialize(daemon_handle, cpg_callbacks);
    if (err != CPG_OK) {
            log_debug("cpg_initialize error %d", err);
            return -1;
    }

    cpg_fd_get(*daemon_handle, &fd);
    
    if (fd < 0) {
            log_debug("cpg_fd_get error %d", err);
            return -1;
    }

    memset(&daemon_name, 0, sizeof(daemon_name));
    strcpy(daemon_name.value, CLUVIRT_GROUP_NAME);
    daemon_name.length = strlen(CLUVIRT_GROUP_NAME);

retry:
    err = cpg_join(*daemon_handle, &daemon_name);

    if (err == CPG_ERR_TRY_AGAIN) { 
            sleep(1);
            goto retry;
    }
    if (err != CPG_OK) {
            log_debug("cpg_join error %d", err);
            cpg_finalize(*daemon_handle);
            return -1;
    }

    log_debug("cpg %d", fd);
    return fd;
}

int send_message(cpg_handle_t *daemon_handle, void *buf, int len)
{
    struct iovec iov;
    cpg_error_t err;
    int retries = 0;

    iov.iov_base = buf;
    iov.iov_len = len;

retry: 
    err = cpg_mcast_joined(*daemon_handle, CPG_TYPE_AGREED, &iov, 1);
    
    if (err == CPG_ERR_TRY_AGAIN) {
            retries++;
            usleep(1000);
            if (!(retries % 100))
                    log_error("retry %d", retries);
            goto retry;
    }
    if (err != CPG_OK) {
            log_error("error %d handle %llx",
                      err, (unsigned long long) daemon_handle);
            return -1;
    }

    if (retries) {
            log_debug("retried %d", retries);
    }

    return 0;
}

