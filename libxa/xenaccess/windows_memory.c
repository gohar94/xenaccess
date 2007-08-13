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

/* finds the task struct for a given pid */
/*
unsigned char *windows_get_taskstruct (
        xa_instance_t *instance, int pid, uint32_t *offset)
*/

/* finds the address of the page global directory for a given pid */
/*
uint32_t windows_pid_to_pgd (xa_instance_t *instance, int pid)
*/

/*
uint32_t windows_pagetable_lookup (
            xa_instance_t *instance,
            uint32_t pgd,
            uint32_t virt_address,
            int kernel)
*/

/* maps the page associated with the machine address */
/*
void *windows_access_machine_address (
        xa_instance_t *instance, uint32_t mach_address, uint32_t *offset)

void *windows_access_machine_address_rw (
        xa_instance_t *instance, uint32_t mach_address,
        uint32_t *offset, int prot)
*/

/*
void *windows_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
*/

/* maps the page associated with a virtual address */
/*
void *windows_access_user_virtual_address (
            xa_instance_t *instance,
            uint32_t virt_address,
            uint32_t *offset,
            int pid)
*/

/*
void *windows_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
*/

/* fills the taskaddr struct for a given linux process */
/*
int xa_windows_get_taskaddr (
        xa_instance_t *instance, int pid, xa_linux_taskaddr_t *taskaddr)
*/
