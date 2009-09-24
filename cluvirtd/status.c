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

#include <libvirt/libvirt.h>

#include "utils.h"
#include "status.h"


#define LIBVIRT_URI             "qemu:///system"
#define XPATH_VNC_PORT          \
    "string(/domain/devices/graphics[@type='vnc']/@port)"


static int _domain_status_vncport(virDomainPtr *domain, domain_status_t *vm)
{
    char *doc;
    
    xmlDocPtr xml;
    xmlXPathContextPtr ctxt;
    xmlXPathObjectPtr obj;
    
    vm->vncport = -1;
    
    if ((doc = virDomainGetXMLDesc(*domain, VIR_DOMAIN_XML_SECURE)) == 0) {
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
    
    vm->vncport = atoi((char*) obj->stringval);
    xmlXPathFreeObject(obj);

clean_exit3:
    xmlXPathFreeContext(ctxt);

clean_exit2:
    xmlFreeDoc(xml);

clean_exit1:
    free(doc);

    return 0;
}

int domain_status_fetch(domain_status_t *vm, int max_vm)
{
    int *ids, nvm, i;
    
    virConnectPtr conn;
    virDomainPtr domain;
    virDomainInfo info;  
    
    if ((conn = virConnectOpen("qemu:///system")) == 0) {
        log_error("unable to connect to libvirtd: %i", errno);
        return -1;
    }
    
    if ((nvm = virConnectNumOfDomains(conn)) < 0) {
        log_error("unable to fetch the number of domains: %i", errno);
        goto clean_exit1;
    }
    
    if ((ids = malloc(sizeof(int) * nvm)) == 0) {
        log_error("out of memory: %i", errno);
        exit(EXIT_FAILURE);
    }
        
    if ((nvm = virConnectListDomains(conn, ids, nvm)) < 0) {
        log_error("unable to fetch the domains list: %i", errno);
        goto clean_exit2;
    }

    if (nvm > max_vm) {
        log_error("out of memory: %i vm to write in %i", nvm, max_vm);
        goto clean_exit2;
    }
    
    memset(vm, 0, sizeof(domain_status_t) * max_vm);

    for (i = 0; i < nvm; i++) {
        vm[i].id = ids[i];
        
        if ((domain = virDomainLookupByID(conn, ids[i])) == 0) {
            log_error("unable to fetch domain %i: %i", ids[i], errno);
            continue;
        }

        vm[i].name = strdup(virDomainGetName(domain));
       
        if (virDomainGetInfo(domain, &info) >= 0) {
            time_t updated  = time(0);
            
            vm[i].state     = info.state;
            vm[i].memory    = info.memory;
            vm[i].ncpu      = info.nrVirtCpu;
            vm[i].usage     = 0;
            vm[i].cputime   = info.cpuTime;
            vm[i].updated   = updated;
        }
        else {
            log_error("unable to fetch info for domain %i: %i", ids[i], errno);
        }
        
        _domain_status_vncport(&domain, &vm[i]);
        
        virDomainFree(domain);
    }

clean_exit2:
    free(ids);
    
clean_exit1:
    if (virConnectClose(conn) != 0) {
        log_error("error closing connection: %i", errno);
    }
    
    return nvm;
}

domain_status_t *domain_status_init(domain_status_t *vm)
{
    if (vm == 0) {
        vm = malloc(sizeof(domain_status_t));
    }
    
    memset(vm, 0, sizeof(domain_status_t));
    vm->name = strdup("(unknown)");
    
    return vm;
}

void domain_status_copy(domain_status_t *dst, domain_status_t *src)
{
    LIST_ENTRY(_domain_status_t) tmp;
    
    memcpy(&tmp, &dst->next, sizeof(tmp));
    
    domain_status_free(dst);
    
    memcpy(dst, src, sizeof(domain_status_t));
    dst->name = strdup(src->name);
    
    memcpy(&dst->next, &tmp, sizeof(dst->next));
}

void domain_status_free(domain_status_t *vm)
{
    if (vm->name) free(vm->name);
    vm->name = 0;
}

int domain_status_to_msg(domain_status_t *vm, char *msg, int max_size)
{
    int name_len = strlen(vm->name) + 1;
    int msg_len = sizeof(domain_status_t) + name_len;
    domain_status_t *msg_vm = (domain_status_t *) msg;
    
    if (msg_len > max_size) return -1;

    memcpy(msg, vm, sizeof(domain_status_t));
    memcpy(msg + sizeof(domain_status_t), vm->name, name_len);

    memset(&msg_vm->next, 0, sizeof(vm->next));
    msg_vm->name = 0;
    msg_vm->rcvtime = 0;
    
    return msg_len;
}

int domain_status_from_msg(char *msg, domain_status_t *vm)
{
    char *msg_name = msg + sizeof(domain_status_t);
    
    memcpy(vm, msg, sizeof(domain_status_t));
    memset(&vm->next, 0, sizeof(vm->next));
    
    vm->name = msg_name;
    
    return (sizeof(domain_status_t) + strlen(msg_name) + 1);
}

