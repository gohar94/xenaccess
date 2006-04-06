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
#include <string.h>
#include <sys/mman.h>
#include "xa_private.h"

/* offset to each of these fields from the beginning of the struct
   assuming that CONFIG_SCHEDSTATS is not defined  and CONFIG_KEYS
   is defined in the guest's kernel (this is the default in xen) */
#define XALINUX_TASKS_OFFSET 24 * 4   /* task_struct->tasks */
#define XALINUX_MM_OFFSET 30 * 4      /* task_struct->mm */
#define XALINUX_PID_OFFSET 39 * 4     /* task_struct->pid */
#define XALINUX_NAME_OFFSET 106 * 4   /* task_struct->comm */
#define XALINUX_PGD_OFFSET 7 * 4      /* mm_struct->pgd */

/* finds the address of the page global directory for a given pid */
uint32_t linux_pid_to_pgd (xa_instance_t *instance, int pid)
{
    unsigned char *memory = NULL;
    uint32_t pgd = 0, list_head = 0, next_process = 0, ptr = 0, offset = 0;
    int task_pid = 0;

    /* first we need a pointer to this pid's task_struct */
    memory = xa_access_kernel_symbol(instance, "init_task", &offset);
    if (NULL == memory){
        printf("ERROR: failed to get task list head 'init_task'\n");
        goto error_exit;
    }
    memcpy(&next_process, memory + offset + XALINUX_TASKS_OFFSET, 4);
    list_head = next_process;
    munmap(memory, XA_PAGE_SIZE);

    while (1){
        memory = xa_access_virtual_address(instance, next_process, &offset);
        if (NULL == memory){
            printf("ERROR: failed to get task list next pointer");
            goto error_exit;
        }
        memcpy(&next_process, memory + offset, 4);

        /* if we are back at the list head, we are done */
        if (list_head == next_process){
            goto error_exit;
        }

        memcpy(&task_pid,
               memory + offset + XALINUX_PID_OFFSET - XALINUX_TASKS_OFFSET,
               4
        );
        
        /* if pid matches, then we found what we want */
        if (task_pid == pid){
            break;
        }
        munmap(memory, XA_PAGE_SIZE);
    }

    /* now follow the pointer to the memory descriptor and
       grab the pgd value */
    memcpy(&ptr, memory + offset + XALINUX_MM_OFFSET - XALINUX_TASKS_OFFSET, 4);
    munmap(memory, XA_PAGE_SIZE);
    memory = xa_access_virtual_address(instance, ptr, &offset);
    if (NULL == memory){
        printf("ERROR: failed to follow mm pointer");
        goto error_exit;
    }
    memcpy(&pgd, memory + offset + XALINUX_PGD_OFFSET, 4);

error_exit:
    if (memory) munmap(memory, XA_PAGE_SIZE);
    return pgd;
}

/* converty address to machine address via page tables */
uint32_t linux_pagetable_lookup (
            xa_instance_t *instance,
            uint32_t page_table_base,
            uint32_t virt_address)
{
    uint32_t index = 0;
    uint32_t offset = 0;
    uint32_t pgd_entry = 0;
    uint32_t pte_entry = 0;
    unsigned char *memory = NULL;

    /* perform the lookup in the global directory */
    index = (((virt_address) >> 22) & (1024 - 1));
    pgd_entry = page_table_base + (index * sizeof(uint32_t));
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

void *linux_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
{
    return linux_access_user_virtual_address(instance, virt_address, offset, 0);
}

/* maps the page associated with a virtual address */
void *linux_access_user_virtual_address (
            xa_instance_t *instance,
            uint32_t virt_address,
            uint32_t *offset,
            int pid)
{
    /* use kernel page tables */
    /*TODO HYPERVISOR_VIRT_START = 0xFC000000 so we can't go over that.
      Figure out what this should be b/c there still may be a fixed
      mapping range between the page'd addresses and VIRT_START */
    if (XA_PAGE_OFFSET <= virt_address && virt_address < 0xfc000000){
        uint32_t kpgd = 0;
        uint32_t mach_address = 0;
        uint32_t local_offset = 0;
        unsigned char *memory = NULL;

        /* get the address for the kernel master global directory */
        if (linux_system_map_symbol_to_address(
                instance, "swapper_pg_dir", &kpgd) == XA_FAILURE){
            return NULL;
        }
        memory = linux_access_physical_address(
                    instance, kpgd - XA_PAGE_OFFSET, &local_offset);
        if (NULL == memory){
            return NULL;
        }
        kpgd = *((uint32_t*)(memory + local_offset));
        munmap(memory, XA_PAGE_SIZE);

        mach_address = linux_pagetable_lookup(instance, kpgd, virt_address); 
        if (!mach_address){
            printf("ERROR: address not in page table\n");
            return NULL;
        }
        return linux_access_machine_address(instance, mach_address, offset);
    }

    /* use user page tables */
    else if (virt_address < XA_PAGE_OFFSET && pid != 0){
        uint32_t pgd = linux_pid_to_pgd(instance, pid);
        uint32_t mach_address = 0;

        if (!pgd){
            mach_address = linux_pagetable_lookup(instance, pgd, virt_address); 
        }
        if (!mach_address){
            printf("ERROR: address not in page table\n");
            return NULL;
        }
        return linux_access_machine_address(instance, mach_address, offset);
    }

    /* something's not right, we can't find this memory */
    else{
        printf("ERROR: cannot translate this address\n");
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
