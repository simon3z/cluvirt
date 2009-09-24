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

#ifndef __CLUSTER_H_
#define __CLUSTER_H_

#include <openais/cpg.h>


#define CLUVIRTD_MAGIC      0x43560001


void cpg_deliver(cpg_handle_t,
    struct cpg_name *, uint32_t, uint32_t, void *, int);

void cpg_confchg(cpg_handle_t,
    struct cpg_name*, struct cpg_address*, int,
    struct cpg_address*, int, struct cpg_address*, int);


extern cpg_handle_t     daemon_handle;


int setup_cpg();
int send_message(void *, int);


#endif
