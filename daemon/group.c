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
#include <errno.h>
#include <error.h>

#include <libcman.h>

#include <group.h>
#include <utils.h>


static cman_handle_t    cman_handle = 0;


int group_init(void)
{
    if (cman_handle != 0) {
        log_debug("cman already initialized");
        return 0;
    }
    
    if ((cman_handle = cman_init(0)) == 0) {
        log_debug("unable to initialize cman: %i", errno);
        return -1;
    }
    
    return 0;
}

int group_finish(void)
{
    if (cman_handle == 0) {
        log_debug("cman already initialized");
        return 0;
    }
    
    return cman_finish(cman_handle);
}

int group_node_add(clv_clnode_head_t *cn_head, uint32_t id, uint32_t pid)
{
    clv_clnode_t *n;
    char *node_host = 0;
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) return -1; /* node already present */
    }
    
    if (cman_handle != 0) {
        cman_node_t node_info;
        memset(&node_info, 0, sizeof(node_info));
        
        if (cman_get_node(cman_handle, (int) id, &node_info) == 0) {
            node_host = node_info.cn_name;
        }
    }
    
    if ((n = clv_clnode_new(node_host, id, pid)) == 0) {
        log_error("unable to create new node: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    n->status |= CLUSTER_NODE_ONLINE;
    STAILQ_INSERT_TAIL(cn_head, n, next);

    log_debug("adding node '%s', id: %u, pid: %u", n->host, n->id, n->pid);
    
    return 0;
}

int group_node_remove(clv_clnode_head_t *cn_head, uint32_t id, uint32_t pid)
{
    clv_clnode_t   *n;
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) break;
    }
    
    if (n == 0) return -1; /* node not present */
    
    log_debug("removing node '%s', id: %u, pid: %u", n->host, n->id, n->pid);
    
    STAILQ_REMOVE(cn_head, n, _clv_clnode_t, next);
    
    clv_clnode_free(n); /* freeing node domains */

    return 0;
}

