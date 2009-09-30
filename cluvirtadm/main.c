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


static cluster_node_head_t  cn_head = STAILQ_HEAD_INITIALIZER(cn_head);

static int cmdline_flags;
static int group_members = -1, group_answers = 0; /* FIXME: something smarter */


void cpg_deliver(cpg_handle_t handle,
        struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, int msg_len)
{
    domain_info_t       *d;
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
    domain_status_from_msg(&n->domain, msg + 1, msg_len);

    group_answers++;
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
    char request_msg[] = { 0x01 };
    send_message(request_msg, sizeof(request_msg));
}

void print_version()
{
    printf("%s %s\n", PROGRAM_NAME, PACKAGE_VERSION);
    printf("Copyright (C) 2009 Nethesis, Srl.\n");
    printf("This is free software.  " \
           "You may redistribute copies of it under the terms of\n");
    printf("the GNU General Public License "\
           "<http://www.gnu.org/licenses/gpl.html>.\n");
    printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
    printf("Written by Federico Simoncelli.\n");
}

void print_usage(char *binpath)
{
    printf("Usage: %s [OPTIONS]\n", binpath);
    printf("Virtual machines supervisor for openais cluster and libvirt.\n\n");
    printf("  -h, --help                 display this help and exit\n");
    printf("      --version              " \
           "display version information and exit\n");
    printf("  -D                         debug output enabled\n");
}

void cmdline_options(char argc, char *argv[])
{
    int c, option_index = 0;
    static struct option long_options[] = {
        {"help",    no_argument,        0, 'h'},
        {"version", no_argument,        0, 0},
        {0, 0, 0, 0}
    };

    cmdline_flags = 0x00;

    while (1) {    
        c = getopt_long(argc, argv, "hD", long_options, &option_index);
    
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

            case 'D':
                cmdline_flags |= CMDLINE_OPT_DEBUG;
                break;
        }
    }
    
    log_debug("cmdline_flags: 0x%04x", cmdline_flags);
}

void print_status()
{
    domain_info_t   *d;
    cluster_node_t  *n;

    printf("%6.6s  %-24.24s %-8.8s\n", "ID", "Host", "Status");
    printf("%6.6s  %-24.24s %-8.8s\n", "--", "----", "------");
        
    STAILQ_FOREACH(n, &cn_head, next) {
        printf("%6.0i  %-24.24s ", n->id, n->host);
        
        printf((n->status & CLUSTER_NODE_ONLINE) ? "online" : "offline");
        
        if (n->status & CLUSTER_NODE_LOCAL) {
            printf(", local");
        }
        
        if (n->status & CLUSTER_NODE_JOINED) {
            printf(", cluvirtd");
        }
        
        printf("\n");
    }
    
    printf("\n");
    printf("%6.6s  %-24.24s %-24.24s %6.6s %8.8s\n",
                    "ID", "Name", "Host", "VNC#", "CPU%");
    printf("%6.6s  %-24.24s %-24.24s %6.6s %8.8s\n",
                    "--", "----", "----", "----", "----");

    STAILQ_FOREACH(n, &cn_head, next) {
        LIST_FOREACH(d, &n->domain, next) {
            printf("%6.0i  %-24.24s %-24.24s %6.0i %8i\n",
                d->id, d->name, n->host, d->status.vncport, d->status.usage);
        }
    }
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
        
        if (group_answers >= group_members) { /* FIXME: something smarter */
            break;
        }
    }

    print_status();
    
    return 0;
}

int main(int argc, char *argv[])
{
    cmdline_options(argc, argv);

    if (cmdline_flags & CMDLINE_OPT_HELP) {
        print_usage(argv[0]);
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

