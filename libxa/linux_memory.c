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
 * This file contains routines for accessing memory on a linux domU.
 *
 * File: linux_memory.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */

#include <stdio.h>
#include <sys/mman.h>
#include "xa_private.h"

/* convert high_memory address to machine address via page tables */
uint32_t linux_highmem_lookup (xa_instance_t *instance, uint32_t virt_address)
{
    uint32_t kpgd = 0;
    uint32_t index = 0;
    uint32_t offset = 0;
    uint32_t pgd_entry = 0;
    uint32_t pte_entry = 0;
    unsigned char *memory = NULL;

    /* make sure that the address is in the right range */
    if (virt_address < instance->high_memory || 0xfe000000 < virt_address){
        return 0;
    }

    /* get the address for the kernel master global directory */
    if (linux_system_map_symbol_to_address(
            instance, "swapper_pg_dir", &kpgd) == XA_FAILURE){
        return 0;
    }
    memory = linux_access_physical_address(
                instance, kpgd - XA_PAGE_OFFSET, &offset);
    if (NULL == memory){
        return 0;
    }
    kpgd = *((uint32_t*)(memory + offset));
    munmap(memory, XA_PAGE_SIZE);

    /* perform the lookup in the global directory */
    index = (((virt_address) >> 22) & (1024 - 1));
    pgd_entry = kpgd + (index * sizeof(uint32_t));
    memory = linux_access_physical_address(
                instance, pgd_entry - XA_PAGE_OFFSET, &offset);
    if (NULL == memory){
        return 0;
    }
    pgd_entry = *((uint32_t*)(memory + offset));
    munmap(memory, XA_PAGE_SIZE);

    /* no page middle directory since we are assuming no PAE for now */

    /* perform the lookup in the page table */
    index = (((virt_address) >> 12) & (1024 - 1));
    pte_entry = (pgd_entry & 0xfffff000) + (index * sizeof(uint32_t));
    memory = linux_access_machine_address(instance, pte_entry, &offset);
    if (NULL == memory){
        return 0;
    }
    pte_entry = *((uint32_t*)(memory + offset));
    munmap(memory, XA_PAGE_SIZE);
    
    /* finally grab the location in memory */
    index = virt_address & 4095;
    return ((pte_entry & 0xfffff000) + index);
}

/* maps the page associated with the machine address */
void *linux_access_machine_address (
        xa_instance_t *instance, uint32_t mach_address, uint32_t *offset)
{
    unsigned long mfn;
    uint32_t bitmask;
    int i;

    /* machine frame number = machine address >> PAGE_SHIFT */
    mfn = mach_address >> XC_PAGE_SHIFT;

    /* get the offset */
    /*TODO why build this by hand everytime? */
    bitmask = 1;
    for (i = 0; i < XC_PAGE_SHIFT - 1; ++i){
        bitmask <<= 1;
        bitmask += 1;
    }
    *offset = mach_address & bitmask;

    /* access the memory */
    /*TODO allow r/rw to be passed as an argument to this function */
    /* return xa_mmap_pfn(instance, PROT_READ | PROT_WRITE, pfn); */
    return xa_mmap_mfn(instance, PROT_READ, mfn);
}

/* maps the page associated with a physical address */
void *linux_access_physical_address (
        xa_instance_t *instance, uint32_t phys_address, uint32_t *offset)
{
    unsigned long pfn;
    uint32_t bitmask;
    int i;

    /* page frame number = physical address >> PAGE_SHIFT */
    pfn = phys_address >> XC_PAGE_SHIFT;

    /* get the offset */
    /*TODO why build this by hand everytime? */
    bitmask = 1;
    for (i = 0; i < XC_PAGE_SHIFT - 1; ++i){
        bitmask <<= 1;
        bitmask += 1;
    }
    *offset = phys_address & bitmask;

    /* access the memory */
    /*TODO allow r/rw to be passed as an argument to this function */
    /* return xa_mmap_pfn(instance, PROT_READ | PROT_WRITE, pfn); */
    return xa_mmap_pfn(instance, PROT_READ, pfn);
}

/* maps the page associated with a virtual address */
void *linux_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
{
    /* this range is linear mapped */
    if (XA_PAGE_OFFSET < virt_address && virt_address < instance->high_memory){
        uint32_t phys_address = virt_address - XA_PAGE_OFFSET;
        return linux_access_physical_address(instance, phys_address, offset);
    }
    else if (instance->high_memory < virt_address && virt_address < 0xfe000000){
        uint32_t mach_address = linux_highmem_lookup(instance, virt_address); 
        if (!mach_address){
            printf("ERROR: address not in page table\n");
            return NULL;
        }
        return linux_access_machine_address(instance, mach_address, offset);
    }

    /*TODO this range is NOT linear mapped...need to find a way to
     * perform this mapping later */
    /*TODO two cases to handle here:
            2) 0xfe000000 < virt_address
            3) user space memory
    */
    /*TODO determine how to handle boundry cases (vaddr == high_memory?) */
    /*TODO note that 0xfe000000 should really be PKMAP_BASE */
    /*TODO remember than Xen is at top 64MB of virtual addr space */
    else{
        printf("ERROR: range not linearly mapped, cannot convert\n");
        return NULL;
    }
}

void *linux_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
{
    uint32_t virt_address;

    /* get the virtual address of the symbol */
    if (linux_system_map_symbol_to_address(
            instance, symbol, &virt_address) == XA_FAILURE){
        return NULL;
    }

    return linux_access_virtual_address(instance, virt_address, offset);
}
