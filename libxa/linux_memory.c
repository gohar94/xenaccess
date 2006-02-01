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

void *linux_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
{
    uint32_t phys_address;
    uint32_t high_memory;
    unsigned long pfn;
    uint32_t bitmask;
    int i;

    /*TODO assume 256 MB ram for now...need to fix this later */
    high_memory = 0xc0000000 + 0x10000000;

    /* this range is linear mapped */
    if (0xc0000000 < virt_address && virt_address < high_memory){
        phys_address = virt_address - 0xc0000000;
    }

    /*TODO this range is NOT linear mapped...need to find a way to
     * perform this mapping later */
    else{
        printf("ERROR: range not linearly mapped, cannot convert\n");
        return NULL;
    }

    /* page frame number = physical address >> PAGE_SHIFT */
    pfn = phys_address >> XC_PAGE_SHIFT;

    /* get the offset */
    bitmask = 1;
    for (i = 0; i < XC_PAGE_SHIFT - 1; ++i){
        bitmask <<= 1;
        bitmask += 1;
    }
    *offset = phys_address & bitmask;

    /* access the memory */
    return xa_mmap_pfn(instance, PROT_READ | PROT_WRITE, pfn);
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
