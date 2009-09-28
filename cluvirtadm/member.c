/*
cluvirtadm - client for cluvirtd daemons.
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

#include "member.h"
#include "utils.h"


int member_init_list(cluster_node_head_t *cn_head)
{
    int             node_count, i;
    cman_handle_t   handle;
    cman_node_t     *cman_nodes;
    
    if ((handle = cman_init(0)) == 0) {
        log_error("unable to initialize cman: %i", errno);
        return -1;
    }
    
    if ((node_count = cman_get_node_count(handle)) < 0) {
        log_error("unable to count cman nodes: %i", errno);
        return -1;
    }
    
    cman_nodes = calloc(node_count, sizeof(cman_node_t));
    cman_get_nodes(handle, node_count, &node_count, cman_nodes);
    
    for (i = 0; i < node_count; i++) {
        cluster_node_t  *n;
        
        n = malloc(sizeof(cluster_node_t));
        LIST_INSERT_HEAD(cn_head, n, next);

        n->id       = cman_nodes[i].cn_nodeid;
        n->host     = strdup(cman_nodes[i].cn_name);
        n->joined   = 0;
        
        LIST_INIT(&n->domain);
    }
    
    free(cman_nodes);
    
    cman_finish(handle);
    
    return 0;
}

