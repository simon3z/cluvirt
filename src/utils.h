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

#ifndef __UTILS_H_
#define __UTILS_H_

#include <endian.h>
#include <byteswap.h>

#include <syslog.h>


extern int utils_debug;


#define log_debug(fmt, args...) \
if (utils_debug) {  \
    printf("DEBUG(%s:%i|%s): " fmt "\n",                \
           __FILE__, __LINE__, __FUNCTION__, ##args);   \
}

#define log_error(fmt, args...) \
do {    \
    fprintf(stderr, "Error: " fmt "\n", ##args);  \
    syslog(LOG_ERR, fmt, ##args);               \
} while (0)


#if __BYTE_ORDER == __LITTLE_ENDIAN
#define be_swap16(x) bswap_16(x)
#define be_swap32(x) bswap_32(x)
#define be_swap64(x) bswap_64(x)
#else
#define be_swap16(x) (x)
#define be_swap32(x) (x)
#define be_swap64(x) (x)
#endif


#endif
