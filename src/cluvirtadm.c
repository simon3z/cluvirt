/*
cluvirtadm - client for cluvirtd daemons.
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
#include <getopt.h>
#include <errno.h>
#include <error.h>

#include <libcman.h>

#include "status.h"
#include "cluster.h"
#include "member.h"
#include "utils.h"


#define PROGRAM_NAME            "cluvirtadm"
#define CMDLINE_OPT_HELP        0x0001
#define CMDLINE_OPT_VERSION     0x0002
#define CMDLINE_OPT_DEBUG       0x0004
#define CMDLINE_OPT_XMLOUT      0x0008


static cluster_node_head_t  cn_head = STAILQ_HEAD_INITIALIZER(cn_head);

static int cmdline_flags;
static int group_members = -1, group_replies = 0; /* FIXME: something smarter */


void cpg_deliver(cpg_handle_t handle,
        struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, int msg_len)
{
    cluster_node_t      *n;
    
    if (((char*)msg)[0] != 0x00) return; /* FIXME: improve message type */
    
    log_debug("receiving a message: %i", msg_len);
    
    STAILQ_FOREACH(n, &cn_head, next) {
        if (n->id == nodeid) break;
    }
    
    if (n == 0) {
        log_error("cluster node %i not found", nodeid);
        return;
    }
    
    if (n->status & CLUSTER_NODE_JOINED) {
        log_error("double answer from node %i", nodeid);
        return;
    }
    
    n->status |= CLUSTER_NODE_JOINED;
    domain_status_from_msg(&n->domain, msg + 1, msg_len - 1);

    group_replies++;
}

void cpg_confchg(cpg_handle_t handle,
        struct cpg_name *group_name,
        struct cpg_address *member_list, int member_list_entries,
        struct cpg_address *left_list, int left_list_entries,
        struct cpg_address *joined_list, int joined_list_entries)
{
    if ((group_members < 0) || (group_members >= member_list_entries)) {
        group_members = member_list_entries - 1; /* FIXME: something smarter */
    }
}

void request_domain_info()
{
    char request_msg[] = { 0x01 }; /* FIXME: improve message type */
    send_message(request_msg, sizeof(request_msg));
}

char *uuid_to_string(unsigned char *uuid)
{
    static char _uuid_string[VIR_UUID_STRING_BUFLEN];

    snprintf(_uuid_string, VIR_UUID_STRING_BUFLEN,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0],   uuid[1],    uuid[2],    uuid[3],
        uuid[4],   uuid[5],    uuid[6],    uuid[7],
        uuid[8],   uuid[9],    uuid[10],   uuid[11],
        uuid[12],  uuid[13],   uuid[14],   uuid[15]);
    _uuid_string[VIR_UUID_STRING_BUFLEN-1] = '\0';

    return _uuid_string;
}

void print_version()
{
    printf("%s %s\n", PROGRAM_NAME, PACKAGE_VERSION);
    printf(
        "Copyright (C) 2009 Nethesis, Srl.\n"
        "This is free software.  "
        "You may redistribute copies of it under the terms of\n"
        "the GNU General Public License "
        "<http://www.gnu.org/licenses/gpl.html>.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n\n"
        "Written by Federico Simoncelli.\n"
    );
}

void print_usage()
{
    printf("Usage: %s [OPTIONS]\n", PROGRAM_NAME);
    printf(
        "Virtual machines supervisor for openais cluster and libvirt.\n\n"
        "  -h, --help                 display this help and exit\n"
        "      --version              display version information and exit\n"
        "  -x, --xml                  output in xml format\n"
        "  -D                         debug output enabled\n"
    );
}

void cmdline_options(char argc, char *argv[])
{
    int c, option_index = 0;
    static struct option long_options[] = {
        {"help",    no_argument,        0, 'h'},
        {"version", no_argument,        0, 0},
        {"xml",     no_argument,        0, 'x'},
        {0, 0, 0, 0}
    };

    cmdline_flags = 0x00;

    while (1) {    
        c = getopt_long(argc, argv, "hxD", long_options, &option_index);
    
        if (c == -1)
            break;

        switch (c)
        {                    
            case 0: /* version */
                cmdline_flags |= CMDLINE_OPT_VERSION;
                break;

            case 'h':
                cmdline_flags |= CMDLINE_OPT_HELP;
                break;
                
            case 'x':
                cmdline_flags |= CMDLINE_OPT_XMLOUT;
                break;

            case 'D':
                cmdline_flags |= CMDLINE_OPT_DEBUG;
                break;
        }
    }
    
    log_debug("cmdline_flags: 0x%04x", cmdline_flags);
}

void print_status_txt(void)
{
    domain_info_t   *d;
    cluster_node_t  *n;
    char            vm_state;

    static char     state_name[] = {
            'N',    /* VIR_DOMAIN_NOSTATE   =   0 */
            'R',    /* VIR_DOMAIN_RUNNING   =   1 */
            'B',    /* VIR_DOMAIN_BLOCKED	= 	2 */
            'P',    /* VIR_DOMAIN_PAUSED	= 	3 */
            'S',    /* VIR_DOMAIN_SHUTDOWN	= 	4 */
            's',    /* VIR_DOMAIN_SHUTOFF	= 	5 */
            'C'     /* VIR_DOMAIN_CRASHED	= 	6 */
    };

    printf("%4.4s  %-24.24s %-8.8s\n", "ID", "Member Name", "Status");
    printf("%4.4s  %-24.24s %-8.8s\n", "--", "-----------", "------");
        
    STAILQ_FOREACH(n, &cn_head, next) {
        printf("%4i  %-24.24s %s", n->id, n->host,
            (n->status & CLUSTER_NODE_ONLINE) ? "online" : "offline");
        
        if (n->status & CLUSTER_NODE_LOCAL) {
            printf(", %s", "local");
        }
        
        if (n->status & CLUSTER_NODE_JOINED) {
            printf(", %s", "cluvirtd");
        }
        
        printf("\n");
    }
    
    printf("\n");
    printf("%4.4s  %-20.20s %-24.24s %2.2s %6.6s %6.6s %6.6s\n",
                    "ID", "VM Name", "Host", "St", "VNC#", "Mem", "CPU%");
    printf("%4.4s  %-20.20s %-24.24s %2.2s %6.6s %6.6s %6.6s\n",
                    "--", "-------", "----", "--", "----", "---", "----");

    STAILQ_FOREACH(n, &cn_head, next) {
        LIST_FOREACH(d, &n->domain, next) {
            vm_state = (d->state < sizeof(state_name)) ?
                            state_name[d->state] : state_name[0];

            printf("%4i  %-20.20s %-24.24s  %c %6i %6i %4i.%1.1i\n",
                    d->id, d->name, n->host, vm_state, d->vncport,
                    d->memory / 1024, d->usage / 10, d->usage % 10);
        }
    }
}

void print_status_xml(void)
{
    domain_info_t   *d;
    cluster_node_t  *n;

    printf("<?xml version=\"1.0\"?>\n"
           "<%s version=\"%s\">\n<nodes>\n", PROGRAM_NAME, PACKAGE_VERSION);
        
    STAILQ_FOREACH(n, &cn_head, next) {
        printf("  <node id=\"%i\" name=\"%s\" online=\"%i\""
               " local=\"%i\" cluvirtd=\"%i\"/>\n", n->id, n->host, 
            (n->status & CLUSTER_NODE_ONLINE) ? 1 : 0,
            (n->status & CLUSTER_NODE_LOCAL)  ? 1 : 0,
            (n->status & CLUSTER_NODE_JOINED) ? 1 : 0);
    }
    
    printf("</nodes>\n<virtuals>\n");

    STAILQ_FOREACH(n, &cn_head, next) {
        LIST_FOREACH(d, &n->domain, next) {
            printf("  <vm id=\"%i\" uuid=\"%s\"\n"
                   "      name=\"%s\" node=\"%s\" state=\"%i\" vncport=\"%i\""
                   " memory=\"%i\" usage=\"%i\"/>\n",
                        d->id, uuid_to_string(d->uuid), d->name,
                        n->host, d->state, d->vncport, d->memory, d->usage);
        }
    }
    
    printf("</virtuals>\n</%s>\n", PROGRAM_NAME);
}


static cpg_callbacks_t cpg_callbacks = {
    .cpg_deliver_fn = cpg_deliver, .cpg_confchg_fn = cpg_confchg
};

int main_loop(void)
{
    int             fd_csync, fd_max;
    fd_set          readfds;
    struct timeval  select_timeout;
 
    if ((fd_csync = setup_cpg(&cpg_callbacks)) < 0) {
        log_error("unable to initialize openais: %i", errno);
        exit(EXIT_FAILURE);
    }

    member_init_list(&cn_head);

    fd_max = fd_csync + 1;
    FD_ZERO(&readfds);
    
    request_domain_info();

    while (1) {
        select_timeout.tv_sec   = 5;
        select_timeout.tv_usec  = 0;
        
        FD_SET(fd_csync, &readfds);
        
        select(fd_max, &readfds, 0, 0, &select_timeout);
        
        if (FD_ISSET(fd_csync, &readfds)) {
            dispatch_message();
        }
        
        if (group_replies >= group_members) { /* FIXME: something smarter */
            break;
        }
    }

    if (cmdline_flags & CMDLINE_OPT_XMLOUT) {
        print_status_xml();
    }
    else {
        print_status_txt();
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    cmdline_options(argc, argv);

    if (cmdline_flags & CMDLINE_OPT_HELP) {
        print_usage();
        exit(EXIT_SUCCESS);
    }
    
    if (cmdline_flags & CMDLINE_OPT_VERSION) {
        print_version();
        exit(EXIT_SUCCESS);
    }
    
    if (cmdline_flags & CMDLINE_OPT_DEBUG) {
        utils_debug = 0x01;
    }
    
    main_loop();

    return EXIT_SUCCESS; 
}

