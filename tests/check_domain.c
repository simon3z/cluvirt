/*
check_domain - cluvirt domain unit test.
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
#include <string.h>
#include <cluvirt.h>
#include <utils.h>

int utils_debug = 0;

void domain_test1()
{
    domain_info_t *d;
    
    if ((d = clv_domain_new("mydomain")) == 0) {
        fprintf(stderr, "unable to create domain\n");
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(d->name, "mydomain") != 0) {
        fprintf(stderr, "unable to set domain name\n");
        exit(EXIT_FAILURE);
    }
    
    if (clv_domain_set_name(d, "mynewdomain") < 0) {
        fprintf(stderr, "unable to change domain name\n");
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(d->name, "mynewdomain") != 0) {
        fprintf(stderr, "unable to properly change domain name\n");
        exit(EXIT_FAILURE);
    }
    
    clv_domain_free(d);
}

int main(int argc, char *argv[])
{
    domain_test1();
    
    return EXIT_SUCCESS;
}
