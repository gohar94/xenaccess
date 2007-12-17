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
#include <xen/arch-x86/xen.h>

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
        printf("ERROR: config file not found at /etc/xenaccess.conf\n");
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
    xa_dbprint("--got domain name from id (%d ==> %s).\n",
               instance->domain_id, instance->domain_name);

    if (xa_parse_config(instance->domain_name)){
        printf("ERROR: failed to read config file\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    entry = xa_get_config();

    /* copy the values from entry into instance struct */
    instance->sysmap = strdup(entry->sysmap);
    xa_dbprint("--got sysmap from config (%s).\n", instance->sysmap);
    
    if (strncmp(entry->ostype, "Linux", CONFIG_STR_LENGTH) == 0){
        instance->os_type = XA_OS_LINUX;
    }
    else if (strncmp(entry->ostype, "Windows", CONFIG_STR_LENGTH) == 0){
        instance->os_type = XA_OS_WINDOWS;
    }
    else{
        printf("ERROR: Unknown or undefined OS type.\n");
        ret = XA_FAILURE;
        goto error_exit;
    }

    /* Copy config info based on OS type */
    if(XA_OS_LINUX == instance->os_type){
	    xa_dbprint("--reading in linux offsets from config file.\n");
        if(entry->offsets.linux_offsets.tasks){
            instance->os.linux_instance.tasks_offset =
                 entry->offsets.linux_offsets.tasks;
        }

        if(entry->offsets.linux_offsets.mm){
            instance->os.linux_instance.mm_offset =
                entry->offsets.linux_offsets.mm;
        }

        if(entry->offsets.linux_offsets.pid){
            instance->os.linux_instance.pid_offset =
                entry->offsets.linux_offsets.pid;
        }

        if(entry->offsets.linux_offsets.pgd){
            instance->os.linux_instance.pgd_offset =
                entry->offsets.linux_offsets.pgd;
        }

        if(entry->offsets.linux_offsets.addr){
            instance->os.linux_instance.addr_offset =
                entry->offsets.linux_offsets.addr;
        }
    }
    else if (XA_OS_WINDOWS == instance->os_type){
	    xa_dbprint("--reading in windows offsets from config file.\n");
        if(entry->offsets.windows_offsets.tasks){
            instance->os.windows_instance.tasks_offset =
                entry->offsets.windows_offsets.tasks;
        }

        if(entry->offsets.windows_offsets.pdbase){ 
            instance->os.windows_instance.pdbase_offset =
                entry->offsets.windows_offsets.pdbase;
        }

        if(entry->offsets.windows_offsets.pid){
            instance->os.windows_instance.pid_offset =
                entry->offsets.windows_offsets.pid;
        }

        if(entry->offsets.windows_offsets.peb){
            instance->os.windows_instance.peb_offset =
                entry->offsets.windows_offsets.peb;
        }

        if(entry->offsets.windows_offsets.iba){
            instance->os.windows_instance.iba_offset =
                entry->offsets.windows_offsets.iba;
        }

        if(entry->offsets.windows_offsets.ph){
            instance->os.windows_instance.ph_offset =
                entry->offsets.windows_offsets.ph;
        }
    }

#ifdef XA_DEBUG
    xa_dbprint("--got ostype from config (%s).\n", entry->ostype);
    if (instance->os_type == XA_OS_LINUX){
        xa_dbprint("**set instance->os_type to Linux.\n");
    }
    else if (instance->os_type == XA_OS_WINDOWS){
        xa_dbprint("**set instance->os_type to Windows.\n");
    }
    else{
        xa_dbprint("**set instance->os_type to unknown.\n");
    }
#endif

error_exit:
    if (tmp) free(tmp);
    if (yyin) fclose(yyin);
    if (xsh) xs_daemon_close(xsh);

    return ret;
}

/* check that this domain uses a paging method that we support */
int get_page_info (xa_instance_t *instance)
{
    int ret = XA_SUCCESS;
    int i = 0, j = 0;
    vcpu_guest_context_t ctxt;
    if ((ret = xc_vcpu_getcontext(
                instance->xc_handle,
                instance->domain_id,
                0, /*TODO vcpu, assuming only 1 for now */
                &ctxt)) != 0){
        printf("ERROR: failed to get context information.\n");
        ret = XA_FAILURE;
        goto error_exit;
    }

    /* For details on the registers involved in the x86 paging configuation
       see the Intel 64 and IA-32 Architectures Software Developer's Manual,
       Volume 3A: System Programming Guide, Part 1. */

    /* PG Flag --> CR0, bit 31 == 1 --> paging enabled */
    if (!xa_get_bit(ctxt.ctrlreg[0], 31)){
        printf("ERROR: Paging disabled for this VM, not supported.\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    /* PAE Flag --> CR4, bit 5 == 0 --> pae disabled */
    instance->pae = xa_get_bit(ctxt.ctrlreg[4], 5);
    xa_dbprint("**set instance->pae = %d\n", instance->pae);

    /* PSE Flag --> CR4, bit 4 == 0 --> pse disabled */
    instance->pse = xa_get_bit(ctxt.ctrlreg[4], 4);
    xa_dbprint("**set instance->pse = %d\n", instance->pse);

error_exit:
    return ret;
}

void init_page_offset (xa_instance_t *instance)
{
    if (XA_OS_LINUX == instance->os_type){
        instance->page_offset = 0xc0000000;
    }
    else if (XA_OS_WINDOWS == instance->os_type){
        instance->page_offset = 0x80000000;
    }
    else{
        instance->page_offset = 0;
    }

    /* assume 4k pages for now, update when 4M page is found */
    instance->page_shift = 12;
    instance->page_size = 1 << instance->page_shift;
}

void init_xen_version (xa_instance_t *instance)
{
    int cmd1 = XENVER_extraversion;
    int cmd2 = XENVER_capabilities;
    void *extra = malloc(sizeof(xen_extraversion_t));
    void *cap = malloc(sizeof(xen_capabilities_info_t));

    xc_version(instance->xc_handle, cmd1, extra);
    xc_version(instance->xc_handle, cmd2, cap);

    instance->xen_version = XA_XENVER_UNKNOWN;
    if (strncmp((char *)extra, ".4-1", 4) == 0){
        if (strncmp((char *)cap, "xen-3.0-x86_32", 14) == 0){
            instance->xen_version = XA_XENVER_3_0_4;
            xa_dbprint("**set instance->xen_version = 3.0.4\n");
        }
    }
    else if (strncmp((char *)extra, ".0", 2) == 0){
        if (strncmp((char *)cap, "xen-3.0-x86_32p", 15) == 0){
            instance->xen_version = XA_XENVER_3_1_0;
            xa_dbprint("**set instance->xen_version = 3.1.0\n");
        }
    }
    free(extra);
    free(cap);

    if (instance->xen_version == XA_XENVER_UNKNOWN){
        xa_dbprint("extra: %s\n", extra);
        xa_dbprint("cap: %s\n", cap);
        printf("WARNING: This Xen version not supported by XenAccess.\n");
    }
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
    xa_dbprint("--got domain info.\n");

    /* find the version of xen that we are running */
    init_xen_version(instance);

    /* read in configure file information */
    if (read_config_file(instance) == XA_FAILURE){
        ret = XA_FAILURE;
        goto error_exit;
    }
    
    /* determine the page sizes and layout for target OS */
    if (get_page_info(instance) == XA_FAILURE){
        printf("ERROR: memory layout not supported\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    xa_dbprint("--got memory layout.\n");

    /* setup the correct page offset size for the target OS */
    init_page_offset(instance);

    /* init instance->hvm */
    instance->hvm = xa_ishvm(instance->domain_id);
#ifdef XA_DEBUG
    if (instance->hvm){
        xa_dbprint("**set instance->hvm to true (HVM).\n");
    }
    else{
        xa_dbprint("**set instance->hvm to false (PV).\n");
    }
#endif

    /* setup OS specific stuff */
    if (instance->os_type == XA_OS_LINUX){
        ret = linux_init(instance);
    }
    else if (instance->os_type == XA_OS_WINDOWS){
        ret = windows_init(instance);
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

    xa_destroy_cache(instance);

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
    xa_dbprint("XenAccess Devel Version\n");

    /* populate struct with critical values */
    instance->xc_handle = xc_handle;
    instance->domain_id = domain_id;
    instance->live_pfn_to_mfn_table = NULL;
    instance->nr_pfns = 0;
    instance->cache_head = NULL;
    instance->cache_tail = NULL;
    instance->current_cache_size = 0;

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
