/*
chkutils.h - cluvirt unit test utility.
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

#ifndef __CHKUTILS_H_
#define __CHKUTILS_H_

#define chkutils_check(expr) do {                   \
    if (!(expr)) {                                  \
        fprintf(stderr, "FAIL: %s:%i `%s'\n",__FILE__, __LINE__, #expr);    \
        exit(-1);                                   \
    }                                               \
} while (/*CONSTCOND*/0)

#define chkutils_test_function(fun) do {            \
    fprintf(stdout, "=> running %s()\n", #fun);     \
    fun();                                          \
} while (/*CONSTCOND*/0)

#endif
