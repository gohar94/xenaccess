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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <xs.h>
#include <xa_private.h>

/* quick local implementation of non ISO C function for use below */
/*TODO move this into its own file of utility-type functions */
char * strdup (const char *s)
{
    int length = strlen(s) + 1;
    char *ret = malloc(length);
    if (NULL == ret){
    }
    else{
        memset(ret, 0, length);
        memcpy(ret, s, length);
    }
    return ret;
}

char *linux_predict_sysmap_name (int id)
{
    char *kernel = NULL;
    char *sysmap = NULL;
    int length = 0;
    int i = 0;

    kernel = xa_get_kernel_name(id);
    if (NULL == kernel){
        printf("ERROR: could not get kernel name for domain id %d\n", id);
        goto error_exit;
    }

    /* tmp hard code for testing */
    else if (strcmp(kernel, "/usr/lib/xen/boot/hvmloader") == 0){
        sysmap = strdup("/boot/System.map-2.6.15-1.2054_FC5");
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

