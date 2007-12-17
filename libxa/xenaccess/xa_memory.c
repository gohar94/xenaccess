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
 * This file contains functions for sharing the memory of a domU
 * machine.  The functions basically only differ in how the memory
 * is referenced (pfn, mfn, virtual address, physical address, etc).
 *
 * File: xa_memory.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date: 2006-11-29 20:38:20 -0500 (Wed, 29 Nov 2006) $
 */

#include <stdlib.h>
#include <sys/mman.h>
#include "xenaccess.h"
#include "xa_private.h"

/* hack to get this to compile on xen 3.0.4 */
#ifndef XENMEM_maximum_gpfn
#define XENMEM_maximum_gpfn 0
#endif

/* convert a pfn to a mfn based on the live mapping tables */
unsigned long helper_pfn_to_mfn (xa_instance_t *instance, unsigned long pfn)
{
    shared_info_t *live_shinfo = NULL;
    unsigned long *live_pfn_to_mfn_frame_list_list = NULL;
    unsigned long *live_pfn_to_mfn_frame_list = NULL;

    /* Live mapping of the table mapping each PFN to its current MFN. */
    unsigned long *live_pfn_to_mfn_table = NULL;
    unsigned long nr_pfns = 0;
    unsigned long ret = -1;
//    unsigned long mfn;
//    int i;

    if (instance->hvm){
        return pfn;
    }

    if (NULL == instance->live_pfn_to_mfn_table){
        live_shinfo = xa_mmap_mfn(
            instance, PROT_READ, instance->info.shared_info_frame);
        if (live_shinfo == NULL){
            printf("ERROR: failed to init live_shinfo\n");
            goto error_exit;
        }

        if (instance->xen_version == XA_XENVER_3_1_0){
            nr_pfns = xc_memory_op(
                        instance->xc_handle,
                        XENMEM_maximum_gpfn,
                        &(instance->domain_id)) + 1;
        }
        else{
            //nr_pfns = instance->info.max_memkb >> (XC_PAGE_SHIFT - 10);
            nr_pfns = live_shinfo->arch.max_pfn;
        }

        live_pfn_to_mfn_frame_list_list = xa_mmap_mfn(
            instance, PROT_READ, live_shinfo->arch.pfn_to_mfn_frame_list_list);
        if (live_pfn_to_mfn_frame_list_list == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_frame_list_list\n");
            goto error_exit;
        }

        live_pfn_to_mfn_frame_list = xc_map_foreign_batch(
            instance->xc_handle, instance->domain_id, PROT_READ,
            live_pfn_to_mfn_frame_list_list,
            (nr_pfns+(fpp*fpp)-1)/(fpp*fpp) );
        if (live_pfn_to_mfn_frame_list == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_frame_list\n");
            goto error_exit;
        }

        live_pfn_to_mfn_table = xc_map_foreign_batch(
            instance->xc_handle, instance->domain_id, PROT_READ,
            live_pfn_to_mfn_frame_list, (nr_pfns+fpp-1)/fpp );
        if (live_pfn_to_mfn_table  == NULL){
            printf("ERROR: failed to init live_pfn_to_mfn_table\n");
            goto error_exit;
        }

        /*TODO validate the mapping */
//        for (i = 0; i < nr_pfns; ++i){
//            mfn = live_pfn_to_mfn_table[i];
//            if( (mfn != INVALID_P2M_ENTRY) && (mfn_to_pfn(mfn) != i) )
//            {
//                DPRINTF("i=0x%x mfn=%lx live_m2p=%lx\n", i,
//                        mfn, mfn_to_pfn(mfn));
//                err++;
//            }
//        }

        /* save mappings for later use */
        instance->live_pfn_to_mfn_table = live_pfn_to_mfn_table;
        instance->nr_pfns = nr_pfns;
    }

    ret = instance->live_pfn_to_mfn_table[pfn];

error_exit:
    if (live_shinfo) munmap(live_shinfo, XC_PAGE_SIZE);
    if (live_pfn_to_mfn_frame_list_list)
        munmap(live_pfn_to_mfn_frame_list_list, XC_PAGE_SIZE);
    if (live_pfn_to_mfn_frame_list)
        munmap(live_pfn_to_mfn_frame_list, XC_PAGE_SIZE);

    return ret;
}

void *xa_mmap_mfn (xa_instance_t *instance, int prot, unsigned long mfn)
{
    xa_dbprint("--MapMFN: Mapping mfn = 0x%.8x.\n", (unsigned int)mfn);
    return xc_map_foreign_range(
        instance->xc_handle, instance->domain_id, 1, prot, mfn);
}

void *xa_mmap_pfn (xa_instance_t *instance, int prot, unsigned long pfn)
{
    unsigned long mfn;

    mfn = helper_pfn_to_mfn(instance, pfn);

    if (-1 == mfn){
        printf("ERROR: pfn to mfn mapping failed.\n");
        return NULL;
    }
    else{
        xa_dbprint("--MapPFN: Mapping mfn = %lu.\n", mfn);
        return xc_map_foreign_range(
            instance->xc_handle, instance->domain_id, 1, prot, mfn);
    }
}

/* bit flag testing */
int entry_present (unsigned long entry){
    return xa_get_bit(entry, 0);
}

int page_size_flag (unsigned long entry){
    return xa_get_bit(entry, 7);
}

/* page directory pointer table */
uint32_t get_pdptb (uint32_t pdpr){
    return pdpr & 0xFFFFFFE0;
}

uint32_t pdpi_index (uint32_t pdpi){
    return (pdpi >> 30) * sizeof(uint64_t);
}

uint64_t get_pdpi (
        xa_instance_t *instance, uint32_t vaddr, uint32_t cr3, int k)
{
    uint64_t value;
    uint32_t pdpi_entry = get_pdptb(cr3) + pdpi_index(vaddr);
    xa_dbprint("--PTLookup: pdpi_entry = 0x%.8x\n", pdpi_entry);
    if (k){
        xa_read_long_long_phys(
            instance, pdpi_entry-instance->page_offset, &value);
    }
    else{
        xa_read_long_long_virt(instance, pdpi_entry, 0, &value);
    }
    return value;
}

/* page directory */
uint32_t pgd_index (xa_instance_t *instance, uint32_t address){
    if (!instance->pae){
        return (((address) >> 22) & 0x3FF) * sizeof(uint32_t);
    }
    else{
        return (((address) >> 21) & 0x1FF) * sizeof(uint64_t);
    }
}

uint32_t pdba_base_nopae (uint32_t pdpe){
    return pdpe & 0xFFFFF000;
}

uint64_t pdba_base_pae (uint64_t pdpe){
    return pdpe & 0xFFFFFF000ULL;
}

uint32_t get_pgd_nopae (
        xa_instance_t *instance, uint32_t vaddr, uint32_t pdpe, int k)
{
    uint32_t value;
    uint32_t pgd_entry = pdba_base_nopae(pdpe) + pgd_index(instance, vaddr);
    xa_dbprint("--PTLookup: pgd_entry = 0x%.8x\n", pgd_entry);
    if (k){
        xa_read_long_phys(instance, pgd_entry-instance->page_offset, &value);
    }
    else{
        xa_read_long_virt(instance, pgd_entry, 0, &value);
    }
    return value;
}

uint64_t get_pgd_pae (
        xa_instance_t *instance, uint32_t vaddr, uint64_t pdpe, int k)
{
    uint64_t value;
    uint32_t pgd_entry = pdba_base_pae(pdpe) + pgd_index(instance, vaddr);
    xa_dbprint("--PTLookup: pgd_entry = 0x%.8x\n", pgd_entry);
    xa_read_long_long_phys(instance, pgd_entry, &value);
    return value;
}

/* page table */
uint32_t pte_index (xa_instance_t *instance, uint32_t address){
    if (!instance->pae){
        return (((address) >> 12) & 0x3FF) * sizeof(uint32_t);
    }
    else{
        return (((address) >> 12) & 0x1FF) * sizeof(uint64_t); 
    }
}
        
uint32_t ptba_base_nopae (uint32_t pde){
    return pde & 0xFFFFF000;
}

uint64_t ptba_base_pae (uint64_t pde){
    return pde & 0xFFFFFF000ULL;
}

uint32_t get_pte_nopae (xa_instance_t *instance, uint32_t vaddr, uint32_t pgd){
    uint32_t value;
    uint32_t pte_entry = ptba_base_nopae(pgd) + pte_index(instance, vaddr);
    xa_dbprint("--PTLookup: pte_entry = 0x%.8x\n", pte_entry);
    xa_read_long_mach(instance, pte_entry, &value);
    return value;
}

uint64_t get_pte_pae (xa_instance_t *instance, uint32_t vaddr, uint64_t pgd){
    uint64_t value;
    uint32_t pte_entry = ptba_base_pae(pgd) + pte_index(instance, vaddr);
    xa_dbprint("--PTLookup: pte_entry = 0x%.8x\n", pte_entry);
    xa_read_long_long_mach(instance, pte_entry, &value);
    return value;
}

/* page */
uint32_t pte_pfn_nopae (uint32_t pte){
    return pte & 0xFFFFF000;
}

uint64_t pte_pfn_pae (uint64_t pte){
    return pte & 0xFFFFFF000ULL;
}

uint32_t get_paddr_nopae (uint32_t vaddr, uint32_t pte){
    return pte_pfn_nopae(pte) | (vaddr & 0xFFF);
}

uint64_t get_paddr_pae (uint32_t vaddr, uint64_t pte){
    return pte_pfn_pae(pte) | (vaddr & 0xFFF);
}

uint32_t get_large_paddr (
        xa_instance_t *instance, uint32_t vaddr, uint32_t pgd_entry)
{
    if (!instance->pae){
        return (pgd_entry & 0xFFC00000) | (vaddr & 0x3FFFFF);
    }
    else{
        return (pgd_entry & 0xFFE00000) | (vaddr & 0x1FFFFF);
    }
}

/* "buffalo" routines
 * see "Using Every Part of the Buffalo in Windows Memory Analysis" by
 * Jesse D. Kornblum for details. 
 * for now, just test the bits and print out details */
int get_transition_bit(uint32_t entry)
{
    return xa_get_bit(entry, 11);
}

int get_prototype_bit(uint32_t entry)
{
    return xa_get_bit(entry, 10);
}

void buffalo_nopae (xa_instance_t *instance, uint32_t entry, int pde)
{
    /* similar techniques are surely doable in linux, but for now
     * this is only testing for windows domains */
    if (!instance->os_type == XA_OS_WINDOWS){
        return;
    }

    if (!get_transition_bit(entry) && !get_prototype_bit(entry)){
        uint32_t pfnum = (entry >> 1) & 0xF;
        uint32_t pfframe = entry & 0xFFFFF000;

        /* pagefile */
        if (pfnum != 0 && pfframe != 0){
            xa_dbprint("--Buffalo: page file = %d, frame = 0x%.8x\n",
                pfnum, pfframe);
        }
        /* demand zero */
        else if (pfnum == 0 && pfframe == 0){
            xa_dbprint("--Buffalo: demand zero page\n");
        }
    }

    else if (get_transition_bit(entry) && !get_prototype_bit(entry)){
        /* transition */
        xa_dbprint("--Buffalo: page in transition\n");
    }

    else if (!pde && get_prototype_bit(entry)){
        /* prototype */
        xa_dbprint("--Buffalo: prototype entry\n");
    }

    else if (entry == 0){
        /* zero */
        xa_dbprint("--Buffalo: entry is zero\n");
    }

    else{
        /* zero */
        xa_dbprint("--Buffalo: unknown\n");
    }
}

/* translation */
uint32_t v2p_nopae(xa_instance_t *instance, uint32_t cr3, uint32_t vaddr, int k)
{
    uint32_t paddr = 0;
    uint32_t pgd, pte;
        
    xa_dbprint("--PTLookup: lookup vaddr = 0x%.8x\n", vaddr);
    xa_dbprint("--PTLookup: cr3 = 0x%.8x\n", cr3);
    pgd = get_pgd_nopae(instance, vaddr, cr3, k);
    xa_dbprint("--PTLookup: pgd = 0x%.8x\n", pgd);
        
    if (entry_present(pgd)){
        if (page_size_flag(pgd)){
            paddr = get_large_paddr(instance, vaddr, pgd);
            xa_dbprint("--PTLookup: 4MB page\n", pgd);
        }
        else{
            pte = get_pte_nopae(instance, vaddr, pgd);
            xa_dbprint("--PTLookup: pte = 0x%.8x\n", pte);
            if (entry_present(pte)){
                paddr = get_paddr_nopae(vaddr, pte);
            }
            else{
                buffalo_nopae(instance, pte, 1);
            }
        }
    }
    else{
        buffalo_nopae(instance, pgd, 0);
    }
    xa_dbprint("--PTLookup: paddr = 0x%.8x\n", paddr);
    return paddr;
}

uint32_t v2p_pae (xa_instance_t *instance, uint32_t cr3, uint32_t vaddr, int k)
{
    uint32_t paddr = 0;
    uint64_t pdpe, pgd, pte;
        
    xa_dbprint("--PTLookup: lookup vaddr = 0x%.8x\n", vaddr);
    xa_dbprint("--PTLookup: cr3 = 0x%.8x\n", cr3);
    pdpe = get_pdpi(instance, vaddr, cr3, k);
    xa_dbprint("--PTLookup: pdpe = 0x%.16x\n", pdpe);
    if (!entry_present(pdpe)){
        return paddr;
    }
    pgd = get_pgd_pae(instance, vaddr, pdpe, k);
    xa_dbprint("--PTLookup: pgd = 0x%.16x\n", pgd);

    if (entry_present(pgd)){
        if (page_size_flag(pgd)){
            paddr = get_large_paddr(instance, vaddr, pgd);
            xa_dbprint("--PTLookup: 2MB page\n", pgd);
        }
        else{
            pte = get_pte_pae(instance, vaddr, pgd);
            xa_dbprint("--PTLookup: pte = 0x%.16x\n", pte);
            if (entry_present(pte)){
                paddr = get_paddr_pae(vaddr, pte);
            }
        }
    }
    xa_dbprint("--PTLookup: paddr = 0x%.8x\n", paddr);
    return paddr;
}

/* convert address to machine address via page tables */
uint32_t xa_pagetable_lookup (
            xa_instance_t *instance,
            uint32_t cr3,
            uint32_t vaddr,
            int kernel)
{
    if (instance->pae){
        return v2p_pae(instance, cr3, vaddr, kernel);
    }
    else{
        return v2p_nopae(instance, cr3, vaddr, kernel);
    }
}

/* expose virtual to physical mapping via api call */
uint32_t xa_translate_kv2p(xa_instance_t *instance, uint32_t virt_address)
{
    return xa_pagetable_lookup(instance, instance->kpgd, virt_address, 1);
}

/* map memory given a kernel symbol */
void *xa_access_kernel_symbol (
        xa_instance_t *instance, char *symbol, uint32_t *offset)
{
    if (XA_OS_LINUX == instance->os_type){
        return linux_access_kernel_symbol(instance, symbol, offset);
    }
    else if (XA_OS_WINDOWS == instance->os_type){
        return windows_access_kernel_symbol(instance, symbol, offset);
    }
    else{
        return NULL;
    }
}

/* finds the address of the page global directory for a given pid */
uint32_t xa_pid_to_pgd (xa_instance_t *instance, int pid)
{
    if (XA_OS_LINUX == instance->os_type){
        return linux_pid_to_pgd(instance, pid);
    }
    else if (XA_OS_WINDOWS == instance->os_type){
        return windows_pid_to_pgd(instance, pid);
    }
    else{
        return 0;
    }
}

void *xa_access_user_virtual_address (
        xa_instance_t *instance,
        uint32_t virt_address,
        uint32_t *offset,
        int pid)
{
    uint32_t address = 0;

    /* check the LRU cache */
    if (xa_check_cache_virt(instance, virt_address, pid, &address)){
        return xa_access_machine_address(instance, address, offset);
    }

    /* use kernel page tables */
    /*TODO HYPERVISOR_VIRT_START = 0xFC000000 so we can't go over that.
      Figure out what this should be b/c there still may be a fixed
      mapping range between the page'd addresses and VIRT_START */
    if (!pid){
        address = xa_pagetable_lookup(
                            instance, instance->kpgd, virt_address, 1);
        if (!address){
            printf("ERROR: address not in page table (0x%x)\n", virt_address);
            return NULL;
        }
        xa_update_cache(instance, NULL, virt_address, pid, address);
        return xa_access_machine_address(instance, address, offset);
    }

    /* use user page tables */
    else{
        uint32_t pgd = xa_pid_to_pgd(instance, pid);
        xa_dbprint("--UserVirt: pgd for pid=%d is 0x%.8x.\n", pid, pgd);

        if (pgd){
            address = xa_pagetable_lookup(instance, pgd, virt_address, 0);
        }

        if (!address){
            printf("ERROR: address not in page table (0x%x)\n", virt_address);
            return NULL;
        }
        xa_update_cache(instance, NULL, virt_address, pid, address);
        return xa_access_machine_address_rw(
            instance, address, offset, PROT_READ | PROT_WRITE);
    }
}

void *xa_access_user_virtual_range (
        xa_instance_t *instance,
        uint32_t virt_address,
		uint32_t size,
        uint32_t *offset,
        int pid)
{
	int i;

	uint32_t num_pages = size/instance->page_size + 1;

	uint32_t pgd = pid ? xa_pid_to_pgd(instance, pid) : instance->kpgd;
	xen_pfn_t* pfns = (xen_pfn_t*)malloc(sizeof(xen_pfn_t)*num_pages);
	
	uint32_t start = virt_address & ~(instance->page_size-1);
	for(i=0; i < num_pages; i++) {
		// Virtual address for each page we will map
		uint32_t addr = start + i*instance->page_size;
	
		if(!addr) {
			printf("ERROR: address not in page table (%p)\n", addr);
			return NULL;
		}

		// Physical page frame number of each page
		pfns[i] = xa_pagetable_lookup(
					instance, pgd, addr, pid ? 0 : 1) >> instance->page_shift;
	}

	*offset = virt_address - start;

	return xc_map_foreign_pages(
				instance->xc_handle, instance->domain_id, PROT_READ, pfns, num_pages);
}

void *xa_access_virtual_address (
        xa_instance_t *instance, uint32_t virt_address, uint32_t *offset)
{
    return xa_access_user_virtual_address(instance, virt_address, offset, 0);
}

void *xa_access_virtual_range (
	xa_instance_t *instance,
	uint32_t virt_address,
	uint32_t size,
	uint32_t* offset)
{
	return xa_access_user_virtual_range(instance, virt_address, size, offset, 0);
}

void *xa_access_physical_address (
        xa_instance_t *instance, uint32_t phys_address, uint32_t *offset)
{
    unsigned long pfn;
    int i;
    
    /* page frame number = physical address >> PAGE_SHIFT */
    pfn = phys_address >> instance->page_shift;
    
    /* get the offset */
    *offset = (instance->page_size-1) & phys_address;
    
    /* access the memory */
    return xa_mmap_pfn(instance, PROT_READ, pfn);
}

void *xa_access_machine_address (
        xa_instance_t *instance, uint32_t mach_address, uint32_t *offset)
{
    return xa_access_machine_address_rw(
                instance, mach_address, offset, PROT_READ);
}

void *xa_access_machine_address_rw (
        xa_instance_t *instance, uint32_t mach_address,
        uint32_t *offset, int prot)
{
    unsigned long mfn;
    int i;

    /* machine frame number = machine address >> PAGE_SHIFT */
    mfn = mach_address >> instance->page_shift;

    /* get the offset */
    *offset = (instance->page_size-1) & mach_address;

    /* access the memory */
    return xa_mmap_mfn(instance, prot, mfn);
}

