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
 * This file contains functions for sharing the memory of a domU
 * machine.  The functions basically only differ in how the memory
 * is referenced (pfn, mfn, virtual address, physical address, etc).
 *
 * File: xa_memory.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date: 2006-11-29 20:38:20 -0500 (Wed, 29 Nov 2006) $
 */

#include <stdlib.h>
#include <sys/mman.h>
#include "xenaccess.h"
#include "xa_private.h"

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
        return pfn;
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
            (nr_pfns+(XA_PFN_PER_FRAME*XA_PFN_PER_FRAME)-1)/(XA_PFN_PER_FRAME*XA_PFN_PER_FRAME) );
        if (live_pfn_to_mfn_frame_list == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_frame_list\n");
            goto error_exit;
        }

        live_pfn_to_mfn_table = xc_map_foreign_batch(
            instance->xc_handle, instance->domain_id, PROT_READ,
            live_pfn_to_mfn_frame_list, (nr_pfns+XA_PFN_PER_FRAME-1)/XA_PFN_PER_FRAME );
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
    xa_dbprint("--MapMFN: Mapping mfn = %lu.\n", mfn);
    return xc_map_foreign_range(
        instance->xc_handle, instance->domain_id, 1, prot, mfn);
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
        xa_dbprint("--MapPFN: Mapping mfn = %lu.\n", mfn);
        return xc_map_foreign_range(
            instance->xc_handle, instance->domain_id, 1, prot, mfn);
    }
}

/* TODO: make this more flexible (e.g., PAE) */
/* convert address to machine address via page tables */
uint32_t xa_pagetable_lookup (
            xa_instance_t *instance,
            uint32_t pgd,
            uint32_t virt_address,
            int kernel)
{
    uint32_t index = 0;
    uint32_t offset = 0;
    uint32_t pgd_entry = 0;
    uint32_t pte_entry = 0;
    unsigned char *memory = NULL;

    xa_dbprint("--PTLookup: lookup vaddr = 0x%.8x\n", virt_address);

    /* perform the lookup in the global directory */
    index = (((virt_address) >> 22) & (1024 - 1));
    xa_dbprint("--PTLookup: pgd index = 0x%.8x.\n", index);
    pgd_entry = pgd + (index * sizeof(uint32_t));
    xa_dbprint("--PTLookup: pgd_entry address = 0x%.8x.\n", pgd_entry);
    if (kernel){
        memory = xa_access_physical_address(
                instance, pgd_entry - instance->page_offset, &offset);
    }
    else{
        memory = xa_access_virtual_address(instance, pgd_entry, &offset);
    }
    if (NULL == memory){
        printf("ERROR: pgd entry lookup failed (phys addr = 0x%.8x)\n",
                pgd_entry - instance->page_offset);
        printf("** pgd = 0x%.8x\n", pgd);
        printf("** pgd_entry = 0x%.8x\n", pgd_entry);
        printf("** vaddr = 0x%.8x\n", virt_address);
        return 0;
    }
    pgd_entry = *((uint32_t*)(memory + offset));
    xa_dbprint("--PTLookup: pgd_entry = 0x%.8x.\n", pgd_entry);
    munmap(memory, instance->page_size);
    if (!xa_get_bit(pgd_entry, 0)){ /* is page in phys memory? */
        printf("ERROR: requested page is not in physical memory\n");
        return 0;
    }
    if (instance->pse && xa_get_bit(pgd_entry, 7)){
        xa_dbprint("--PTLookup: page size is 4MB.\n");
        index = virt_address & 0x3fffff;
        return ((pgd_entry & 0xffc00000) + index);
    }

    /* no page middle directory since we are assuming no PAE for now */

    /* perform the lookup in the page table */
    index = (((virt_address) >> 12) & (1024 - 1));
    xa_dbprint("--PTLookup: pte index = 0x%.8x.\n", index);
    pte_entry = (pgd_entry & 0xfffff000) + (index * sizeof(uint32_t));
    xa_dbprint("--PTLookup: pte_entry address = 0x%.8x.\n", pte_entry);
    memory = xa_access_machine_address(instance, pte_entry, &offset);
    if (NULL == memory){
        printf("ERROR: pte entry lookup failed (mach addr = 0x%.8x)\n",
                pte_entry);
        printf("** pgd_entry = 0x%.8x\n", pgd_entry);
        printf("** pte_entry = 0x%.8x\n", pte_entry);
        printf("** vaddr = 0x%.8x\n", virt_address);
        return 0;
    }
    pte_entry = *((uint32_t*)(memory + offset));
    xa_dbprint("--PTLookup: pte_entry = 0x%.8x.\n", pte_entry);
    munmap(memory, instance->page_size);

    /* finally grab the location in memory */
    index = virt_address & 0xfff;
    return ((pte_entry & 0xfffff000) + index);
}

/* expose virtual to physical mapping via api call */
uint32_t xa_translate_kv2p(xa_instance_t *instance, uint32_t virt_address)
{
    return xa_pagetable_lookup(instance, instance->kpgd, virt_address, 1);
}

void *xa_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
{
    if (XA_OS_LINUX == instance->os_type){
        return linux_access_kernel_symbol(instance, symbol, offset);
    }

    /*TODO we do not yet support any other OSes */
    else{
        return NULL;
    }
}

/* finds the address of the page global directory for a given pid */
uint32_t xa_pid_to_pgd (xa_instance_t *instance, int pid)
{
    if (XA_OS_LINUX == instance->os_type){
        return linux_pid_to_pgd(instance, pid);
    }
    else if (XA_OS_WINDOWS == instance->os_type){
        return windows_pid_to_pgd(instance, pid);
    }

    /*TODO we do not yet support any other OSes */
    else{
        return 0;
    }
}

void *xa_access_user_virtual_address (
        xa_instance_t *instance,
        uint32_t virt_address,
        uint32_t *offset,
        int pid)
{
    uint32_t address = 0;

    /* check the LRU cache */
    if (xa_check_cache_virt(virt_address, pid, &address)){
        return xa_access_machine_address(instance, address, offset);
    }

    /* use kernel page tables */
    /*TODO HYPERVISOR_VIRT_START = 0xFC000000 so we can't go over that.
      Figure out what this should be b/c there still may be a fixed
      mapping range between the page'd addresses and VIRT_START */
    if (!pid){
        address = xa_pagetable_lookup(
                            instance, instance->kpgd, virt_address, 1);
        if (!address){
            printf("ERROR: address not in page table (0x%x)\n", virt_address);
            return NULL;
        }
        xa_update_cache(NULL, virt_address, pid, address);
        return xa_access_machine_address(instance, address, offset);
    }

    /* use user page tables */
    else{
        uint32_t pgd = xa_pid_to_pgd(instance, pid);
        xa_dbprint("--UserVirt: pgd for pid=%d is 0x%.8x.\n", pid, pgd);

        if (pgd){
            address = xa_pagetable_lookup(instance, pgd, virt_address, 0);
        }

        if (!address){
            printf("ERROR: address not in page table (0x%x)\n", virt_address);
            return NULL;
        }
        xa_update_cache(NULL, virt_address, pid, address);
        return xa_access_machine_address_rw(
            instance, address, offset, PROT_READ | PROT_WRITE);
    }
}

void *xa_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
{
    return xa_access_user_virtual_address(instance, virt_address, offset, 0);
}

void *xa_access_physical_address (
        xa_instance_t *instance, uint32_t phys_address, uint32_t *offset)
{
    unsigned long pfn;
    int i;
    
    /* page frame number = physical address >> PAGE_SHIFT */
    pfn = phys_address >> instance->page_shift;
    
    /* get the offset */
    *offset = (instance->page_size-1) & phys_address;
    
    /* access the memory */
    return xa_mmap_pfn(instance, PROT_READ, pfn);
}

void *xa_access_machine_address (
        xa_instance_t *instance, uint32_t mach_address, uint32_t *offset)
{
    return xa_access_machine_address_rw(
                instance, mach_address, offset, PROT_READ);
}

void *xa_access_machine_address_rw (
        xa_instance_t *instance, uint32_t mach_address,
        uint32_t *offset, int prot)
{
    unsigned long mfn;
    int i;

    /* machine frame number = machine address >> PAGE_SHIFT */
    mfn = mach_address >> instance->page_shift;

    /* get the offset */
    *offset = (instance->page_size-1) & mach_address;

    /* access the memory */
    return xa_mmap_mfn(instance, prot, mfn);
}

