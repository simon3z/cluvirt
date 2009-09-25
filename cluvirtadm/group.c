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
#include <string.h>

#include "utils.h"
#include "status.h"
#include "group.h"


LIST_HEAD(, _group_node_t) node_head;


group_node_t *group_add_node(unsigned int nodeid)
{
    group_node_t *p;

    p = malloc(sizeof(group_node_t));
    
    p->nodeid   = nodeid;
    LIST_INIT(&p->vm);

    LIST_INSERT_HEAD(&node_head, p, next);
    
    return p;
}

group_node_t *group_find_node(unsigned int nodeid)
{
    group_node_t *p;
    
    LIST_FOREACH(p, &node_head, next) {
        if (p->nodeid == nodeid) return p;
    }
    
    return 0;
}

void group_remove_node(group_node_t *p)
{
    while (!LIST_EMPTY(&p->vm)) {
        domain_status_t *t = LIST_FIRST(&p->vm);
        
        LIST_REMOVE(t, next);
        
        domain_status_free(t);
        free(t);
    }
    
    LIST_REMOVE(p, next);
    
    free(p);
}

void group_update_status(group_node_t *p, domain_status_t *vm)
{
    domain_status_t *t;
    
    LIST_FOREACH(t, &p->vm, next) {
        if (t->id == vm->id) break;
    }
    
    if (t == 0) {
        t = domain_status_init(0);
        domain_status_copy(t, vm);
        LIST_INSERT_HEAD(&p->vm, t, next);
    }
    else {
        t->state = vm->state;
        t->usage = ((vm->cputime - t->cputime) / 10000000) /
                        (vm->updated - t->updated);
        t->cputime = vm->cputime;
        t->updated = vm->updated;
        t->rcvtime = vm->rcvtime;
    }
}

void group_remove_status_old(group_node_t *p, time_t rcvtime)
{
    domain_status_t *t, *n;
    
    t = LIST_FIRST(&p->vm);
    
    while ((n = t) != 0) {
        t = LIST_NEXT(t, next);
        
        if (n->rcvtime < rcvtime) {
            LIST_REMOVE(n, next);
            domain_status_free(n);
            free(n);
        }
    }
}

void group_print_all(FILE *stream)
{
    group_node_t *p;
    domain_status_t *t;
    
    LIST_FOREACH(p, &node_head, next) {
        fprintf(stream, "+ node id: %i\n", p->nodeid);

        LIST_FOREACH(t, &p->vm, next) {
            fprintf(stream,
                    "+-> vm id: %i, name: %s, state: %c, memory: %li, "
                    "ncpu: %i, vncport: %i, cputime: %llu, usage: %i\n",
                    t->id, t->name, t->state + 0x30, t->memory,
                    t->ncpu, t->vncport, t->cputime, t->usage);
        }
    }
}

