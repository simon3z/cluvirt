/*
check_domain - cluvirt clnode unit test.
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

#include <chkutils.h>

void clnode_test1()
{
    int i;
    clv_clnode_t *n;
    clv_vminfo_t *d;
    
    chkutils_check((n = clv_clnode_new("testnode", 1, 1)) != 0);
    
    for (i = 0; i < 10; i++) {
        chkutils_check((d = clv_vminfo_new("testvm")) != 0);
        LIST_INSERT_HEAD(&n->domain, d, next);
    }
    
    clv_clnode_free(n);
}

void clnode_test2()
{
    clv_clnode_t *n;
    
    chkutils_check((n = clv_clnode_new("testnode", 1, 1)) != 0);
    clv_clnode_free(n);
}

int main(int argc, char *argv[])
{
    chkutils_test_function(clnode_test1);
    chkutils_test_function(clnode_test2);
    
    return EXIT_SUCCESS;
}

