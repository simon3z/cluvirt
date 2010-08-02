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

#include <cluvirt.h>

#include "utils.h"


#define XPATH_VNC_PORT          \
    "string(/domain/devices/graphics[@type='vnc']/@port)"

#define LIBVIRT_ID_BUFFER_SIZE  64


static uint16_t _libvirt_vncport(virDomain *);


int domain_status_update(char *uri, domain_info_head_t *di_head)
{
    static int      id[LIBVIRT_ID_BUFFER_SIZE];
    int             n, i;
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
        time_t              time_delta;
        
        if (id[i] == 0) { /* skipping Domain-0 for xen hypervisor */
            continue;
        }
        
        if ((lv_domain = virDomainLookupByID(lvh, id[i])) == 0) {
            log_error("unable to lookup domain %i", id[i]);
            continue;
        }
        
        if (virDomainGetInfo(lv_domain, &lv_info) < 0) {
            log_error("unable to get info for domain %i", id[i]);
            goto clean_loop1;
        }
        
        LIST_FOREACH(d, di_head, next) {
            if (d->id == id[i]) break;
        }
        
        if (d == 0) {
            const char *vm_name;

            log_debug("adding new domain info: %i", id[i]);
            
            d = malloc(sizeof(domain_info_t));
            LIST_INSERT_HEAD(di_head, d, next);
            
            d->id               = id[i];
            
            /* the following values won't be updated anymore */
            if ((vm_name = virDomainGetName(lv_domain)) == 0) {
                d->name         = strdup("(unknown)");
            }
            else {
                d->name         = strdup(vm_name);
            }

            if (virDomainGetUUID(lv_domain, d->uuid) < 0) {
                memset(d->uuid, 0, VIR_UUID_BUFLEN);
                log_error("unable to get domain uuid %i", id[i]);
            }

            d->memory       = lv_info.memory;
            d->ncpu         = lv_info.nrVirtCpu;
            d->vncport      = _libvirt_vncport(lv_domain);
            
            /* initializing cpu load parameters */
            d->usage        = 0;
            d->cputime      = lv_info.cpuTime;
            memcpy(&d->update, &time_now, sizeof(d->update));
        }

        /* these values will be updated at each call */
        d->state    = lv_info.state;
        
        time_delta  = ((time_now.tv_sec - d->update.tv_sec) * 1000000ll) +
                        (time_now.tv_usec - d->update.tv_usec);
        
        /* update cpu utilization if time_delta > 1 second */
        if (time_delta > 1000000ll) { /* > 0 to avoid divide-by-zero */
            d->usage        = (unsigned short) 
                                  ((time_t) (lv_info.cpuTime - d->cputime)
                                          / (lv_info.nrVirtCpu * time_delta));

            d->cputime      = lv_info.cpuTime;
            memcpy(&d->update, &time_now, sizeof(d->update));
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

static uint16_t _libvirt_vncport(virDomain *domain)
{
    char                *doc;
    xmlDoc              *xml;
    xmlXPathContext     *ctxt;
    xmlXPathObject      *obj;
    uint16_t            port = 0;
    
    if ((doc = virDomainGetXMLDesc(domain, VIR_DOMAIN_XML_SECURE)) == 0) {
        log_error("failed to dump xml: %i", errno);
        return 0;
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
    
    port = (uint16_t) atoi((char*) obj->stringval);

    xmlXPathFreeObject(obj);

clean_exit3:
    xmlXPathFreeContext(ctxt);

clean_exit2:
    xmlFreeDoc(xml);

clean_exit1:
    free(doc);

    return port;
}

ssize_t domain_status_to_msg(
        domain_info_head_t *di_head, char *msg, size_t max_size)
{
    domain_info_t       *d;
    domain_info_msg_t   *m;
    size_t              p_offset, n_offset, name_size;
    
    p_offset = 0;
    
    LIST_FOREACH(d, di_head, next) {
        name_size   = strlen(d->name) + 1;
        n_offset    = p_offset + sizeof(domain_info_msg_t) + name_size;
        
        if (n_offset > max_size) {
            log_error("message buffer is not large enough: %lu", n_offset);
            return -1;
        }
        
        m = (domain_info_msg_t*) (msg + p_offset);
        
        m->id               = be_swap32((uint32_t) d->id);
        m->memory           = be_swap32((uint32_t) d->memory);
        m->cputime          = be_swap64(d->cputime);
        m->usage            = be_swap16(d->usage);
        m->ncpu             = be_swap16(d->ncpu);
        m->vncport          = be_swap16(d->vncport);

        m->state            = d->state;
        m->payload_size     = be_swap32((uint32_t) name_size);

        memcpy(m->uuid, d->uuid, sizeof(m->uuid));
        memcpy(m->payload, d->name, name_size);
        
        log_debug("p_offset: %lu, n_offset: %lu, m: %p", p_offset, n_offset, m);
        p_offset            = n_offset;
    }
    
    return (ssize_t) p_offset;
}

ssize_t domain_status_from_msg(
        domain_info_head_t *di_head, char *msg, size_t msg_size)
{
    domain_info_t       *d;
    domain_info_msg_t   *m;
    size_t              p_offset = 0, n_offset, name_size;
    
    while ((p_offset + sizeof(domain_info_msg_t)) < msg_size) {
        d = malloc(sizeof(domain_info_t));
        m = (domain_info_msg_t*) (msg + p_offset);
        
        d->id           = be_swap32(m->id);
        d->memory       = be_swap32(m->memory);
        d->cputime      = be_swap64(m->cputime);
        d->usage        = be_swap16(m->usage);
        d->ncpu         = be_swap16(m->ncpu);
        d->vncport      = be_swap16(m->vncport);
        d->state        = m->state;
        name_size       = be_swap32(m->payload_size);
        
        n_offset        = p_offset + sizeof(domain_info_msg_t) + name_size;
        
        if (n_offset > msg_size) {
            log_error("malformed message: %lu %lu", n_offset, msg_size);
            return -1;
        }
        
        d->name         = malloc(name_size);
        
        memcpy(d->name, m->payload, name_size);
        memcpy(d->uuid, m->uuid, sizeof(d->uuid));

        LIST_INSERT_HEAD(di_head, d, next);

        log_debug("p_offset: %lu, n_offset: %lu, m: %p", p_offset, n_offset, m);
        p_offset        = n_offset;
    }

    return (ssize_t) p_offset;
}

