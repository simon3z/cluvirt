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

#ifndef __MEMBER_H_
#define __MEMBER_H_

#include <sys/queue.h>

#include <libcman.h>
#include <cluvirt.h>

#define CLUSTER_NODE_ONLINE     0x0001
#define CLUSTER_NODE_LOCAL      0x0002
#define CLUSTER_NODE_JOINED     0x0004

typedef struct _cluster_node_t {
    int                             id;
    char                            *host;
    int                             status;
    domain_info_head_t              domain;
    STAILQ_ENTRY(_cluster_node_t)   next;
} cluster_node_t;


typedef STAILQ_HEAD(_cluster_node_head_t, _cluster_node_t) cluster_node_head_t;


int member_init_list(cluster_node_head_t *);


#endif
