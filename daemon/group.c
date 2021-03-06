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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <error.h>

#include <cluvirt.h>

#include <group.h>
#include <utils.h>


#define CLUVIRT_GROUP_NAME      "cluvirtd"


static int cluvirtd_group_add(
        clv_clnode_head_t *cn_head, uint32_t id, uint32_t pid)
{
    clv_clnode_t *n;
    char *node_host = 0;
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) return -1; /* node already present */
    }
    
/* FIXME: enable cman
    if (cman_handle != 0) {
        cman_node_t node_info;
        memset(&node_info, 0, sizeof(node_info));
        
        if (cman_get_node(cman_handle, (int) id, &node_info) == 0) {
            node_host = node_info.cn_name;
        }
    }
*/
    if ((n = clv_clnode_new(node_host, id, pid)) == 0) {
        return -1;
    }
    
    n->status |= CLUSTER_NODE_ONLINE;
    STAILQ_INSERT_TAIL(cn_head, n, next);
    
    return 0;
}

static int cluvirtd_group_remove(
        clv_clnode_head_t *cn_head, uint32_t id, uint32_t pid)
{
    clv_clnode_t   *n;
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) break;
    }
    
    if (n == 0) return -1; /* node not present */
    
    STAILQ_REMOVE(cn_head, n, _clv_clnode_t, next);
    
    clv_clnode_free(n); /* freeing node domains */

    return 0;
}

void group_cpg_deliver(cpg_handle_t handle,
        const struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, size_t msg_len)
{
    clv_cmd_msg_t *cmd_msg;
    cluvirtd_group_t *grph = 0;

    if (cpg_context_get(handle, (void **) &grph) != CPG_OK) {
        return;
    }

    if (msg_len < sizeof(clv_cmd_msg_t)) { /* message too short */
        return;
    }

    cmd_msg = msg;
    clv_cmd_endian_convert(cmd_msg);

    if (cmd_msg->data_size != msg_len - sizeof(clv_cmd_msg_t)) {
        return;
    }

    grph->msgcb(nodeid, pid, msg, msg_len);
}

void group_cpg_confchg(
        cpg_handle_t handle, const struct cpg_name *group_name,
        const struct cpg_address *member_list, size_t member_list_entries,
        const struct cpg_address *left_list, size_t left_list_entries,
        const struct cpg_address *joined_list, size_t joined_list_entries)
{
    size_t i;
    cluvirtd_group_t *grph;

    if (cpg_context_get(handle, (void **) &grph) != CPG_OK) {
        return; /* error message */
    }
    
    /* adding member nodes */
    for (i = 0; i < member_list_entries; i++) {
        cluvirtd_group_add(
                &grph->nodes, member_list[i].nodeid, member_list[i].pid);
    }
    
    /* adding joining nodes */
    for (i = 0; i < joined_list_entries; i++) {
        cluvirtd_group_add(
                &grph->nodes, joined_list[i].nodeid, joined_list[i].pid);
    }
    
    /* removing leaving nodes */
    for (i = 0; i < left_list_entries; i++) {
        cluvirtd_group_remove(
                &grph->nodes, left_list[i].nodeid, left_list[i].pid);
    }
}

static cpg_callbacks_t
        _group_callbacks = {
            .cpg_deliver_fn = group_cpg_deliver,
            .cpg_confchg_fn = group_cpg_confchg
        };

int cluvirtd_group_init(cluvirtd_group_t *grph, clv_msg_callback_fn_t *msgcb)
{
    cpg_error_t err;

    if (cpg_initialize(&grph->cpg, &_group_callbacks) != CPG_OK) {
        return -1;
    }

    if (cpg_fd_get(grph->cpg, &grph->fd) != CPG_OK) {
        return -1;
    }
    
    if (grph->fd < 0) return -1; /* invalid file descriptor */

    memset(&grph->name, 0, sizeof(struct cpg_name));

    strcpy(grph->name.value, CLUVIRT_GROUP_NAME);
    grph->name.length = strlen(CLUVIRT_GROUP_NAME);

    STAILQ_INIT(&grph->nodes);
    grph->msgcb = msgcb;

retry:
    err = cpg_join(grph->cpg, &grph->name);

    if (err == CPG_ERR_TRY_AGAIN) { 
        usleep(1000);
        goto retry;
    }
    else if (err != CPG_OK) {
        goto exit_fail;
    }
    
    if ((err = cpg_local_get(grph->cpg, &grph->nodeid)) != CPG_OK) {
        goto exit_fail;
    }

    cpg_context_set(grph->cpg, grph);

    return grph->fd;

exit_fail:
    cpg_finalize(grph->cpg);
    return -1;
}

int cluvirtd_group_dispatch(cluvirtd_group_t *grph)
{
    return (cpg_dispatch(grph->cpg, CPG_DISPATCH_ALL) == CPG_OK) ? 0 : -1;
}

int cluvirtd_group_message(cluvirtd_group_t *grph, clv_cmd_msg_t *msg)
{
    struct iovec iov;
    cpg_error_t err;

    iov.iov_len     = sizeof(clv_cmd_msg_t) + msg->data_size;
    iov.iov_base    = alloca(iov.iov_len);

    memcpy(iov.iov_base, msg, iov.iov_len);
    clv_cmd_endian_convert((clv_cmd_msg_t *) iov.iov_base);

retry: 
    err = cpg_mcast_joined(grph->cpg, CPG_TYPE_AGREED, &iov, 1);
    
    if (err == CPG_ERR_TRY_AGAIN) {
        usleep(1000);
        goto retry;
    }
    else if (err != CPG_OK) {
        return -1;
    }

    return 0;
}

