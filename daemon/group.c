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

int group_node_add(cluster_node_head_t *cn_head, uint32_t id, uint32_t pid)
{
    cluster_node_t *n;
    static char n_name[256];
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) return -1; /* node already present */
    }
    
    n = malloc(sizeof(cluster_node_t));
    STAILQ_INSERT_TAIL(cn_head, n, next);

    n->id           = id;
    n->pid          = pid;
    n->host         = 0;
    n->status       = CLUSTER_NODE_ONLINE;
    
    if (cman_handle != 0) {
        cman_node_t node_info;
        
        memset(&node_info, 0, sizeof(node_info));
        
        if (cman_get_node(cman_handle, (int) n->id, &node_info) == 0) {
            n->host = strdup(node_info.cn_name);
        }
    }
    
    if (n->host == 0) {
        snprintf(n_name, sizeof(n_name), "<node%u:%u>", n->id, n->pid);
        n->host     = strdup(n_name);
    }
    
    LIST_INIT(&n->domain);

    log_debug("adding node '%s', id: %u, pid: %u", n->host, n->id, n->pid);
    
    return 0;
}

int group_node_remove(cluster_node_head_t *cn_head, uint32_t id, uint32_t pid)
{
    domain_info_t  *d;
    cluster_node_t *n;
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) break;
    }
    
    if (n == 0) return -1; /* node not present */
    
    log_debug("removing node '%s', id: %u, pid: %u", n->host, n->id, n->pid);
    
    STAILQ_REMOVE(cn_head, n, _cluster_node_t, next);
    
    while ((d = LIST_FIRST(&n->domain)) != 0) { /* freeing node domains */
        LIST_REMOVE(d, next);
        free(d->name);
        free(d);
    }
    
    free(n->host);
    free(n);
        
    return 0;
}

