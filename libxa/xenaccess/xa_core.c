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
 * $Date: 2006-12-06 01:23:30 -0500 (Wed, 06 Dec 2006) $
 */

#include <string.h>
#include <sys/mman.h>
#include <xs.h>
#include "config/config_parser.h"
#include "xenaccess.h"
#include "xa_private.h"

/* determine what type of OS is running in the requested domain */
int set_os_type (int *os_type)
{
    /*TODO for now we only support linux.  But this should be changed
     * to verify that the domain is really running linux */
    *os_type = XA_OS_LINUX;
    return XA_SUCCESS;
}

int read_config_file (xa_instance_t *instance)
{
    extern FILE *yyin;
    int ret = XA_SUCCESS;
    xa_config_entry_t *entry;
    struct xs_handle *xsh = NULL;
    xs_transaction_t xth = XBT_NULL;
    char *tmp = NULL;

    yyin = fopen("/etc/xenaccess.conf", "r");
    if (NULL == yyin){
        printf("WARNING: config file not found at /etc/xenaccess.conf\n");
        ret = XA_FAILURE;
        goto error_exit;
    }

    /* convert domain id to domain name */
    tmp = malloc(100);
    if (NULL == tmp){
        printf("ERROR: failed to allocate memory for tmp variable\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    memset(tmp, 0, 100);
    sprintf(tmp, "/local/domain/%d/name", instance->domain_id);
    xsh = xs_domain_open();
    instance->domain_name = xs_read(xsh, xth, tmp, NULL);
    if (NULL == instance->domain_name){
        printf("ERROR: domain id %d is not running\n", instance->domain_id);
        ret = XA_FAILURE;
        goto error_exit;
    }
#ifdef XA_DEBUG
    printf("Got domain name from id (%d ==> %s).\n",
           instance->domain_id, instance->domain_name);
#endif

    xa_parse_config(instance->domain_name);
    entry = xa_get_config();

    /* copy the values from entry into instance struct */
    instance->sysmap = strdup(entry->sysmap);
#ifdef XA_DEBUG
    printf("Got sysmap (%s).\n", instance->sysmap);
#endif

error_exit:
    if (tmp) free(tmp);
    if (yyin) fclose(yyin);
    if (xsh) xs_daemon_close(xsh);

    return ret;
}

/* given a xa_instance_t struct with the xc_handle and the
 * domain_id filled in, this function will fill in the rest
 * of the values using queries to libxc. */
int helper_init (xa_instance_t *instance)
{
    int ret = XA_SUCCESS;
    uint32_t local_offset = 0;
    unsigned char *memory = NULL;

    /* init instance->xc_handle */
    if (xc_domain_getinfo(
            instance->xc_handle, instance->domain_id,
            1, &(instance->info)
        ) != 1){
        printf("ERROR: Failed to get domain info\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
#ifdef XA_DEBUG
    printf("Got domain info.\n");
#endif

    /* read in configure file information */
    if (read_config_file(instance) == XA_FAILURE){
        printf("WARNING: failed to read configureation file\n");
    }
    
    /* init instance->os_type */
    if (set_os_type(&(instance->os_type)) == XA_FAILURE){
        ret = XA_FAILURE;
        goto error_exit;
    }
#ifdef XA_DEBUG
    printf("Got OS type.\n");
#endif

    /* init instance->hvm */
    instance->hvm = xa_ishvm(instance->domain_id);

    /* setup os-specific stuff */
    if (instance->os_type == XA_OS_LINUX){
        if (linux_system_map_symbol_to_address(
                 instance, "swapper_pg_dir", &instance->kpgd) == XA_FAILURE){
            printf("ERROR: failed to lookup 'swapper_pg_dir' address\n");
            ret = XA_FAILURE;
            goto error_exit;
        }
#ifdef XA_DEBUG
        printf("Got vaddr for swapper_pg_dir = 0x%.8x.\n", instance->kpgd);
#endif

        if (!instance->hvm){
            memory = linux_access_physical_address(
                instance, instance->kpgd - XA_PAGE_OFFSET, &local_offset);
            if (NULL == memory){
                printf("ERROR: failed to get physical addr for kpgd\n");
                ret = XA_FAILURE;
                goto error_exit;
            }
            instance->kpgd = *((uint32_t*)(memory + local_offset));
            munmap(memory, XA_PAGE_SIZE);
        }
#ifdef XA_DEBUG
        printf("swapper_pg_dir = 0x%.8x.\n", instance->kpgd);
#endif

        memory = xa_access_kernel_symbol(instance, "init_task", &local_offset);
        if (NULL == memory){
            printf("ERROR: failed to get task list head 'init_task'\n");
            ret = XA_FAILURE;
            goto error_exit;
        }
        instance->init_task =
            *((uint32_t*)(memory + local_offset + XALINUX_TASKS_OFFSET));
    }


error_exit:
    return ret;
}

/* cleans up any information in the xa_instance_t struct other
 * than the xc_handle and the domain_id */
int helper_destroy (xa_instance_t *instance)
{
    if (instance->live_pfn_to_mfn_table){
        munmap(instance->live_pfn_to_mfn_table, instance->nr_pfns*4);
    }

    xa_destroy_cache();

    return XA_SUCCESS;
}

int xa_init (uint32_t domain_id, xa_instance_t *instance)
{
    int xc_handle;

    /* open handle to the libxc interface */
    if ((xc_handle = xc_interface_open()) == -1){
        printf("ERROR: Failed to open libxc interface\n");
        return XA_FAILURE;
    }

/* example of getting version information from xen */
/*
{
int cmd = XENVER_extraversion;
void *arg = malloc(sizeof(xen_extraversion_t));
int rc = xc_version(xc_handle, cmd, arg);
printf("--> rc = %d\n", rc);
printf("--> arg = %s\n", (char*)arg);
free(arg);
}
*/

    /* populate struct with critical values */
    instance->xc_handle = xc_handle;
    instance->domain_id = domain_id;
    instance->live_pfn_to_mfn_table = NULL;
    instance->nr_pfns = 0;
    
    /* fill in rest of the information */
    return helper_init(instance);
}

int xa_destroy (xa_instance_t *instance)
{
    int ret1, ret2;

    instance->domain_id = 0;
    ret1 = helper_destroy(instance);
    ret2 = xc_interface_close(instance->xc_handle);

    if (XA_FAILURE == ret1 || XA_FAILURE == ret2){
        return XA_FAILURE;
    }
    return XA_SUCCESS;
}
