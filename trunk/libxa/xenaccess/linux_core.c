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
 * Core linux functionality, primarily iniitalization routines for now.
 *
 * File: linux_core.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 */

#include "xenaccess.h"
#include "xa_private.h"

int linux_init (xa_instance_t *instance)
{
    int ret = XA_SUCCESS;
    unsigned char *memory = NULL;
    uint32_t local_offset = 0;

    if (linux_system_map_symbol_to_address(
             instance, "swapper_pg_dir", &instance->kpgd) == XA_FAILURE){
        printf("ERROR: failed to lookup 'swapper_pg_dir' address\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    xa_dbprint("--got vaddr for swapper_pg_dir (0x%.8x).\n", instance->kpgd);

    if (!instance->hvm){
        instance->kpgd -= instance->page_offset;
        instance->kpgd = xa_read_long_phys(instance, instance->kpgd);
        if (!instance->kpgd){
            printf("ERROR: failed to get physical addr for kpgd\n");
            ret = XA_FAILURE;
            goto error_exit;
        }
    }
    xa_dbprint("**set instance->kpgd (0x%.8x).\n", instance->kpgd);

    memory = xa_access_kernel_symbol(instance, "init_task", &local_offset);
    if (NULL == memory){
        printf("ERROR: failed to get task list head 'init_task'\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    instance->init_task =
        *((uint32_t*)(memory + local_offset + xalinux_tasks_offset));
    xa_dbprint("**set instance->init_task (0x%.8x).\n", instance->init_task);
    munmap(memory, instance->page_size);

error_exit:
    return ret;
}