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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <error.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libvirt/libvirt.h>

#include <domain.h>
#include <utils.h>


#if CLUVIRT_UUID_BUFLEN != VIR_UUID_BUFLEN
#error cluvirt uuid buffer size mismatch (check your libvirt version)
#endif


#define XPATH_VNC_PORT          \
    "string(/domain/devices/graphics[@type='vnc']/@port)"

#define LIBVIRT_ID_BUFFER_SIZE  64


static virConnect *_lv_handle = 0;


int lv_init(char *uri)
{
    if ((_lv_handle = virConnectOpen(uri)) == 0) {
        log_error("unable to connect to libvirtd: %i", errno);
        return -1;
    }
    
    return 0;
}

int lv_finish(void)
{
    if (virConnectClose(_lv_handle) != 0) {
        log_error("error closing libvirt: %p", _lv_handle);
        return -1;
    }
    
    return 0;
}

static uint16_t _lv_vncport(virDomain *domain)
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

int lv_update_vminfo(clv_vminfo_head_t *di_head)
{
    static int      id[LIBVIRT_ID_BUFFER_SIZE];
    int             n, i;
    clv_vminfo_t    *d, *j;
    struct timeval  time_now;
    
    if ((n = virConnectNumOfDomains(_lv_handle)) < 0) {
        log_error("unable to count the domains: %i", n);
        return -1;
    }
    
    if (n > LIBVIRT_ID_BUFFER_SIZE) {
        log_error("id buffer size not large enough: %i", n);
        return -1;
    }

    if ((n = virConnectListDomains(_lv_handle, id, n)) < 0) {
        log_error("unable to fetch domains list: %i", n);
        return -1;
    }
    
    if (gettimeofday(&time_now, 0) < 0) {
        log_error("unable to get time of day: %i", errno);
        return -1;
    }
    
    for (i = 0; i < n; i++) {
        virDomain           *lv_domain;
        virDomainInfo       lv_info;
        time_t              time_delta;
        
        if (id[i] == 0) { /* skipping Domain-0 for xen hypervisor */
            continue;
        }
        
        if ((lv_domain = virDomainLookupByID(_lv_handle, id[i])) == 0) {
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
            
            vm_name = virDomainGetName(lv_domain);

            d = clv_vminfo_new(vm_name); /* FIXME: check error */
            LIST_INSERT_HEAD(di_head, d, next);
            
            d->id           = id[i];

            if (virDomainGetUUID(lv_domain, d->uuid) < 0) {
                memset(d->uuid, 0, CLUVIRT_UUID_BUFLEN);
                log_error("unable to get domain uuid %i", id[i]);
            }

            d->memory       = lv_info.memory;
            d->ncpu         = lv_info.nrVirtCpu;
            d->vncport      = _lv_vncport(lv_domain);
            
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
            clv_vminfo_free(j);
        }
    }
    
    return 0;
}

int lv_destroy_vm(const char *vmname)
{
    int retval = -1;
    virDomain *lv_domain;

    if ((lv_domain = virDomainLookupByName(_lv_handle, vmname)) == 0) {
        log_debug("domain %s not found", vmname);
        return -1;
    }
    
    if ((retval = virDomainDestroy(lv_domain)) != 0) {
        log_debug("unable to destroy domain %s", vmname);
    }

    virDomainFree(lv_domain);
    
    return (retval == 0) ? 0 : -1;
}

