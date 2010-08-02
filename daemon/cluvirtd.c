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
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <error.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <group.h>
#include <domain.h>
#include <cluster.h>
#include <utils.h>


typedef struct _clv_client_t {
    int                         fd;
    struct sockaddr_un          address;
    socklen_t                   addrlen;
    LIST_ENTRY(_clv_client_t)   next;
} clv_client_t;

typedef LIST_HEAD(_clv_client_head_t, _clv_client_t) clv_client_head_t;


#define MESSAGE_BUFFER_SIZE     8192
#define DEVNULL_PATH            "/dev/null"

#define PROGRAM_NAME            "cluvirtd"
#define CMDLINE_OPT_HELP        0x0001
#define CMDLINE_OPT_VERSION     0x0002
#define CMDLINE_OPT_DEBUG       0x0004
#define CMDLINE_OPT_DAEMON      0x0008


static domain_info_head_t  di_head = LIST_HEAD_INITIALIZER(di_head);
static cluster_node_head_t cn_head = STAILQ_HEAD_INITIALIZER(cn_head);
static clv_client_head_t   cl_head = LIST_HEAD_INITIALIZER(cl_head);

static int cmdline_flags;
static char *libvirt_uri = 0;


void cpg_deliver(cpg_handle_t handle,
        const struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, size_t msg_len)
{
    static char     asw_msg[MESSAGE_BUFFER_SIZE];
    ssize_t         msg_size;
    clv_cmd_msg_t   req_cmd, *asw_cmd;

    if (clv_rcv_command(&req_cmd, msg, msg_len) == 0) {
        log_error("malformed command from %u:%u", nodeid, pid);
        return;
    }
    
    if (req_cmd.cmd == CLV_CMD_REQUEST
            && req_cmd.nodeid == get_local_nodeid()) { /* request is for us */
        asw_cmd         = (clv_cmd_msg_t *) asw_msg;
        
        asw_cmd->cmd    = be_swap32(CLV_CMD_ANSWER);
        asw_cmd->token  = 0; /* FIXME: unused */
        asw_cmd->nodeid = be_swap32(get_local_nodeid());
        asw_cmd->pid    = 0; /* FIXME: unused */

        if ((msg_size = clv_domain_to_msg(
                        &di_head, asw_msg + sizeof(clv_cmd_msg_t),
                        MESSAGE_BUFFER_SIZE - sizeof(clv_cmd_msg_t))) < 0) {
            log_error("unable to prepare domain status message");
            return;
        }

        /* FIXME: better error handling */
        log_debug("sending domain info: %lu", msg_size);
        
        send_message(asw_msg, (size_t) msg_size + sizeof(clv_cmd_msg_t));
    }
    else if (req_cmd.cmd == CLV_CMD_ANSWER) { /* delivering, FIXME: token */
        clv_client_t *cl;
        
        LIST_FOREACH(cl, &cl_head, next) {
            write(cl->fd, msg, msg_len);
        }
    }
}

void cpg_confchg(cpg_handle_t handle, const struct cpg_name *group_name,
        const struct cpg_address *member_list, size_t member_list_entries,
        const struct cpg_address *left_list, size_t left_list_entries,
        const struct cpg_address *joined_list, size_t joined_list_entries)
{
    size_t i;
    
    /* adding member nodes */
    for (i = 0; i < member_list_entries; i++) {
        group_node_add(&cn_head, member_list[i].nodeid, member_list[i].pid);
    }
    
    /* adding joining nodes */
    for (i = 0; i < joined_list_entries; i++) {
        group_node_add(&cn_head, joined_list[i].nodeid, joined_list[i].pid);
    }
    
    /* removing leaving nodes */
    for (i = 0; i < left_list_entries; i++) {
        group_node_remove(&cn_head, left_list[i].nodeid, left_list[i].pid);
    }
}

void fork_daemon(void)
{
    pid_t pid, sid, devnull;

    pid = fork();
    
    if (pid < 0) {
        log_error("unable to fork the daemon: %i", errno);
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    if ((sid = setsid()) < 0) {
        log_error("unable to create a new session: %i", errno);
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
        log_error("unable to change working directory: %i", errno);
        exit(EXIT_FAILURE);
    }

    if ((devnull = open(DEVNULL_PATH, O_RDWR)) < 0) {
        log_error("unable to open %s: %i", DEVNULL_PATH, errno);
    }
    
    dup2(devnull, 0);
    dup2(devnull, 1);
    dup2(devnull, 2);
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

void print_usage()
{
    printf("Usage: %s [OPTIONS]\n", PROGRAM_NAME);
    printf("Virtual machines supervisor for openais cluster and libvirt.\n\n");
    printf("  -h, --help                 display this help and exit\n");
    printf("      --version              " \
           "display version information and exit\n");
    printf("  -u, --uri=URI              libvirt connection URI\n");
    printf("  -f                         no daemon, run in foreground\n");
    printf("  -D                         debug output enabled\n");
    printf("\nReport bugs to <%s>.\n", PACKAGE_BUGREPORT);
}

void cmdline_options(int argc, char *argv[])
{
    int c, option_index = 0;
    static struct option long_options[] = {
        {"help",    no_argument,        0, 'h'},
        {"version", no_argument,        0, 0},
        {"uri",     required_argument,  0, 'u'},
        {0, 0, 0, 0}
    };

    cmdline_flags = CMDLINE_OPT_DAEMON;

    while (1) {    
        c = getopt_long(argc, argv, "hfDu:", long_options, &option_index);
    
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
            
            case 'f':
                cmdline_flags &= ~CMDLINE_OPT_DAEMON;
                break;
            
            case 'D':
                cmdline_flags |= CMDLINE_OPT_DEBUG;
                break;

            case 'u':
                libvirt_uri = strdup(optarg);
                break;
        }
    }
    
    log_debug("cmdline_flags: 0x%04x", cmdline_flags);
    log_debug("libvirt_uri: %s", libvirt_uri);
}

int dispatch_request(clv_client_t *cl)
{
    clv_cmd_msg_t   req_cmd, asw_cmd;
    cluster_node_t  *n;
    ssize_t         msg_len;
    static char     msg[MESSAGE_BUFFER_SIZE]; /* FIXME: one buffer */
    
    msg_len = read(cl->fd, msg, MESSAGE_BUFFER_SIZE);

    if (msg_len == 0) {
        return -1; /* client disconnection */
    }
    else if (msg_len < 0) {
        log_error("error reading from client");
        return 0; /* FIXME: better error handling */
    }
    
    if (clv_rcv_command(&req_cmd, msg, (size_t) msg_len) == 0) {
        log_error("malformed command from client: %p", cl);
        return 0; /* FIXME: better error handling */
    }

    STAILQ_FOREACH(n, &cn_head, next) { /* FIXME: performances */
         if (n->id == req_cmd.nodeid) break;
    }
    
    if (n != 0) {
        send_message(msg, (size_t) msg_len); /* dispatch message to group */
    }
    else {
        asw_cmd.cmd             = CLV_CMD_ERROR;
        asw_cmd.token           = 0;
        asw_cmd.nodeid          = be_swap32(get_local_nodeid());
        asw_cmd.pid             = 0;
        
        write(cl->fd, &asw_cmd, sizeof(asw_cmd));
    }
    
    return 0;
}

static cpg_callbacks_t cpg_callbacks = {
    .cpg_deliver_fn = cpg_deliver, .cpg_confchg_fn = cpg_confchg
};

void main_loop(void)
{
    int             fd_cpg, fd_clv;
    fd_set          fds_active, fds_status;
    struct timeval  select_timeout;
    clv_handle_t    clvh;
    
    if ((fd_cpg = setup_cpg(&cpg_callbacks)) < 0) {
        log_error("unable to initialize openais: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    if (clv_init(&clvh, CLV_SOCKET, CLV_INIT_SERVER) != 0) {
        log_error("unable to initialize cluvirt socket: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    fd_clv = clv_get_fd(&clvh);
    
    group_init();

    FD_ZERO(&fds_active);
    FD_SET(fd_cpg, &fds_active);
    FD_SET(fd_clv, &fds_active);
    
    while (1) {
        clv_client_t *cl, *j;
        
        select_timeout.tv_sec   = 5;
        select_timeout.tv_usec  = 0;
        
        fds_status = fds_active;
        select(FD_SETSIZE, &fds_status, 0, 0, &select_timeout);

        domain_update_status(libvirt_uri, &di_head);
        
        if (FD_ISSET(fd_cpg, &fds_status)) { /* dispatching cpg messages */
            dispatch_message();
        }
        else if (FD_ISSET(fd_clv, &fds_status)) { /* accepting connections */
            cl = malloc(sizeof(clv_client_t));
            
            cl->addrlen = sizeof(cl->address);
            cl->fd = accept(
                    fd_clv, (struct sockaddr *) &cl->address, &cl->addrlen);
            
            if (cl->fd < 0) {
                free(cl);
                log_error("unable to accept connection: %i", errno);
            }
            else {
                LIST_INSERT_HEAD(&cl_head, cl, next);
                FD_SET(cl->fd, &fds_active);
                log_debug("client connection accepted: %p", cl);
            }
        }
        else { /* dispatching requests */
            for (cl = j = LIST_FIRST(&cl_head); j; cl = j) {
                j = LIST_NEXT(cl, next);

                if (FD_ISSET(cl->fd, &fds_status)) {
                    int dispatch_status;
                    
                    dispatch_status = dispatch_request(cl);
                    
                    if (dispatch_status < 0) { /* closing connections */
                        close(cl->fd);

                        FD_CLR(cl->fd, &fds_active);
                        LIST_REMOVE(cl, next);
                        
                        log_debug("client connection closed: %p", cl);
                        free(cl);
                    }
                }
            }
        }
    }
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

    if (cmdline_flags & CMDLINE_OPT_DAEMON) {
        fork_daemon();
    }
    
    if (cmdline_flags & CMDLINE_OPT_DEBUG) {
        utils_debug = 0x01;
    }
    
    main_loop();

    return EXIT_SUCCESS; 
}

