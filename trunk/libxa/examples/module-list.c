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

int main (int argc, char **argv)
{
    xa_instance_t xai;
    unsigned char *memory = NULL;
    uint32_t offset, next_module, list_head;
    char *name = NULL;

    /* this is the domain ID that we are looking at */
    uint32_t dom = atoi(argv[1]);

    /* initialize the xen access library */
    if (xa_init(dom, &xai) == XA_FAILURE){
        perror("failed to init XenAccess library");
        goto error_exit;
    }

    /* get the head of the module list */
    memory = xa_access_kernel_symbol(&xai, "modules", &offset);
    if (NULL == memory){
        perror("failed to get module list head");
        goto error_exit;
    }
    memcpy(&next_module, memory + offset, 4);
    list_head = next_module;
    munmap(memory, XA_PAGE_SIZE);

    /* walk the module list */
    while (1){

        /* Note: the next two steps are equiv to something like
           'next_module = next_module->next' except that we can't
           use pointers directly since we are in someone else's
           memory space! */

        /* follow the next pointer */
        memory = xa_access_virtual_address(&xai, next_module, &offset);
        if (NULL == memory){
            perror("failed to map memory for module list pointer");
            goto error_exit;
        }

        /* update the next pointer */
        memcpy(&next_module, memory + offset, 4);

        /* if we are back at the list head, we are done */
        if (list_head == next_module){
            break;
        }

        /* print out the module name */

        /* Note: the module struct that we are looking at has a string
           directly following the next / prev pointers.  This is why you
           can just add 8 to get the name.  See include/linux/module.h
           for mode details */
        name = (char *) (memory + offset + 8);
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

