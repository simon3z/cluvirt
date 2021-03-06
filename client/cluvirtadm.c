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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <error.h>

#include <libcman.h>

#include <member.h>
#include <interactive.h>

#include <cluvirt.h>
#include <utils.h>


#define PROGRAM_NAME            "cluvirtadm"
#define CMDLINE_OPT_HELP        0x0001
#define CMDLINE_OPT_VERSION     0x0002
#define CMDLINE_OPT_DEBUG       0x0004
#define CMDLINE_OPT_XMLOUT      0x0008
#define CMDLINE_OPT_INTERACTIVE 0x0010


typedef struct _clvadm_vmtable_t {
    uint32_t                        nodeid;
    clv_vminfo_head_t               vmlist;
    LIST_ENTRY(_clvadm_vmtable_t)   next;
} clvadm_vmtable_t;

typedef LIST_HEAD(
            _clvadm_vmtable_head_t, _clvadm_vmtable_t) clvadm_vmtable_head_t;


static int cmdline_flags;
static clv_clnode_head_t     cn_head = STAILQ_HEAD_INITIALIZER(cn_head);
static clvadm_vmtable_head_t vi_head = LIST_HEAD_INITIALIZER(vi_head);


char *uuid_to_string(unsigned char *uuid)
{
    static char _uuid_string[CLUVIRT_UUID_STRING_BUFLEN];

    snprintf(_uuid_string, CLUVIRT_UUID_STRING_BUFLEN,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0],   uuid[1],    uuid[2],    uuid[3],
        uuid[4],   uuid[5],    uuid[6],    uuid[7],
        uuid[8],   uuid[9],    uuid[10],   uuid[11],
        uuid[12],  uuid[13],   uuid[14],   uuid[15]);
    _uuid_string[CLUVIRT_UUID_STRING_BUFLEN-1] = '\0';

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
        "  -h, --help               display this help and exit\n"
        "      --version            display version information and exit\n"
        "  -x, --xml                output in xml format\n"
        "  -i, --interactive        run in interactive mode\n"
        "  -D                       debug output enabled\n"
    );
}

void cmdline_options(int argc, char *argv[])
{
    int c, option_index = 0;
    static struct option long_options[] = {
        {"help",    no_argument,    0, 'h'},
        {"version", no_argument,    0, 0},
        {"xml",     no_argument,    0, 'x'},
        {"interactive", no_argument,    0, 'i'},
        {0, 0, 0, 0}
    };

    cmdline_flags = 0x00;

    while (1) {    
        c = getopt_long(argc, argv, "hxiD", long_options, &option_index);
    
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

            case 'i':
                cmdline_flags |= CMDLINE_OPT_INTERACTIVE;
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
    clv_vminfo_t    *d;
    clv_clnode_t    *n;
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
        clvadm_vmtable_t *t;
        
        LIST_FOREACH(t, &vi_head, next) {   /* FIXME: preformances */
            if (t->nodeid == n->id) break;
        }
        
        if (t == 0) continue;
        
        LIST_FOREACH(d, &t->vmlist, next) {
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
    clv_vminfo_t    *d;
    clv_clnode_t    *n;

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
        clvadm_vmtable_t *t;
        
        LIST_FOREACH(t, &vi_head, next) {   /* FIXME: preformances */
            if (t->nodeid == n->id) break;
        }
        
        if (t == 0) continue;
        
        LIST_FOREACH(d, &t->vmlist, next) {
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
    clv_handle_t    clvh;
    clv_clnode_t    *n;

    if (member_list_nodes(&cn_head) < 0) {
        log_error("unable to get cluster members: %i", errno);
        exit(EXIT_FAILURE);
    }

    if (clv_init(&clvh, CLV_SOCKET_PATH) < 0) {
        log_error("unable to initialize cluvirt socket: %i", errno);
        exit(EXIT_FAILURE);
    }

    STAILQ_FOREACH(n, &cn_head, next) {
        clvadm_vmtable_t *t = malloc(sizeof(clvadm_vmtable_t));
        
        t->nodeid = n->id;
        LIST_INIT(&t->vmlist);
        
        if (clv_fetch_vminfo(&clvh, n->id, &t->vmlist) == 0) {
            n->status |= CLUSTER_NODE_JOINED;
        }
        
        LIST_INSERT_HEAD(&vi_head, t, next);
    }

    if (cmdline_flags & CMDLINE_OPT_XMLOUT) {
        print_status_xml();
    }
    else {
        print_status_txt();
    }
    
    while((n = STAILQ_FIRST(&cn_head)) != 0) {
        STAILQ_REMOVE(&cn_head, n, _clv_clnode_t, next);
        clv_clnode_free(n);
    }
    
    clv_finish(&clvh);
    
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
    
    if (cmdline_flags & CMDLINE_OPT_INTERACTIVE) {
        interactive_loop();
    }
    else {
        main_loop();
    }

    return EXIT_SUCCESS; 
}

