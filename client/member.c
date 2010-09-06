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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <error.h>

#include <libcman.h>

#include <member.h>


int member_list_nodes(clv_clnode_head_t *cn_head)
{
    int             node_count, i;
    cman_handle_t   cman_handle;
    cman_node_t     *cman_nodes, local_node;
    
    if ((cman_handle = cman_init(0)) == 0) {
        return -1;
    }

    if ((node_count = cman_get_node_count(cman_handle)) < 0) {
        return -1;
    }
    
    cman_nodes = calloc((size_t) node_count, sizeof(cman_node_t));
    cman_get_nodes(cman_handle, node_count, &node_count, cman_nodes);
    
    local_node.cn_name[0] = '\0'; /* init cn_name for cman_get_node */

    if (cman_get_node(cman_handle, CMAN_NODEID_US, &local_node) != 0) {
        return -1;
    }
    
    for (i = 0; i < node_count; i++) {
        clv_clnode_t    *n;
        
        n = malloc(sizeof(clv_clnode_t));
        STAILQ_INSERT_TAIL(cn_head, n, next);

        n->id           = (uint32_t) cman_nodes[i].cn_nodeid;
        n->host         = strdup(cman_nodes[i].cn_name);
        n->status       = 0;
        
        if (cman_nodes[i].cn_member != 0) {
            n->status   |= CLUSTER_NODE_ONLINE;
        }
        
        if (cman_nodes[i].cn_nodeid == local_node.cn_nodeid) {
            n->status   |= CLUSTER_NODE_LOCAL;
        }
    }
    
    free(cman_nodes);

    cman_finish(cman_handle);
    return 0;
}
