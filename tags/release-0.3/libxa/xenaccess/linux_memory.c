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
 * $Date: 2006-12-06 01:23:30 -0500 (Wed, 06 Dec 2006) $
 */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "xa_private.h"

/* finds the task struct for a given pid */
unsigned char *linux_get_taskstruct (
        xa_instance_t *instance, int pid, uint32_t *offset)
{
    unsigned char *memory = NULL;
    uint32_t list_head = 0, next_process = 0;
    int task_pid = 0;

    /* first we need a pointer to this pid's task_struct */
    next_process = instance->init_task;
    list_head = next_process;

    while (1){
        memory = xa_access_virtual_address(instance, next_process, offset);
        if (NULL == memory){
            printf("ERROR: failed to get task list next pointer");
            goto error_exit;
        }
        memcpy(&next_process, memory + *offset, 4);

        /* if we are back at the list head, we are done */
        if (list_head == next_process){
            goto error_exit;
        }

        memcpy(&task_pid,
               memory + *offset + XALINUX_PID_OFFSET - XALINUX_TASKS_OFFSET,
               4
        );
        
        /* if pid matches, then we found what we want */
        if (task_pid == pid){
            return memory;
        }
        munmap(memory, XA_PAGE_SIZE);
    }

error_exit:
    if (memory) munmap(memory, XA_PAGE_SIZE);
    return NULL;
}

/* finds the address of the page global directory for a given pid */
uint32_t linux_pid_to_pgd (xa_instance_t *instance, int pid)
{
    unsigned char *memory = NULL;
    uint32_t pgd = 0, ptr = 0, offset = 0;

    /* first we need a pointer to this pid's task_struct */
    memory = linux_get_taskstruct(instance, pid, &offset);
    if (NULL == memory){
        printf("ERROR: could not find task struct for pid = %d\n", pid);
        goto error_exit;
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
    /* memcpy(&pgd, memory + offset + XALINUX_PGD_OFFSET, 4); */
    pgd = *((uint32_t*)(memory + offset + XALINUX_PGD_OFFSET));

error_exit:
    if (memory) munmap(memory, XA_PAGE_SIZE);
    return pgd;
}

/* TODO: make this portable (PAE, x86_64) */
/* converty address to machine address via page tables */
uint32_t linux_pagetable_lookup (
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

    /* perform the lookup in the global directory */
    index = (((virt_address) >> 22) & (1024 - 1));
    pgd_entry = pgd + (index * sizeof(uint32_t));
    if (kernel){
        memory = linux_access_physical_address(
                instance, pgd_entry - XA_PAGE_OFFSET, &offset);
    }
    else{
        memory = linux_access_virtual_address(instance, pgd_entry, &offset);
    }
    if (NULL == memory){
        printf("ERROR: pgd entry lookup failed (phys addr = 0x%.8x)\n",
                pgd_entry - XA_PAGE_OFFSET);
        printf("** pgd = 0x%.8x\n", pgd);
        printf("** pgd_entry = 0x%.8x\n", pgd_entry);
        printf("** vaddr = 0x%.8x\n", virt_address);
        return 0;
    }
    pgd_entry = *((uint32_t*)(memory + offset));
    munmap(memory, XA_PAGE_SIZE);

    /* no page middle directory since we are assuming no PAE for now */

    /* perform the lookup in the page table */
    index = (((virt_address) >> 12) & (1024 - 1));
    pte_entry = (pgd_entry & 0xfffff000) + (index * sizeof(uint32_t));
    if (instance->hvm){
        memory = linux_access_physical_address(instance, pte_entry, &offset);
    }
    else{
        memory = linux_access_machine_address(instance, pte_entry, &offset);
    }
    if (NULL == memory){
        printf("ERROR: pte entry lookup failed (mach addr = 0x%.8x)\n",
                pte_entry);
        printf("** pgd_entry = 0x%.8x\n", pgd_entry);
        printf("** pte_entry = 0x%.8x\n", pte_entry);
        printf("** vaddr = 0x%.8x\n", virt_address);
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
    return linux_access_machine_address_rw(
                instance, mach_address, offset, PROT_READ);
}

void *linux_access_machine_address_rw (
        xa_instance_t *instance, uint32_t mach_address,
        uint32_t *offset, int prot)
{
    unsigned long mfn;
    int i;

    /* machine frame number = machine address >> PAGE_SHIFT */
    mfn = mach_address >> XC_PAGE_SHIFT;

    /* get the offset */
    *offset = (XC_PAGE_SIZE-1) & mach_address;

    /* access the memory */
    /*TODO allow r/rw to be passed as an argument to this function */
    /* return xa_mmap_pfn(instance, PROT_READ | PROT_WRITE, pfn); */
    return xa_mmap_mfn(instance, prot, mfn);
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
    *offset = (XC_PAGE_SIZE-1) & phys_address;

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
    uint32_t address = 0;

    /* check the LRU cache */
    if (xa_check_cache_virt(virt_address, pid, &address)){
        if (instance->hvm){
            return linux_access_physical_address(instance, address, offset);
        }
        else{
            return linux_access_machine_address(instance, address, offset);
        }
    }

    /* use kernel page tables */
    /*TODO HYPERVISOR_VIRT_START = 0xFC000000 so we can't go over that.
      Figure out what this should be b/c there still may be a fixed
      mapping range between the page'd addresses and VIRT_START */
    if (!pid){
        address = linux_pagetable_lookup(
                            instance, instance->kpgd, virt_address, 1); 
        if (!address){
            printf("ERROR: address not in page table\n");
            return NULL;
        }
        xa_update_cache(NULL, virt_address, pid, address);
        if (instance->hvm){
            return linux_access_physical_address(instance, address, offset);
        }
        else{
            return linux_access_machine_address(instance, address, offset);
        }
    }

    /* use user page tables */
    else{
        uint32_t pgd = linux_pid_to_pgd(instance, pid);

        if (pgd){
            address = linux_pagetable_lookup(instance, pgd, virt_address, 0); 
        }

        if (!address){
            printf("ERROR: address not in page table (0x%x)\n", virt_address);
            return NULL;
        }
        xa_update_cache(NULL, virt_address, pid, address);
        return linux_access_machine_address_rw(
            instance, address, offset, PROT_READ | PROT_WRITE);
    }
}

void *linux_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
{
    uint32_t virt_address;
    uint32_t address;

    /* check the LRU cache */
    if (xa_check_cache_sym(symbol, 0, &address)){
        if (instance->hvm){
            return linux_access_physical_address(instance, address, offset);
        }
        else{
            return linux_access_machine_address(instance, address, offset);
        }
    }

    /* get the virtual address of the symbol */
    if (linux_system_map_symbol_to_address(
            instance, symbol, &virt_address) == XA_FAILURE){
        return NULL;
    }

    xa_update_cache(symbol, virt_address, 0, 0);
    return xa_access_virtual_address(instance, virt_address, offset);
}

/* fills the taskaddr struct for a given linux process */
int xa_linux_get_taskaddr (
        xa_instance_t *instance, int pid, xa_linux_taskaddr_t *taskaddr)
{
    unsigned char *memory;
    uint32_t ptr = 0, offset = 0;

    /* find the right task struct */
    memory = linux_get_taskstruct(instance, pid, &offset);
    if (NULL == memory){
        printf("ERROR: could not find task struct for pid = %d\n", pid);
        goto error_exit;
    }

    /* copy the information out of the memory descriptor */
    memcpy(&ptr, memory + offset + XALINUX_MM_OFFSET - XALINUX_TASKS_OFFSET, 4);
    munmap(memory, XA_PAGE_SIZE);
    memory = xa_access_virtual_address(instance, ptr, &offset);
    if (NULL == memory){
        printf("ERROR: failed to follow mm pointer (0x%x)\n", ptr);
        goto error_exit;
    }
    memcpy(
        taskaddr,
        memory + offset + XALINUX_ADDR_OFFSET,
        sizeof(xa_linux_taskaddr_t)
    );

    return XA_SUCCESS;

error_exit:
    if (memory) munmap(memory, XA_PAGE_SIZE);
    return XA_FAILURE;
}
