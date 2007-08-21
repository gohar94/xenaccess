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


/* Globals */
int xalinux_tasks_offset    = 0x60;
int xalinux_mm_offset       = 0x78;
int xalinux_pid_offset      = 0x9c;
int xalinux_name_offset     = 0x1b0;
int xalinux_pgd_offset      = 0x24;
int xalinux_addr_offset     = 0x80;

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
               memory + *offset + xalinux_pid_offset - xalinux_tasks_offset,
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
    memcpy(&ptr, memory + offset + xalinux_mm_offset - xalinux_tasks_offset, 4);
    munmap(memory, instance->page_size);
    memory = xa_access_virtual_address(instance, ptr, &offset);
    if (NULL == memory){
        printf("ERROR: failed to follow mm pointer");
        goto error_exit;
    }
    /* memcpy(&pgd, memory + offset + xalinux_pgd_offset, 4); */
    pgd = *((uint32_t*)(memory + offset + xalinux_pgd_offset));

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return pgd;
}

void *linux_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
{
    uint32_t virt_address;
    uint32_t address;

    /* check the LRU cache */
    if (xa_check_cache_sym(symbol, 0, &address)){
        return xa_access_machine_address(instance, address, offset);
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
    memcpy(&ptr, memory + offset + xalinux_mm_offset - xalinux_tasks_offset, 4);
    munmap(memory, instance->page_size);
    memory = xa_access_virtual_address(instance, ptr, &offset);
    if (NULL == memory){
        printf("ERROR: failed to follow mm pointer (0x%x)\n", ptr);
        goto error_exit;
    }
    memcpy(
        taskaddr,
        memory + offset + xalinux_addr_offset,
        sizeof(xa_linux_taskaddr_t)
    );
    munmap(memory, instance->page_size);

    return XA_SUCCESS;

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return XA_FAILURE;
}
