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

#include <sys/time.h>
#include <sys/queue.h>

#include <libvirt/libvirt.h>


typedef struct _domain_info_t {
    int                         id;
    char                        *name;
    unsigned char               uuid[VIR_UUID_BUFLEN];
    unsigned char               state;
    unsigned long               memory;
    unsigned short              ncpu;
    unsigned long long          cputime;
    unsigned short              vncport;
    unsigned short              usage;
    struct timeval              update;
    LIST_ENTRY(_domain_info_t)  next;
} domain_info_t;

/* not used yet, endianness */
typedef struct __attribute__ ((__packed__)) _domain_info_msg_t {
    uint32_t                    id;
    int32_t                     usage;
    uint64_t                    cputime;
    uint32_t                    memory;
    uint16_t                    ncpu;
    uint16_t                    vncport;
    int8_t                      state;
    uint8_t                     uuid[VIR_UUID_BUFLEN];
    uint32_t                    data_size;
    unsigned char               *payload[0];
} domain_info_msg_t;


typedef LIST_HEAD(_domain_info_head_t, _domain_info_t) domain_info_head_t;


int     domain_status_update(char *, domain_info_head_t *);

size_t  domain_status_to_msg(domain_info_head_t *, char *, size_t);
size_t  domain_status_from_msg(domain_info_head_t *, char *, size_t);


#endif
