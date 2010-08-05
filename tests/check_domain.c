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

#include <chkutils.h>

void domain_test1()
{
    clv_vminfo_t  *d;

    chkutils_check((d = clv_vminfo_new("mydomain")) != 0);
    
    chkutils_check(strcmp(d->name, "mydomain") == 0);
    chkutils_check(clv_vminfo_set_name(d, "mynewdomain") == 0);
    chkutils_check(strcmp(d->name, "mynewdomain") == 0);
    
    clv_vminfo_free(d);
}

int main(int argc, char *argv[])
{
    chkutils_test_function(domain_test1);   
    return EXIT_SUCCESS;
}

