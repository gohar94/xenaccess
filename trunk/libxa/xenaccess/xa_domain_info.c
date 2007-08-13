/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005 - 2007  Bryan D. Payne (bryan@thepaynes.cc)
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
 * File: xa_domain_info.c
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

char *xa_get_vmpath (int id)
{
    struct xs_handle *xsh = NULL;
    xs_transaction_t xth = XBT_NULL;
    char *tmp = NULL;
    char *vmpath = NULL;

    tmp = malloc(100);
    if (NULL == tmp){
        goto error_exit;
    }

    /* get the vm path */
    memset(tmp, 0, 100);
    sprintf(tmp, "/local/domain/%d/vm", id);
    xsh = xs_domain_open();
    vmpath = xs_read(xsh, xth, tmp, NULL);

error_exit:
    /* cleanup memory here */
    if (tmp) free(tmp);
    if (xsh) xs_daemon_close(xsh);

    return vmpath;
}

char *xa_get_kernel_name (int id)
{
    struct xs_handle *xsh = NULL;
    xs_transaction_t xth = XBT_NULL;
    char *vmpath = NULL;
    char *kernel = NULL;
    char *tmp = NULL;

    vmpath = xa_get_vmpath(id);

    /* get the kernel name */
    tmp = malloc(100);
    if (NULL == tmp){
        goto error_exit;
    }
    memset(tmp, 0, 100);
    sprintf(tmp, "%s/image/kernel", vmpath);
    xsh = xs_domain_open();
    kernel = xs_read(xsh, xth, tmp, NULL);

error_exit:
    /* cleanup memory here */
    if (tmp) free(tmp);
    if (vmpath) free(vmpath);
    if (xsh) xs_daemon_close(xsh);

    return kernel;
}

int xa_ishvm (int id)
{
    struct xs_handle *xsh = NULL;
    xs_transaction_t xth = XBT_NULL;
    char *vmpath = NULL;
    char *ostype = NULL;
    char *tmp = NULL;
    int ret = 0;

    vmpath = xa_get_vmpath(id);

    /* get the os type */
    tmp = malloc(100);
    if (NULL == tmp){
        goto error_exit;
    }
    memset(tmp, 0, 100);
    sprintf(tmp, "%s/image/ostype", vmpath);
    xsh = xs_domain_open();
    ostype = xs_read(xsh, xth, tmp, NULL);

    if (NULL == ostype){
        goto error_exit;
    }
    else if (strcmp(ostype, "hvm") == 0){
        ret = 1;
    }

error_exit:
    /* cleanup memory here */
    if (tmp) free(tmp);
    if (vmpath) free(vmpath);
    if (ostype) free(ostype);
    if (xsh) xs_daemon_close(xsh);

    return ret;
}
