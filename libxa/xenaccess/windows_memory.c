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
 * This file contains utility functions for printing out data and
 * debugging information.
 *
 * File: windows_memory.c
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

/* Globals */
int xawin_tasks_offset = 0x88;
int xawin_pdbase_offset = 0x18;
int xawin_pid_offset = 0x84;
int xawin_peb_offset = 0x1b0;
int xawin_iba_offset = 0x8;
int xawin_ph_offset = 0x18;


/* finds the EPROCESS struct for a given pid */
unsigned char *windows_get_EPROCESS (
        xa_instance_t *instance, int pid, uint32_t *offset)
{
    unsigned char *memory = NULL;
    uint32_t list_head = 0, next_process = 0;
    int task_pid = 0;

    /* first we need a pointer to this pid's EPROCESS struct */
    next_process = instance->init_task;
    list_head = next_process;

    while (1){
        memory = xa_access_virtual_address(instance, next_process, offset);
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
               memory + *offset + xawin_pid_offset - xawin_tasks_offset,
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

    /* first we need a pointer to this pid's EPROCESS struct */
    memory = windows_get_EPROCESS(instance, pid, &offset);
    if (NULL == memory){
        printf("ERROR: could not find EPROCESS struct for pid = %d\n", pid);
        goto error_exit;
    }

    /* now follow the pointer to the memory descriptor and
       grab the pgd value */
    pgd = *((uint32_t*)(memory+offset+xawin_pdbase_offset-xawin_tasks_offset));
    pgd += instance->page_offset;
    munmap(memory, instance->page_size);

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return pgd;
}

/*
void *windows_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
*/

/* fills the taskaddr struct for a given windows process */
int xa_windows_get_peb (
        xa_instance_t *instance, int pid, xa_windows_peb_t *peb)
{
    unsigned char *memory;
    uint32_t ptr = 0, offset = 0;

    /* find the right EPROCESS struct */
    memory = windows_get_EPROCESS(instance, pid, &offset);
    if (NULL == memory){
        printf("ERROR: could not find EPROCESS struct for pid = %d\n", pid);
        goto error_exit;
    }
    ptr = *((uint32_t*)(memory+offset+xawin_peb_offset-xawin_tasks_offset));
    munmap(memory, instance->page_size);

    /* map the PEB struct */
    memory = xa_access_user_virtual_address(instance, ptr, &offset, pid);
    if (NULL == memory){
        printf("ERROR: could not find PEB struct for pid = %d\n", pid);
        goto error_exit;
    }

    /* copy appropriate values into peb struct */
    memcpy(peb->ImageBaseAddress,
           memory + offset + xawin_iba_offset - xawin_tasks_offset,
           sizeof(uint32_t));
    memcpy(peb->ProcessHeap,
           memory + offset + xawin_ph_offset - xawin_tasks_offset,
           sizeof(uint32_t));
    munmap(memory, instance->page_size);
    return XA_SUCCESS;

error_exit:
    if (memory) munmap(memory, instance->page_size);
    return XA_FAILURE;
}
