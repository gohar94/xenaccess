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
 * This file contains functions for sharing the memory of a domU
 * machine.  The functions basically only differ in how the memory
 * is referenced (pfn, mfn, virtual address, physical address, etc).
 *
 * File: xa_memory.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id: xa_memory.c 36 2006-11-30 01:38:20Z bdpayne $
 * $Date: 2006-11-29 20:38:20 -0500 (Wed, 29 Nov 2006) $
 */

#include <stdlib.h>
#include <sys/mman.h>
#include "xenaccess.h"
#include "xa_private.h"

unsigned long p2m_hvm (xa_instance_t *instance, unsigned long pfn)
{
    unsigned long mfn = -1;

    xc_domain_translate_gpfn_list(
        instance->xc_handle,
        instance->domain_id,
        1,
        &pfn,
        &mfn
    );

    return mfn;
}

/* convert a pfn to a mfn based on the live mapping tables */
unsigned long helper_pfn_to_mfn (xa_instance_t *instance, unsigned long pfn)
{
    shared_info_t *live_shinfo = NULL;
    unsigned long *live_pfn_to_mfn_frame_list_list = NULL;
    unsigned long *live_pfn_to_mfn_frame_list = NULL;

    /* Live mapping of the table mapping each PFN to its current MFN. */
    unsigned long *live_pfn_to_mfn_table = NULL;
    unsigned long nr_pfns = 0;
    unsigned long ret = -1;

    if (instance->hvm){
        return p2m_hvm(instance, pfn);
    }

    if (NULL == instance->live_pfn_to_mfn_table){
        nr_pfns = instance->info.max_memkb >> (XC_PAGE_SHIFT - 10);

        live_shinfo = xa_mmap_mfn(
            instance, PROT_READ, instance->info.shared_info_frame);
        if (live_shinfo == NULL){
            printf("ERROR: failed to init live_shinfo\n");
            goto error_exit;
        }

        live_pfn_to_mfn_frame_list_list = xa_mmap_mfn(
            instance, PROT_READ, live_shinfo->arch.pfn_to_mfn_frame_list_list);
        if (live_pfn_to_mfn_frame_list_list == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_frame_list_list\n");
            goto error_exit;
        }

        live_pfn_to_mfn_frame_list = xc_map_foreign_batch(
            instance->xc_handle, instance->domain_id, PROT_READ,
            live_pfn_to_mfn_frame_list_list,
            (nr_pfns+(1024*1024)-1)/(1024*1024) );
        if (live_pfn_to_mfn_frame_list == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_frame_list\n");
            goto error_exit;
        }

        live_pfn_to_mfn_table = xc_map_foreign_batch(
            instance->xc_handle, instance->domain_id, PROT_READ,
            live_pfn_to_mfn_frame_list, (nr_pfns+1023)/1024 );
        if (live_pfn_to_mfn_table  == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_table\n");
            goto error_exit;
        }

        instance->live_pfn_to_mfn_table = live_pfn_to_mfn_table;
        instance->nr_pfns = nr_pfns;
    }

    ret = instance->live_pfn_to_mfn_table[pfn];

error_exit:
    if (live_shinfo) munmap(live_shinfo, XC_PAGE_SIZE);
    if (live_pfn_to_mfn_frame_list_list)
        munmap(live_pfn_to_mfn_frame_list_list, XC_PAGE_SIZE);
    if (live_pfn_to_mfn_frame_list)
        munmap(live_pfn_to_mfn_frame_list, XC_PAGE_SIZE);

    return ret;
}

void *xa_mmap_mfn (xa_instance_t *instance, int prot, unsigned long mfn)
{
    return xc_map_foreign_range(
        instance->xc_handle, instance->domain_id, XC_PAGE_SIZE, prot, mfn);
}

void *xa_mmap_pfn (xa_instance_t *instance, int prot, unsigned long pfn)
{
    unsigned long mfn;

    mfn = helper_pfn_to_mfn(instance, pfn);

    if (-1 == mfn){
        printf("ERROR: pfn to mfn mapping failed.\n");
        return NULL;
    }
    else{
        return xc_map_foreign_range(
            instance->xc_handle, instance->domain_id, XC_PAGE_SIZE, prot, mfn);
    }
}

void *xa_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
{
    if (instance->os_type == XA_OS_LINUX){
        return linux_access_kernel_symbol(instance, symbol, offset);
    }

    /*TODO we do not yet support any other OSes */
    else{
        return NULL;
    }
}

void *xa_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
{
/* test code */
/*
    unsigned long mfn;
    uint32_t bitmask;
    int i;

    mfn = xc_translate_foreign_address(
            instance->xc_handle,
            instance->domain_id,
            0,
            virt_address
    );

    bitmask = 1;
    for (i = 0; i < XC_PAGE_SHIFT - 1; ++i){
        bitmask <<= 1;
        bitmask += 1;
    }
    *offset = virt_address & bitmask;

    return xa_mmap_mfn(instance, PROT_READ, mfn);
*/
/* end test code */


    if (instance->os_type == XA_OS_LINUX){
        return linux_access_virtual_address(instance, virt_address, offset);
    }
    else{
        return NULL;
    }
}

void *xa_access_user_virtual_address (
            xa_instance_t *instance,
            uint32_t virt_address,
            uint32_t *offset,
            int pid)
{
    if (instance->os_type == XA_OS_LINUX){
        return linux_access_user_virtual_address(
                    instance, virt_address, offset, pid);
    }

    /*TODO we do not yet support any other OSes */
    else{
        return NULL;
    }
}
