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

#include <sys/queue.h>

typedef struct _domain_status_t {
    LIST_ENTRY(_domain_status_t)    next;
    int                             id;
    char                            *name;
    char                            state;
    unsigned long                   memory;
    unsigned short                  ncpu;
    unsigned short                  vncport;
    unsigned long long              cputime;
    int                             usage;
    time_t                          updated;
    time_t                          rcvtime;
} domain_status_t;

domain_status_t *domain_status_init(domain_status_t *);

int     domain_status_fetch(domain_status_t *, int);
void    domain_status_copy(domain_status_t *, domain_status_t *);
void    domain_status_free(domain_status_t *);

int     domain_status_to_msg(domain_status_t *, char *, int);
int     domain_status_from_msg(char *, domain_status_t *);

#endif
