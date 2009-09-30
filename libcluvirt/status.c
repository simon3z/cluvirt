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
#include <time.h>
#include <errno.h>
#include <error.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "status.h"
#include "utils.h"


#define XPATH_VNC_PORT          \
    "string(/domain/devices/graphics[@type='vnc']/@port)"

#define XEN_HOST_DOMAIN         "Domain-0"
#define LIBVIRT_ID_BUFFER_SIZE  64


static int _libvirt_vncport(virDomain *);


int domain_status_update(char *uri, domain_info_head_t *di_head)
{
    int             id[LIBVIRT_ID_BUFFER_SIZE], n, i;
    virConnect      *lvh;
    domain_info_t   *d, *j;
    struct timeval  time_now;
    
    if ((lvh = virConnectOpen(uri)) == 0) {
        log_error("unable to connect to libvirtd: %i", errno);
        return -1;
    }
    
    if ((n = virConnectNumOfDomains(lvh)) < 0) {
        log_error("unable to count the domains: %i", n);
        goto clean_exit1;
    }
    
    if (n > LIBVIRT_ID_BUFFER_SIZE) {
        log_error("id buffer size not large enough: %i", n);
        goto clean_exit1;
    }

    if ((n = virConnectListDomains(lvh, id, n)) < 0) {
        log_error("unable to fetch domains list: %i", n);
        goto clean_exit1;
    }
    
    if (gettimeofday(&time_now, 0) < 0) {
        log_error("unable to get time of day: %i", errno);
        goto clean_exit1;
    }
    
    for (i = 0; i < n; i++) {
        virDomain           *lv_domain;
        virDomainInfo       lv_info;
        const char          *vm_name;
        unsigned long long  time_delta;
        
        if ((lv_domain = virDomainLookupByID(lvh, id[i])) == 0) {
            log_error("unable to lookup domain %i", id[i]);
            continue;
        }
        
        if (virDomainGetInfo(lv_domain, &lv_info) < 0) {
            log_error("unable to get info for domain %i", id[i]);
            goto clean_loop1;
        }
        
        vm_name = virDomainGetName(lv_domain);
        
        /* skipping Domain-0 for xen hypervisor, FIXME: URI check? */
        if (vm_name && !strcmp(vm_name, XEN_HOST_DOMAIN)) {
            goto clean_loop1;
        }
        
        LIST_FOREACH(d, di_head, next) {
            if (d->id == id[i]) break;
        }
        
        if (d == 0) {
            log_debug("adding new domain info: %i", id[i]);
            
            d = malloc(sizeof(domain_info_t));
            LIST_INSERT_HEAD(di_head, d, next);
            
            d->id               = id[i];
            
            /* the following values won't be updated anymore */
            d->name             = (vm_name == 0) ? 
                                    strdup("(unknown)") : strdup(vm_name);

            if (virDomainGetUUID(lv_domain, d->uuid) < 0) {
                memset(d->uuid, 0, VIR_UUID_BUFLEN);
                log_error("unable to get domain uuid %i", id[i]);
            }

            d->status.memory    = lv_info.memory;
            d->status.ncpu      = lv_info.nrVirtCpu;
            d->status.vncport   = _libvirt_vncport(lv_domain);
            
            /* initializing cpu load parameters */
            d->status.usage     = -1;
            d->status.cputime   = lv_info.cpuTime;
            d->update.tv_sec    = time_now.tv_sec;
            d->update.tv_usec   = time_now.tv_usec;
        }

        /* these values will be updated at each call */
        d->status.state         = lv_info.state;
        
        time_delta = ((time_now.tv_sec - d->update.tv_sec) * 1000000ull) +
                        (time_now.tv_usec - d->update.tv_usec);
        
        /* update cpu utilization every 10 seconds */
        if (time_delta > (10 * 1000000ull)) { /* > 0 to avoid divide-by-zero */
            d->status.usage     = ((lv_info.cpuTime - d->status.cputime) /
                                    lv_info.nrVirtCpu) / (time_delta * 10);

            d->status.cputime   = lv_info.cpuTime;
            d->update.tv_sec    = time_now.tv_sec;
            d->update.tv_usec   = time_now.tv_usec;
        }

clean_loop1:
        virDomainFree(lv_domain);
    }
    
    /* cleaning up old domains */
    for (j = d = LIST_FIRST(di_head); d; j = d) {
        d = LIST_NEXT(d, next);
        
        for (i = 0; i < n; i++) {
            if (j->id == id[i]) break;
        }
        
        if (i >= n) {
            log_debug("removing old domain info: %i", j->id);
            LIST_REMOVE(j, next);
            free(j->name);
            free(j);
        }
    }

clean_exit1:
    if (virConnectClose(lvh) != 0) {
        log_error("error closing libvirt: %p", lvh);
    }

    return 0;
}

static int _libvirt_vncport(virDomain *domain)
{
    char                *doc;
    xmlDoc              *xml;
    xmlXPathContext     *ctxt;
    xmlXPathObject      *obj;
    int                 port = -1;
    
    if ((doc = virDomainGetXMLDesc(domain, VIR_DOMAIN_XML_SECURE)) == 0) {
        log_error("failed to dump xml: %i", errno);
        return -1;
    }

    if ((xml = xmlReadDoc((const xmlChar *) doc, "domain.xml", NULL,
                 XML_PARSE_NOENT | XML_PARSE_NONET |
                 XML_PARSE_NOWARNING)) == 0) {
        log_error("failed to parse document: %i", errno);
        goto clean_exit1;
    }

    if ((ctxt = xmlXPathNewContext(xml)) == 0) {
        log_error("failed to create context: %i", errno);
        goto clean_exit2;
    }
    
    if ((obj = xmlXPathEval((const xmlChar*) XPATH_VNC_PORT, ctxt)) == 0) {
        log_error("failed to create object: %i", errno);
        goto clean_exit3;
    }
    
    port = atoi((char*) obj->stringval);

    xmlXPathFreeObject(obj);

clean_exit3:
    xmlXPathFreeContext(ctxt);

clean_exit2:
    xmlFreeDoc(xml);

clean_exit1:
    free(doc);

    return port;
}

size_t domain_status_to_msg(
        domain_info_head_t *di_head, char *msg, size_t max_size)
{
    size_t          p_offset = 0;
    domain_info_t   *d;
    
    /* FIXME: endianness */
    
    LIST_FOREACH(d, di_head, next) {
        size_t      name_len;
        size_t      n_offset;
        
        name_len    = strlen(d->name) + 1;
        n_offset    = p_offset + sizeof(domain_info_t) + name_len;
        
        if (n_offset > max_size) {
            log_error("message buffer is not large enough: %lu", n_offset);
            return -1;
        }
        
        memcpy(msg + p_offset, d, sizeof(domain_info_t));
        p_offset += sizeof(domain_info_t);
        
        memcpy(msg + p_offset, d->name, name_len);
        
        p_offset = n_offset;
    }
    
    return p_offset;
}

size_t domain_status_from_msg(
        domain_info_head_t *di_head, char *msg, size_t msg_size)
{
    size_t          p_offset = 0;
    domain_info_t   *d;
    
    /* FIXME: endianness */
    
    while (p_offset + sizeof(domain_info_t) < msg_size) { 
        d = malloc(sizeof(domain_info_t));
        
        memcpy(d, msg + p_offset, sizeof(domain_info_t));
        p_offset    += sizeof(domain_info_t);
        
        LIST_INSERT_HEAD(di_head, d, next);

        d->name      = strdup(msg + p_offset);        
        p_offset    += strlen(msg + p_offset) + 1;
    }

    return p_offset;
}

