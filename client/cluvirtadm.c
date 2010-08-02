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

#include <cluvirt.h>
#include <utils.h>


#define PROGRAM_NAME            "cluvirtadm"
#define CMDLINE_OPT_HELP        0x0001
#define CMDLINE_OPT_VERSION     0x0002
#define CMDLINE_OPT_DEBUG       0x0004
#define CMDLINE_OPT_XMLOUT      0x0008

static int cmdline_flags;
static cluster_node_head_t  cn_head = STAILQ_HEAD_INITIALIZER(cn_head);


int member_init_list(void)
{
    int             node_count, i;
    cman_handle_t   handle;
    cman_node_t     *cman_nodes, local_node;
    
    if ((handle = cman_init(0)) == 0) {
        log_error("unable to initialize cman: %i", errno);
        return -1;
    }
    
    if ((node_count = cman_get_node_count(handle)) < 0) {
        log_error("unable to count cman nodes: %i", errno);
        return -1;
    }
    
    cman_nodes = calloc((size_t) node_count, sizeof(cman_node_t));
    cman_get_nodes(handle, node_count, &node_count, cman_nodes);
    
    if (cman_get_node(handle, CMAN_NODEID_US, &local_node) != 0) {
        log_error("unable to get local node: %i", errno);
        return -1;
    }
    
    for (i = 0; i < node_count; i++) {
        cluster_node_t  *n;
        
        n = malloc(sizeof(cluster_node_t));
        STAILQ_INSERT_TAIL(&cn_head, n, next);

        n->id           = (uint32_t) cman_nodes[i].cn_nodeid;
        n->host         = strdup(cman_nodes[i].cn_name);
        n->status       = 0;
        
        if (cman_nodes[i].cn_member != 0) {
            n->status   |= CLUSTER_NODE_ONLINE;
        }
        
        if (cman_nodes[i].cn_nodeid == local_node.cn_nodeid) {
            n->status   |= CLUSTER_NODE_LOCAL;
        }
        
        LIST_INIT(&n->domain);
    }
    
    free(cman_nodes);
    
    cman_finish(handle);
    
    return 0;
}

void receive_domains(int fd_clv)
{
    ssize_t         msg_len;
    static char     msg[8192];
    clv_cmd_msg_t   asw_cmd;
    cluster_node_t  *n;
    
    msg_len = read(fd_clv, msg, sizeof(msg));

    if (msg_len == 0) {
        log_error("server connection closed");
        return;
    }
    else if (msg_len < 0) {
        log_error("error reading from server: %i", errno);
        return;
    }

    if (clv_rcv_command(&asw_cmd, msg, (size_t) msg_len) == 0) {
        log_error("malformed message, size: %lu", msg_len);
        return;
    }
    
    if (asw_cmd.cmd != CLV_CMD_ANSWER) return;
    
    log_debug("receiving a message: %lu", msg_len);

    STAILQ_FOREACH(n, &cn_head, next) {
        if (n->id == asw_cmd.nodeid) break;
    }

    if (n == 0) {
        log_error("cluster node %i not found", asw_cmd.nodeid);
        return;
    }
    
    if (n->status & CLUSTER_NODE_JOINED) {
        log_error("double answer from node %i", asw_cmd.nodeid);
        return;
    }
    
    n->status |= CLUSTER_NODE_JOINED;
    
    if (clv_domain_from_msg(&n->domain,
            (char *) msg + sizeof(clv_cmd_msg_t),
            (size_t) msg_len - sizeof(clv_cmd_msg_t)) < 0) {
        log_error("bad domain status, output might be incomplete");
    }
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

void cmdline_options(int argc, char *argv[])
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
            vm_state = (char) ((d->state < sizeof(state_name)) ?
                                state_name[d->state] : state_name[0]);

            printf("%4i  %-20.20s %-24.24s  %c %6i %6lu %4i.%1.1i\n",
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
                   " memory=\"%lu\" usage=\"%i\"/>\n",
                        d->id, uuid_to_string(d->uuid), d->name,
                        n->host, d->state, d->vncport, d->memory, d->usage);
        }
    }
    
    printf("</virtuals>\n</%s>\n", PROGRAM_NAME);
}

int main_loop(void)
{
    int             fd_clv;
    fd_set          fds_active, fds_status;
    struct timeval  select_timeout;
    cluster_node_t  *n;
 
    if ((fd_clv = clv_init(CLV_SOCKET, CLV_INIT_CLIENT)) < 0) {
        log_error("unable to initialize cluvirt socket: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    member_init_list();

    FD_ZERO(&fds_active);
    FD_SET(fd_clv, &fds_active);
    
    STAILQ_FOREACH(n, &cn_head, next) {
        select_timeout.tv_sec   = 5;
        select_timeout.tv_usec  = 0;
        
        clv_req_domains(fd_clv, n->id);
        
        fds_status = fds_active;
        select(FD_SETSIZE, &fds_status, 0, 0, &select_timeout);
        
        if (FD_ISSET(fd_clv, &fds_status)) {
            receive_domains(fd_clv);
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

