/*
check_handle - cluvirt handle unit test.
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
#include <cluvirt.h>
#include <utils.h>

#include <chkutils.h>

#define TMPSOCKPATH "/tmp/clvtest1.sock"

void handle_test1()
{
    clv_handle_t clvh;
    
    chkutils_check(clv_init(&clvh, TMPSOCKPATH, CLV_INIT_SERVER) == 0);
    
    clv_finish(&clvh);
    
    chkutils_check(unlink(TMPSOCKPATH) == 0);
}

int main(int argc, char *argv[])
{
    chkutils_test_function(handle_test1);
    
    return EXIT_SUCCESS;
}

