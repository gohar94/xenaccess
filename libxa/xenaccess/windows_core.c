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
 * Core windows functionality, primarily iniitalization routines for now.
 *
 * File: windows_core.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 */

#include "xenaccess.h"
#include "xa_private.h"

int windows_init (xa_instance_t *instance)
{
    int ret = XA_SUCCESS;
    uint32_t sysproc = 0;

    /* get base address for kernel image in memory */
    instance->ntoskrnl = get_ntoskrnl_base(instance);
    if (!instance->ntoskrnl){
        ret = XA_FAILURE;
        goto error_exit;
    }
    xa_dbprint("--got ntoskrnl (0x%.8x).\n", instance->ntoskrnl);

    /* get address for page directory (from system process) */
    if (xa_read_long_sym(
            instance, "PsInitialSystemProcess", &sysproc) == XA_FAILURE){
        printf("ERROR: failed to resolve pointer for system process\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    sysproc -= instance->page_offset; /* PA to PsInit.. */
    xa_dbprint("--got PA to PsInititalSystemProcess (0x%.8x).\n", sysproc);

    if (xa_read_long_phys(
            instance,
            sysproc + xawin_pdbase_offset,
            &(instance->kpgd)) == XA_FAILURE){
        printf("ERROR: failed to resolve pointer for system process\n");
        ret = XA_FAILURE;
        goto error_exit;
    }
    instance->kpgd += instance->page_offset; /* store vaddr */
    xa_dbprint("**set instance->kpgd (0x%.8x).\n", instance->kpgd);

    /* get address start of process list */
    xa_read_long_phys(
        instance, sysproc + xawin_tasks_offset, &(instance->init_task));
    xa_dbprint("**set instance->init_task (0x%.8x).\n", instance->init_task);

error_exit:
    return ret;
}
