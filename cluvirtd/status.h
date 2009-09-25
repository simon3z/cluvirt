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

#ifndef __STATUS_H_
#define __STATUS_H_

#include <stdint.h>
#include <sys/queue.h>


typedef struct _domain_status_t {
    uint8_t                         state;
    uint8_t                         usage;
    uint32_t                        memory;
    uint16_t                        ncpu;
    uint16_t                        vncport;
    uint64_t                        cputime;
} domain_status_t;

typedef struct _domain_info_t {
    uint32_t                        id;
    uint32_t                        update;
    char                            *name;
    domain_status_t                 status;
    LIST_ENTRY(_domain_info_t)      next;
} domain_info_t;


extern LIST_HEAD(domain_info_head_t, _domain_info_t) di_head;


int     domain_status_update();

int     domain_status_to_msg(char *, size_t);
int     domain_status_from_msg(char *, domain_status_t *);


#endif
