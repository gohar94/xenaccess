/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005  Bryan D. Payne (bryan@thepaynes.cc)
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
 * This file contains core functions that are responsible for
 * initialization and destruction of the libxa instance.
 *
 * File: xa_core.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */

#include "xenaccess.h"

/* given a xa_instance_t struct with the xc_handle and the
 * domain_id filled in, this function will fill in the rest
 * of the values using queries to libxc. */
int helper_init (xa_instance_t *instance)
{
    int ret = XA_SUCCESS;

    if (xc_domain_getinfo(
            instance->xc_handle, instance->domain_id,
            1, &(instance->info)
        ) != 1){
        ret = XA_FAILURE;
        goto error_exit;
    }

error_exit:
    return ret;
}

/* cleans up any information in the xa_instance_t struct other
 * than the xc_handle and the domain_id */
int helper_destroy (xa_instance_t *instance)
{
    return XA_SUCCESS;
}

int xa_init (uint32_t domain_id, xa_instance_t *instance)
{
    int xc_handle;

    /* open handle to the libxc interface */
    if ((xc_handle = xc_interface_open()) == -1){
        return XA_FAILURE;
    }

    /* populate struct with critical values */
    instance->xc_handle = xc_handle;
    instance->domain_id = domain_id;
    
    /* fill in rest of the information */
    return helper_init(instance);
}

int xa_destroy (xa_instance_t *instance)
{
    int ret1, ret2;

    instance->domain_id = 0;
    ret1 = helper_destroy(instance);
    ret2 = xc_interface_close(instance->xc_handle);

    if (ret1 == XA_FAILURE || ret2 == XA_FAILURE){
        return XA_FAILURE;
    }
    return XA_SUCCESS;
}