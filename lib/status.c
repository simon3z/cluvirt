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


ssize_t clv_domain_to_msg(
        domain_info_head_t *di_head, char *msg, size_t max_size)
{
    domain_info_t       *d;
    domain_info_msg_t   *m;
    size_t              p_offset, n_offset, name_size;
    
    p_offset = 0;
    
    LIST_FOREACH(d, di_head, next) {
        name_size   = strlen(d->name) + 1;
        n_offset    = p_offset + sizeof(domain_info_msg_t) + name_size;
        
        if (n_offset > max_size) {
            log_error("message buffer is not large enough: %lu", n_offset);
            return -1;
        }
        
        m = (domain_info_msg_t*) (msg + p_offset);
        
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
        
        log_debug("p_offset: %lu, n_offset: %lu, m: %p", p_offset, n_offset, m);
        p_offset            = n_offset;
    }
    
    return (ssize_t) p_offset;
}

ssize_t clv_domain_from_msg(
        domain_info_head_t *di_head, char *msg, size_t msg_size)
{
    domain_info_t       *d;
    domain_info_msg_t   *m;
    size_t              p_offset = 0, n_offset, name_size;
    
    while ((p_offset + sizeof(domain_info_msg_t)) < msg_size) {
        d = malloc(sizeof(domain_info_t));
        m = (domain_info_msg_t*) (msg + p_offset);
        
        d->id           = be_swap32(m->id);
        d->memory       = be_swap32(m->memory);
        d->cputime      = be_swap64(m->cputime);
        d->usage        = be_swap16(m->usage);
        d->ncpu         = be_swap16(m->ncpu);
        d->vncport      = be_swap16(m->vncport);
        d->state        = m->state;
        name_size       = be_swap32(m->payload_size);
        
        n_offset        = p_offset + sizeof(domain_info_msg_t) + name_size;
        
        if (n_offset > msg_size) {
            log_error("malformed message: %lu %lu", n_offset, msg_size);
            return -1;
        }
        
        d->name         = malloc(name_size);
        
        memcpy(d->name, m->payload, name_size);
        memcpy(d->uuid, m->uuid, sizeof(d->uuid));

        LIST_INSERT_HEAD(di_head, d, next);

        log_debug("p_offset: %lu, n_offset: %lu, m: %p", p_offset, n_offset, m);
        p_offset        = n_offset;
    }

    return (ssize_t) p_offset;
}

