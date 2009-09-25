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

#ifndef __NODE_H_
#define __NODE_H_

#include <sys/queue.h>

typedef struct _group_node_t {
    LIST_ENTRY(_group_node_t)      next;
    unsigned int                    nodeid;
    LIST_HEAD(, _domain_status_t)   vm;
} group_node_t;


group_node_t *group_add_node(unsigned int);
group_node_t *group_find_node(unsigned int);

void group_remove_node(group_node_t *);
void group_update_status(group_node_t *, domain_status_t *);
void group_remove_status_old(group_node_t *, time_t);

void group_print_all(FILE *);

#endif
