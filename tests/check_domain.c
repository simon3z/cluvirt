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

char domain_test2_data[] = {
    0x00,   0x00,   0x00,   0x01,   /* id */
    0x00,   0x00,   0x04,   0x00,   /* memory */
    0x00,   0x00,   0x04,   0x00,   /* cputime 1 */
    0x00,   0x00,   0x04,   0x00,   /* cputime 2 */
    0x02,   0x34,                   /* usage */
    0x00,   0x02,                   /* ncpu */
    0x17,   0x0D,                   /* vncport */
    0x01,                           /* state */
            0x00,   0x00,   0x00,   /* padding 1 */
    0x00,   0x00,   0x00,   0x00,   /* padding 2 */
    0x00,   0x00,   0x00,   0x01,   /* uuid 1 */
    0x00,   0x00,   0x00,   0x02,   /* uuid 2 */
    0x00,   0x00,   0x00,   0x03,   /* uuid 3 */
    0x00,   0x00,   0x00,   0x04,   /* uuid 2 */
    0x00,   0x00,   0x00,   0x08,   /* payload_size */
    'v', 'm', 't', 'e', 's', 't', '1', '\0',
    0x00,   0x00,   0x00,   0x02,   /* id */
    0x00,   0x00,   0x08,   0x00,   /* memory */
    0x00,   0x00,   0x02,   0x00,   /* cputime 1 */
    0x00,   0x00,   0x02,   0x00,   /* cputime 2 */
    0x02,   0x00,                   /* usage */
    0x00,   0x01,                   /* ncpu */
    0x17,   0x0E,                   /* vncport */
    0x01,                           /* state */
            0x00,   0x00,   0x00,   /* padding 1 */
    0x00,   0x00,   0x00,   0x00,   /* padding 2 */
    0x00,   0x00,   0x00,   0x05,   /* uuid 1 */
    0x00,   0x00,   0x00,   0x06,   /* uuid 2 */
    0x00,   0x00,   0x00,   0x07,   /* uuid 3 */
    0x00,   0x00,   0x00,   0x08,   /* uuid 2 */
    0x00,   0x00,   0x00,   0x08,   /* payload_size */
    'v', 'm', 't', 'e', 's', 't', '2', '\0',
};

void domain_test1()
{
    clv_vminfo_t  *d;

    chkutils_check((d = clv_vminfo_new("mydomain")) != 0);
    
    chkutils_check(strcmp(d->name, "mydomain") == 0);
    chkutils_check(clv_vminfo_set_name(d, "mynewdomain") == 0);
    chkutils_check(strcmp(d->name, "mynewdomain") == 0);
    
    clv_vminfo_free(d);
}

void domain_test2()
{
    clv_vminfo_t *v;
    clv_vminfo_head_t di_head = LIST_HEAD_INITIALIZER(di_head);
    
    chkutils_check(
        clv_vminfo_from_msg(
            &di_head, domain_test2_data, sizeof(domain_test2_data)) > 0
    );
    
    while ((v = LIST_FIRST(&di_head)) != 0) {
        LIST_REMOVE(v, next);
        clv_vminfo_free(v);
    }
}

int main(int argc, char *argv[])
{
    chkutils_test_function(domain_test1);
    chkutils_test_function(domain_test2);
    
    return EXIT_SUCCESS;
}

