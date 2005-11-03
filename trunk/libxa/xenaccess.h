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
 * This file contains the public function definitions for the libxa
 * library.
 *
 * File: xenaccess.h
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */
#ifndef LIB_XEN_ACCESS_H
#define LIB_XEN_ACCESS_H

#include <xenctrl.h>

#define XA_SUCCESS 0
#define XA_FAILURE -1
#define XA_LIST_HEAD 0
#define XA_LIST_NODE 1
#define XA_LIST_TAIL 2
#define XA_PAGE_SIZE XC_PAGE_SIZE

typedef struct xa_instance{
    int xc_handle;
    uint32_t domain_id;
    xc_dominfo_t info;
} xa_instance_t;

/*--------------------------------------------------------
 * Initialization and Destruction functions from xa_core.c
 */

/**
 * Initializes access to a specific domU given a domain id.  The
 * domain id must represent an active domain and must be > 0.  All
 * calls to xa_init must eventually call xa_destroy.
 *
 * @param[in] domain_id Domain to access
 * @param[in,out] instance Struct that holds initialization information
 * @return 0 on success, -1 on failure
 */
int xa_init (uint32_t domain_id, xa_instance_t *instance);

/**
 * Destroys an instance by freeing memory and closing any open handles.
 *
 * @param[in] instance Instance to destroy
 * @return 0 on success, -1 on failure
 */
int xa_destroy (xa_instance_t *instance);

/*-----------------------------------------
 * Memory access functions from xa_memory.c
 */

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with the machine frame number.
 * This memory must be unmapped manually with munmap.
 *
 * @param[in] instance libxa instance
 * @param[in] prot Desired memory protection (see 'man mmap' for values)
 * @param[in] mfn Machine frame number
 * @return Mapped memory or NULL on error
 */
void *xa_mmap_mfn (xa_instance_t *instance, int prot, unsigned long mfn);

/**
 * Memory maps one page from domU to a local address range.  The
 * memory to be mapped is specified with the page frame number.
 * This memory must be unmapped manually with munmap.
 *
 * @param[in] instance libxa instance
 * @param[in] prot Desired memory protection (see 'man mmap' for values)
 * @param[in] pfn Page frame number
 * @return Mapped memory or NULL on error
 */
void *xa_mmap_pfn (xa_instance_t *instance, int prot, unsigned long pfn);


#endif /* LIB_XEN_ACCESS_H */
