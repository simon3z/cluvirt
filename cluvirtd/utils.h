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

#include <syslog.h>

#define log_debug(x, t...) \
do { \
    printf("DEBUG(%s:%i|%s): " x "\n", __FILE__, __LINE__, __FUNCTION__, t); \
} while(0);

#define log_error(fmt, args...) \
do { \
    log_debug(fmt, ##args); \
    syslog(LOG_ERR, fmt, ##args); \
} while (0)


#endif
