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

#ifndef __CLUVIRT_H_
#define __CLUVIRT_H_


#include <stdint.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <libvirt/libvirt.h> /* VIR_UUID_BUFLEN */

#ifdef HAVE_COROSYNC_CPG_H
#include <corosync/cpg.h>
#else
#include <openais/cpg.h>
#endif

typedef struct _clv_vminfo_t {
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
    LIST_ENTRY(_clv_vminfo_t)   next;
} clv_vminfo_t;

typedef LIST_HEAD(_clv_vminfo_head_t, _clv_vminfo_t) clv_vminfo_head_t;

/* big-endian structure */
typedef struct __attribute__ ((__packed__)) _clv_vminfo_msg_t {
    uint32_t                    id;
    uint32_t                    memory;
    uint64_t                    cputime;
    uint16_t                    usage;
    uint16_t                    ncpu;
    uint16_t                    vncport;
    uint8_t                     state;
    uint8_t                     __pad[7]; /* padding */
    uint8_t                     uuid[VIR_UUID_BUFLEN];
    uint32_t                    payload_size;
    uint8_t                     payload[0];
} clv_vminfo_msg_t;

#define CLUSTER_NODE_ONLINE     0x0001
#define CLUSTER_NODE_LOCAL      0x0002
#define CLUSTER_NODE_JOINED     0x0004

typedef struct _clv_clnode_t {
    uint32_t                        id;
    uint32_t                        pid;
    char                            *host;
    int                             status;
    STAILQ_ENTRY(_clv_clnode_t)     next;
} clv_clnode_t;

typedef STAILQ_HEAD(_clv_clnode_head_t, _clv_clnode_t) clv_clnode_head_t;

#define CLV_CMD_ERROR           0xffffffff
#define CLV_CMD_REQVMINFO       0x00000001
#define CLV_CMD_ANSVMINFO       0xff000001
#define CLV_CMD_REQDESTROYVM    0x00000002
#define CLV_CMD_ANSDESTROYVM    0xff000002

/* big-endian structure */
typedef struct __attribute__ ((__packed__)) _clv_cmd_msg_t {
    uint32_t        cmd;
    uint32_t        token;
    uint32_t        nodeid;
    uint32_t        pid;
    uint32_t        __pad;
    uint32_t        payload_size;
    uint8_t         payload[0];
} clv_cmd_msg_t;

#define CLV_INIT_CLIENT     0x00
#define CLV_INIT_SERVER     0x01
#define CLV_SOCKET          "/var/run/cluvirt.sock"

typedef struct _clv_handle_t {
    int             fd;
    long            to_sec;
    long            to_usec;
    void            *reply;
    size_t          reply_len;
} clv_handle_t;

typedef int clv_msg_callback_fn_t(clv_cmd_msg_t *);

typedef struct _clv_group_t {
    int                     fd;
    cpg_handle_t            cpg;
    struct cpg_name         name;
    uint32_t                nodeid;
    clv_clnode_head_t       nodes;
    clv_msg_callback_fn_t   *msgcb;
} clv_group_t;

int clv_init(clv_handle_t *, const char *, int);
void clv_finish(clv_handle_t *);

int clv_get_fd(clv_handle_t *);
int clv_fetch_vminfo(clv_handle_t *, uint32_t, clv_vminfo_head_t *);
void *clv_rcv_command(clv_cmd_msg_t *, void *, size_t);

clv_vminfo_t *clv_vminfo_new(const char *);
void clv_vminfo_free(clv_vminfo_t *);

int clv_vminfo_set_name(clv_vminfo_t *, const char *);

ssize_t clv_vminfo_to_msg(clv_vminfo_head_t *, char *, size_t);
ssize_t clv_vminfo_from_msg(clv_vminfo_head_t *, char *, size_t);

clv_clnode_t *clv_clnode_new(const char *, uint32_t, uint32_t);
void clv_clnode_free(clv_clnode_t *);

#endif
