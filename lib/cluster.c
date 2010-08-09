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
#include <utils.h>


#define CLUVIRT_GROUP_NAME      "cluvirtd"


static int clv_group_add(
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
        log_error("unable to create new node: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    n->status |= CLUSTER_NODE_ONLINE;
    STAILQ_INSERT_TAIL(cn_head, n, next);

    log_debug("adding node '%s', id: %u, pid: %u", n->host, n->id, n->pid);
    
    return 0;
}

static int clv_group_remove(
        clv_clnode_head_t *cn_head, uint32_t id, uint32_t pid)
{
    clv_clnode_t   *n;
    
    STAILQ_FOREACH(n, cn_head, next) {
         if (n->id == id && n->pid == pid) break;
    }
    
    if (n == 0) return -1; /* node not present */
    
    log_debug("removing node '%s', id: %u, pid: %u", n->host, n->id, n->pid);
    
    STAILQ_REMOVE(cn_head, n, _clv_clnode_t, next);
    
    clv_clnode_free(n); /* freeing node domains */

    return 0;
}

void group_cpg_deliver(cpg_handle_t handle,
        const struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, size_t msg_len)
{
    clv_group_t *grph = 0;
    clv_cmd_msg_t *c_msg = 0;

    if (cpg_context_get(handle, (void **) &grph) != CPG_OK) {
        return; /* TODO: error? */
    }

    if (msg_len < sizeof(clv_cmd_msg_t)) {
        return; /* TODO: error? */
    }

    c_msg = msg;

    c_msg->cmd          = be_swap32(c_msg->cmd);
    c_msg->token        = be_swap32(c_msg->token);
    c_msg->nodeid       = be_swap32(c_msg->nodeid);
    c_msg->pid          = be_swap32(c_msg->pid);
    c_msg->payload_size = be_swap32(c_msg->payload_size);

    grph->msgcb(c_msg);
}

void group_cpg_confchg(
        cpg_handle_t handle, const struct cpg_name *group_name,
        const struct cpg_address *member_list, size_t member_list_entries,
        const struct cpg_address *left_list, size_t left_list_entries,
        const struct cpg_address *joined_list, size_t joined_list_entries)
{
    size_t i;
    clv_group_t *grph;

    if (cpg_context_get(handle, (void **) &grph) != CPG_OK) {
        return; /* error message */
    }
    
    /* adding member nodes */
    for (i = 0; i < member_list_entries; i++) {
        clv_group_add(&grph->nodes, member_list[i].nodeid, member_list[i].pid);
    }
    
    /* adding joining nodes */
    for (i = 0; i < joined_list_entries; i++) {
        clv_group_add(&grph->nodes, joined_list[i].nodeid, joined_list[i].pid);
    }
    
    /* removing leaving nodes */
    for (i = 0; i < left_list_entries; i++) {
        clv_group_remove(&grph->nodes, left_list[i].nodeid, left_list[i].pid);
    }
}

static cpg_callbacks_t
        _group_callbacks = {
            .cpg_deliver_fn = group_cpg_deliver,
            .cpg_confchg_fn = group_cpg_confchg
        };

int clv_group_init(clv_group_t *grph, clv_msg_callback_fn_t *msgcb)
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

int clv_group_dispatch(clv_group_t *grph)
{
    return (cpg_dispatch(grph->cpg, CPG_DISPATCH_ALL) == CPG_OK) ? 0 : -1;
}

int clv_group_message(clv_group_t *grph, void *buf, size_t len)
{
    struct iovec iov;
    cpg_error_t err;
    int retries = 0;

    iov.iov_base = buf;
    iov.iov_len = len;

retry: 
    err = cpg_mcast_joined(grph->cpg, CPG_TYPE_AGREED, &iov, 1);
    
    if (err == CPG_ERR_TRY_AGAIN) {
        retries++;
        usleep(1000);
        goto retry;
    }
    if (err != CPG_OK) {
        log_error("unable to send message: %d", err);
        return -1;
    }

    if (retries) {
        log_debug("message sent after %d retries", retries);
    }

    return 0;
}

