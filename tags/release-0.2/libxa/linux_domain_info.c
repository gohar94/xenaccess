/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005 - 2006  Bryan D. Payne (bryan@thepaynes.cc)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * --------------------
 * This file contains utility functions for collecting information
 * from the domains.  Most of this high-level information is
 * gathered using the libvirt library (http://libvirt.org).
 *
 * File: linux_domain_info.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */

#include <stdio.h>
#include <string.h>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/* quick local implementation of non ISO C function for use below */
/*TODO move this into its own file of utility-type functions */
char * strdup (const char *s)
{
    int length = strlen(s) + 1;
    char *ret = malloc(length);
    if (NULL == ret){
        /* nothing */
    }
    else{
        memset(ret, 0, length);
        memcpy(ret, s, length);
    }
    return ret;
}

/* get an xml description of the domain id */
char *linux_get_xml_info (int id)
{
    virConnectPtr conn = NULL;
    virDomainPtr dom = NULL;
    char *xml_data = NULL;

    /* NULL means connect to local Xen hypervisor */
    conn = virConnectOpenReadOnly(NULL);
    if (NULL == conn) {
        printf("ERROR: Failed to connect to hypervisor\n");
        goto error_exit;
    }

    /* Find the domain of the given id */
    dom = virDomainLookupByID(conn, id);
    if (NULL == dom) {
        virErrorPtr error = virConnGetLastError(conn);
        printf("(%d) %s\n", error->code, error->message);
        printf("ERROR: Failed to find Domain %d\n", id);
        goto error_exit;
    }

    xml_data = virDomainGetXMLDesc(dom, 0);

error_exit:
    if (NULL!= dom) virDomainFree(dom);
    if (NULL!= conn) virConnectClose(conn);

    return xml_data;
}

char *linux_get_kernel_name (int id)
{
    char *xml_data = NULL;
    char *kernel = NULL;
    int depth = 0;
    xmlDocPtr xmldoc = NULL;
    xmlNode *root_element, *node;

    xml_data = linux_get_xml_info(id);
    if (NULL == xml_data){
        printf("ERROR: failed to get domain info for domain id %d\n", id);
        goto error_exit;
    }

    xmldoc = xmlParseMemory(xml_data, strlen(xml_data));
    if (NULL == xmldoc){
        printf("ERROR: could not parse xml data for domain id %d\n", id);
        goto error_exit;
    }

    /* get the root element node */
    root_element = xmlDocGetRootElement(xmldoc);

    /* seek out the kernel node */
    /* domain--os--kernel */
    node = root_element;
    while (node){
            if (strcmp((char *)node->name, "domain") == 0 && depth == 0){
                node = node->children;
                depth++;
            }
            else if (strcmp((char *)node->name, "os") == 0 && depth == 1){
                node = node->children;
                depth++;
            }
            else if (strcmp((char *)node->name, "kernel") == 0 && depth == 2){
                /* found it! */
                break;
            }
            else{
                node = node->next;
            }
    }

    /* node either equals NULL or the one we want */
    if (NULL != node){
        xmlChar *xmlStr = xmlNodeListGetString(xmldoc, node->children, 1);
        kernel = strdup((char *) xmlStr);
        xmlFree(xmlStr);
    }

error_exit:
    /* free up memory, clean up xml stuff */
    xmlFreeDoc(xmldoc);
    xmlCleanupParser();
    if (xml_data) free(xml_data);

    return kernel;
}

char *linux_predict_sysmap_name (int id)
{
    char *kernel = NULL;
    char *sysmap = NULL;
    int length = 0;
    int i = 0;

    kernel = linux_get_kernel_name(id);
    if (NULL == kernel){
        printf("ERROR: could not get kernel name for domain id %d\n", id);
        goto error_exit;
    }

    /* replace 'vmlinuz' with 'System.map' */
    length = strlen(kernel) + 4;
    sysmap = malloc(length);
    memset(sysmap, 0, length);
    for (i = 0; i < length; ++i){
        if (strncmp(kernel + i, "vmlinu", 6) == 0){
            strcat(sysmap, "System.map");
            strcat(sysmap, kernel + i + 7);
            break;
        }
        else{
            sysmap[i] = kernel[i];
        }
    }

error_exit:
    if (kernel) free(kernel);
    return sysmap;
}

/* -- USED ONLY FOR TEST / DEBUGGING */
/*
int main(int argc, char **argv) {
    printf("sysmap --> '%s'\n", xa_predict_sysmap_name(2));
    return(0);
}
*/

/*
use the following line to compile

gcc `xml2-config --cflags --libs` -I/usr/local/include/libvirt -L/usr/local/lib -lvirt -g  -o xa_domain_info xa_domain_info.c 
*/

