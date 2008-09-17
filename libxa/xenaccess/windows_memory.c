/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005 - 2008  Bryan D. Payne (bryan@thepaynes.cc)
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
 * This file contains utility functions for printing out data and
 * debugging information.
 *
 * File: windows_memory.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "xa_private.h"

/* brute force to find most likely location */
int bf_test_ntoskrnl_base (xa_instance_t *instance, uint32_t base)
{
    uint32_t header = 0;
    int offset = 0;
    uint32_t testval;
    int count = 0;

    for (offset = 0; offset < 0xd0000; offset += 4){
        xa_read_long_phys(instance, base + offset, &testval);
        if (testval > instance->page_offset){
            testval -= instance->page_offset;
            xa_read_long_phys(instance, testval, &header);
            if (header == 0x001b0003){
                count++;
            }
        }
    }

    return count;
}

/* test a candidate location */
int test_ntoskrnl_base (
        xa_instance_t *instance, uint32_t base, uint32_t sysproc)
{
    uint32_t header = 0;

    sysproc += base;
    xa_read_long_phys(instance, sysproc, &sysproc);
    if (sysproc <= instance->page_offset){
        return XA_FAILURE;
    }
    sysproc -= instance->page_offset;

    /* sysproc should now be the PA of a an EPROCESS location */
    xa_read_long_phys(instance, sysproc, &header);
    
    /* Test for EPROCESS using technique from 'Searching for Processes
       and Threads in Microsoft Windows memory dumps' in DFRWS 2006 */
    if (header != 0x001b0003){
        return XA_FAILURE;
    }
    return XA_SUCCESS;
}

/* find the ntoskrnl base address by brute force scanning */
uint32_t bf_get_ntoskrnl_base (xa_instance_t *instance)
{
    uint32_t paddr = 0x0 + instance->page_size;
    int best_count = 0;
    uint32_t best_answer = 0;

    /* start the downward search looking for MZ header */
    while (1){
        uint32_t header;
        xa_read_long_phys(instance, paddr, &header);
        if ((header & 0xffff) == 0x5a4d){
            int count = 0;
            if ((count = bf_test_ntoskrnl_base(instance, paddr)) > best_count){
                best_count = count;
                best_answer = paddr;
            }
        }

        paddr += instance->page_size;
        if (paddr <= 0 || 0x01000000 <= paddr){
            break;
        }
    }

    return best_answer;
}

/* find the ntoskrnl base address by backwards scanning */
uint32_t get_ntoskrnl_base (xa_instance_t *instance)
{
    uint32_t paddr = 0x0 + instance->page_size;
    uint32_t sysproc_rva;

    /* find RVA for PsInitialSystemProcess to use for testing */
    if (windows_symbol_to_address(
            instance, "PsInitialSystemProcess", &sysproc_rva) == XA_FAILURE){
        return 0;
    }

    /* start the downward search looking for MZ header */
    while (1){
        uint32_t header;
        xa_read_long_phys(instance, paddr, &header);
        if ((header & 0xffff) == 0x5a4d){
            if (test_ntoskrnl_base(instance, paddr, sysproc_rva) == XA_SUCCESS){
                break;
            }
        }

        paddr += instance->page_size;
        if (paddr <= 0 || 0x01000000 <= paddr){
            xa_dbprint("--get_ntoskrnl_base failed, switching to search\n");
            return bf_get_ntoskrnl_base(instance);
        }
    }

    return paddr;
}

void *windows_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset, int prot)
{
    uint32_t virt_address;
    uint32_t phys_address;
    uint32_t address;
    uint32_t rva;

    /* check the LRU cache */
    if (xa_check_cache_sym(instance, symbol, 0, &address)){
        return xa_access_ma(instance, address, offset, PROT_READ);
    }

    /* get the RVA of the symbol */
    if (windows_symbol_to_address(instance, symbol, &rva) == XA_FAILURE){
        return NULL;
    }

    /* convert RVA into virt address */
    phys_address = instance->os.windows_instance.ntoskrnl + rva;
    virt_address = phys_address + instance->page_offset;

    xa_update_cache(instance, symbol, virt_address, 0, 0);
    return xa_access_pa(instance, phys_address, offset, prot);
}

/* finds the EPROCESS struct for a given pid */
unsigned char *windows_get_EPROCESS (
        xa_instance_t *instance, int pid, uint32_t *offset)
{
    unsigned char *memory = NULL;
    uint32_t list_head = 0, next_process = 0;
    int task_pid = 0;
    int pid_offset = instance->os.windows_instance.pid_offset;
    int tasks_offset = instance->os.windows_instance.tasks_offset;

    /* first we need a pointer to this pid's EPROCESS struct */
    next_process = instance->init_task;
    list_head = next_process;

    while (1){
        memory = xa_access_kernel_va(instance, next_process, offset, PROT_READ);
        if (NULL == memory){
            printf("ERROR: failed to get EPROCESS list next pointer");
            goto error_exit;
        }
        memcpy(&next_process, memory + *offset, 4);

        /* if we are back at the list head, we are done */
        if (list_head == next_process){
            goto error_exit;
        }

        memcpy(&task_pid,
               memory + *offset + pid_offset - tasks_offset,
               4
        );

        /* if pid matches, then we found what we want */
        if (task_pid == pid){
            return memory;
        }
        munmap(memory, instance->page_size);
    }

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return NULL;
}

/* finds the address of the page global directory for a given pid */
uint32_t windows_pid_to_pgd (xa_instance_t *instance, int pid)
{
    unsigned char *memory = NULL;
    uint32_t pgd = 0, ptr = 0, offset = 0;
    int pdbase_offset = instance->os.windows_instance.pdbase_offset;
    int tasks_offset = instance->os.windows_instance.tasks_offset;

    /* first we need a pointer to this pid's EPROCESS struct */
    memory = windows_get_EPROCESS(instance, pid, &offset);
    if (NULL == memory){
        printf("ERROR: could not find EPROCESS struct for pid = %d\n", pid);
        goto error_exit;
    }

    /* now follow the pointer to the memory descriptor and
       grab the pgd value */
    pgd = *((uint32_t*)(memory + offset + pdbase_offset - tasks_offset));
    munmap(memory, instance->page_size);

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return pgd;
}

/* fills the taskaddr struct for a given windows process */
int xa_windows_get_peb (
        xa_instance_t *instance, int pid, xa_windows_peb_t *peb)
{
    unsigned char *memory;
    uint32_t ptr = 0, offset = 0;
    int peb_offset = instance->os.windows_instance.peb_offset;
    int tasks_offset = instance->os.windows_instance.tasks_offset;
    int iba_offset = instance->os.windows_instance.iba_offset;
    int ph_offset = instance->os.windows_instance.ph_offset;

    /* find the right EPROCESS struct */
    memory = windows_get_EPROCESS(instance, pid, &offset);
    if (NULL == memory){
        printf("ERROR: could not find EPROCESS struct for pid = %d\n", pid);
        goto error_exit;
    }
    ptr = *((uint32_t*)(memory+offset + peb_offset - tasks_offset));
    munmap(memory, instance->page_size);

    /* map the PEB struct */
    memory = xa_access_user_va(instance, ptr, &offset, pid, PROT_READ);
    if (NULL == memory){
        printf("ERROR: could not find PEB struct for pid = %d\n", pid);
        goto error_exit;
    }

    /* copy appropriate values into peb struct */
    memcpy(&peb->ImageBaseAddress,
           memory + offset + iba_offset - tasks_offset,
           sizeof(uint32_t));
    memcpy(&peb->ProcessHeap,
           memory + offset + ph_offset - tasks_offset,
           sizeof(uint32_t));
    munmap(memory, instance->page_size);
    return XA_SUCCESS;

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return XA_FAILURE;
}
