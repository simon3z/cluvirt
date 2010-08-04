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

#include <cluvirt.h>
#include <utils.h>


clv_vminfo_t *clv_vminfo_new(const char *name)
{
    clv_vminfo_t *d;
    
    if ((d = malloc(sizeof(clv_vminfo_t))) == 0) {
        return 0;
    }
    
    if ((d->name = strdup((name == 0) ? "(unknown)" : name)) == 0) {
        free(d);
        return 0;
    }
    
    return d;
}

int clv_vminfo_set_name(clv_vminfo_t *d, const char *name)
{
    free(d->name);
    return ((d->name = strdup(name)) != 0) ? 0 : -1;
}

void clv_vminfo_free(clv_vminfo_t *d)
{
    free(d->name);
    free(d);
}

ssize_t clv_vminfo_to_msg(
        clv_vminfo_head_t *di_head, char *msg, size_t max_size)
{
    clv_vminfo_t       *d;
    clv_vminfo_msg_t   *m;
    size_t              p_offset, n_offset, name_size;
    
    p_offset = 0;
    
    LIST_FOREACH(d, di_head, next) {
        name_size   = strlen(d->name) + 1;
        n_offset    = p_offset + sizeof(clv_vminfo_msg_t) + name_size;
        
        if (n_offset > max_size) {
            return -1;
        }
        
        m = (clv_vminfo_msg_t*) (msg + p_offset);
        
        m->id               = be_swap32((uint32_t) d->id);
        m->memory           = be_swap32((uint32_t) d->memory);
        m->cputime          = be_swap64(d->cputime);
        m->usage            = be_swap16(d->usage);
        m->ncpu             = be_swap16(d->ncpu);
        m->vncport          = be_swap16(d->vncport);

        m->state            = d->state;
        m->payload_size     = be_swap32((uint32_t) name_size);

        memcpy(m->uuid, d->uuid, sizeof(m->uuid));
        memcpy(m->payload, d->name, name_size);
        
        p_offset            = n_offset;
    }
    
    return (ssize_t) p_offset;
}

ssize_t clv_vminfo_from_msg(
        clv_vminfo_head_t *di_head, char *msg, size_t msg_size)
{
    clv_vminfo_t        *d;
    clv_vminfo_msg_t    *m;
    size_t              p_offset = 0, n_offset, name_size;
    
    while ((p_offset + sizeof(clv_vminfo_msg_t)) < msg_size) {
        m = (clv_vminfo_msg_t*) (msg + p_offset);
        
        name_size   = be_swap32(m->payload_size);
        n_offset    = p_offset + sizeof(clv_vminfo_msg_t) + name_size;    
        
        if ((n_offset > msg_size) ||  (m->payload[name_size] != '\0')) {
            return -1;
        }
        
        d = clv_vminfo_new((const char *) m->payload);
        
        d->id           = be_swap32(m->id);
        d->memory       = be_swap32(m->memory);
        d->cputime      = be_swap64(m->cputime);
        d->usage        = be_swap16(m->usage);
        d->ncpu         = be_swap16(m->ncpu);
        d->vncport      = be_swap16(m->vncport);
        d->state        = m->state;

        memcpy(d->uuid, m->uuid, sizeof(d->uuid));
        
        LIST_INSERT_HEAD(di_head, d, next);

        p_offset        = n_offset;
    }

    return (ssize_t) p_offset;
}

clv_clnode_t *clv_clnode_new(const char *host, uint32_t id, uint32_t pid)
{
    static char def_host[256];
    clv_clnode_t *n;
    
    if ((n = malloc(sizeof(clv_vminfo_t))) == 0) {
        return 0;
    }
    
    n->id       = id;
    n->pid      = pid;
    n->status   = 0;
    
    LIST_INIT(&n->domain);
    
    if (host == 0) {
        if (snprintf(def_host,
                sizeof(def_host), "<node%u:%u>", n->id, n->pid) < 0) {
            goto exit_fail;
        }
        if ((n->host = strdup(def_host)) == 0) {
            goto exit_fail;
        }
    }
    else {
        if ((n->host = strdup(host)) == 0) {
            goto exit_fail;
        }
    }
    
    return n;
    
exit_fail:
    free(n);
    return 0;
}

void clv_clnode_free(clv_clnode_t *n)
{
    clv_vminfo_t *d;
    
    while ((d = LIST_FIRST(&n->domain)) != 0) { /* freeing node domains */
        LIST_REMOVE(d, next);
        clv_vminfo_free(d);
    }
    
    free(n->host);
    free(n);
}

