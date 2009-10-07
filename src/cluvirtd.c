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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <error.h>

#include "status.h"
#include "cluster.h"
#include "utils.h"


#define MESSAGE_BUFFER_SIZE     1024
#define DEVNULL_PATH            "/dev/null"

#define PROGRAM_NAME            "cluvirtd"
#define CMDLINE_OPT_HELP        0x0001
#define CMDLINE_OPT_VERSION     0x0002
#define CMDLINE_OPT_DEBUG       0x0004
#define CMDLINE_OPT_DAEMON      0x0008


static domain_info_head_t  di_head = LIST_HEAD_INITIALIZER();

static int cmdline_flags;
static char *libvirt_uri = 0;


void cpg_deliver(cpg_handle_t handle,
        struct cpg_name *group_name, uint32_t nodeid,
        uint32_t pid, void *msg, int msg_len)
{
    size_t      msg_size;
    char        reply_msg[MESSAGE_BUFFER_SIZE];
    
    if (((char*)msg)[0] == 0x01) { /* FIXME: improve message type */
        reply_msg[0] = 0x00;
        msg_size = domain_status_to_msg(
                        &di_head, &reply_msg[1], MESSAGE_BUFFER_SIZE - 1);
        /* FIXME: better error handling */
        log_debug("sending domain info: %lu", msg_size);
        send_message(reply_msg, msg_size + 1);
    }
}

void cpg_confchg(cpg_handle_t handle,
        struct cpg_name *group_name,
        struct cpg_address *member_list, int member_list_entries,
        struct cpg_address *left_list, int left_list_entries,
        struct cpg_address *joined_list, int joined_list_entries)
{
    return;
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

void cmdline_options(char argc, char *argv[])
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


static cpg_callbacks_t cpg_callbacks = {
    .cpg_deliver_fn = cpg_deliver, .cpg_confchg_fn = cpg_confchg
};


void main_loop(void)
{
    int             fd_csync, fd_max;
    fd_set          readfds;
    struct timeval  select_timeout;
    
    if ((fd_csync = setup_cpg(&cpg_callbacks)) < 0) {
        log_error("unable to initialize openais: %i", errno);
        exit(EXIT_FAILURE);
    }
    
    fd_max = fd_csync + 1;
    FD_ZERO(&readfds);
    
    while (1) {
        select_timeout.tv_sec   = 5;
        select_timeout.tv_usec  = 0;
        
        FD_SET(fd_csync, &readfds);
        
        select(fd_max, &readfds, 0, 0, &select_timeout);

        domain_status_update(libvirt_uri, &di_head);
        
        if (FD_ISSET(fd_csync, &readfds)) {
            dispatch_message();
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

