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

#ifndef __GROUP_H_
#define __GROUP_H_

#ifdef HAVE_COROSYNC_CPG_H
#include <corosync/cpg.h>
#else
#include <openais/cpg.h>
#endif

typedef void clv_msg_callback_fn_t(uint32_t, uint32_t, clv_cmd_msg_t *, size_t);

typedef struct _cluvirtd_group_t {
    int                     fd;
    cpg_handle_t            cpg;
    struct cpg_name         name;
    uint32_t                nodeid;
    clv_clnode_head_t       nodes;
    clv_msg_callback_fn_t   *msgcb;
} cluvirtd_group_t;

int cluvirtd_group_init(cluvirtd_group_t *, clv_msg_callback_fn_t *);
int cluvirtd_group_dispatch(cluvirtd_group_t *);
int cluvirtd_group_message(cluvirtd_group_t *, void *, size_t);

#endif
