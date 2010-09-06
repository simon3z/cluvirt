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

#include <readline/history.h>
#include <readline/readline.h>

#include <member.h>
#include <interactive.h>

#include <cluvirt.h>
#include <utils.h>


#define CMDLINE_ARGS_MAXLEN     (64)
#define CMDLINE_ERROR_MAXLEN    (255)

#define cmdline_error(fmt, args...)  fprintf(stderr, fmt "\n", ##args);


typedef int cmdline_fn_t(int argc, char *argv[]);

typedef struct _cmdline_function_t {
    char            *cmd;
    cmdline_fn_t    *fun;
    char            *help;
} cmdline_function_t;


static int cmdline_do_list_nodes(int argc, char *argv[])
{
    clv_clnode_t        *n;
    clv_clnode_head_t   nodes = STAILQ_HEAD_INITIALIZER(nodes);

    if (member_list_nodes(&nodes) < 0) {
        cmdline_error("%s: unable to get cluster members", argv[1]);
    }

    /* TODO */

    return 0;
}

static int cmdline_do_list_vm(int argc, char *argv[])
{
    /* TODO */
    return 0;
}

static int cmdline_do_list(int argc, char *argv[])
{
    if (argc < 2) goto fail_args;

    if (strcmp(argv[1], "nodes") == 0) {
        if (argc != 2) goto fail_args;
        return cmdline_do_list_nodes(argc, &argv[1]);
    }
    else if (strcmp(argv[1], "vm") == 0) {
        if (argc != 3) goto fail_args;
        return cmdline_do_list_vm(argc, &argv[1]);
    }

    cmdline_error("%s: unknown list target", argv[1]);
    return -1;

fail_args:

    cmdline_error("%s: incorrect number of arguments", argv[0]);
    return -1;
}

static cmdline_function_t _cmd_table[] = {
    { "list", cmdline_do_list, 0},
    { 0, 0 }
};

static int cmdline_exec(char *line)
{
    int argc, i;
    char *argv[CMDLINE_ARGS_MAXLEN], *saveptr;
    
    if ((argv[0] = strtok_r(line, " \t", &saveptr)) == 0) {
        return 0;
    }

    argc = 1;

    while ((argv[argc] = strtok_r(0, " \t", &saveptr)) != 0) {
        argc = argc + 1;
    }

    for (i = 0; _cmd_table[i].cmd != 0; i++) {
        if (strcmp(argv[0], _cmd_table[i].cmd) == 0) {
            return _cmd_table[i].fun(argc, argv);
        }
    }

    cmdline_error("%s: command not found", argv[0]);
    return -1;
}

int interactive_loop(void)
{
    char *line;
    clv_handle_t clvh;

    if (clv_init(&clvh, CLV_SOCKET_PATH) < 0) {
        log_error("unable to initialize cluvirt socket: %i", errno);
        exit(EXIT_FAILURE);
    }

    while((line = readline("clv> ")) != 0) {
        add_history(line);
        cmdline_exec(line);
    }

    printf("\n");
    return 0;
}
