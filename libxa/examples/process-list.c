/*
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
 * This file provides a simple example for walking through the list
 * of modules in a guest domain.
 *
 * File: module-list.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include "../xenaccess.h"

#define TASKS_OFFSET 24 * 4
#define NAME_OFFSET 106 * 4

int main (int argc, char **argv)
{
    xa_instance_t xai;
    unsigned char *memory = NULL;
    uint32_t offset, next_process, list_head;
    char *name = NULL;

    /* this is the domain ID that we are looking at */
    /* this is hard coded for keep this example code simple */
    uint32_t dom = 1;

    /* initialize the xen access library */
    if (xa_init(dom, &xai) == XA_FAILURE){
        perror("failed to init XenAccess library");
        goto error_exit;
    }

    /* get the head of the module list */
    memory = xa_access_kernel_symbol(&xai, "init_task", &offset);
    if (NULL == memory){
        perror("failed to get process list head");
        goto error_exit;
    }
    memcpy(&next_process, memory + offset + TASKS_OFFSET, 4);
    list_head = next_process;
    munmap(memory, XA_PAGE_SIZE);

    /* walk the module list */
    while (1){

        /* Note: the next two steps are equiv to something like
           'next_process = next_process->next' except that we can't
           use pointers directly since we are in someone else's
           memory space! */

        /* follow the next pointer */
        memory = xa_access_virtual_address(&xai, next_process, &offset);
        if (NULL == memory){
            printf("failed on next_process = 0x%.8x\n", next_process);
            perror("failed to map memory for process list pointer");
            goto error_exit;
        }

        /* update the next pointer */
        memcpy(&next_process, memory + offset, 4);

        /* if we are back at the list head, we are done */
        if (list_head == next_process){
            printf("exiting b/c next_process = 0x%.8x\n", next_process);
            break;
        }

        /* print out the process name */

        /* Note: the module struct that we are looking at has a string
           directly following the next / prev pointers.  This is why you
           can just add 420 to get the name.  See include/linux/sched.h
           for mode details */
        name = (char *) (memory + offset + NAME_OFFSET - TASKS_OFFSET);
        printf("%s\n", name);
        munmap(memory, XA_PAGE_SIZE);
    }

error_exit:

    /* cleanup any memory associated with the XenAccess instance */
    xa_destroy(&xai);

    /* sanity check to unmap shared pages */
    if (memory) munmap(memory, XA_PAGE_SIZE);

    return 0;
}

