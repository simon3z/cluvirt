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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_COROSYNC_CPG_H
#include <corosync/cpg.h>
#else
#include <openais/cpg.h>
#endif


#define CLUVIRT_GROUP_NAME      "cluvirtd"


int setup_cpg(cpg_callbacks_t *);
void dispatch_message(void);
int send_message(void *, int);


#endif
