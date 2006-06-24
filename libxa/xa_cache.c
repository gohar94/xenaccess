/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005 - 2006  Bryan D. Payne (bryan@thepaynes.cc)
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
 * This file contains an implementation of a LRU cache for the
 * memory addresses.  The idea is to avoid page table lookups
 * whenever possible since that is an expensive operation.
 *
 * File: xa_cache.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */
#include <time.h>
#include "xa_private.h"

xa_cache_entry_t cache_head = NULL;
xa_cache_entry_t cache_tail = NULL;
int current_cache_size = 0;

int xa_check_cache (uint32_t virt_address, int pid, uint32_t *mach_address)
{
    xa_cache_entry_t current;
    int ret = 0;

    current = cache_head;
    while (current != NULL){
        if ((current->virt_address == virt_address) && (current->pid == pid)){
            current->last_used = time(NULL);
            *mach_address = current->mach_address;
            ret = 1;
            goto exit;
        }
        current = current->next;
    }

exit:
    return ret;
}

int xa_add_cache (uint32_t virt_address, int pid, uint32_t mach_address)
{
    xa_cache_entry_t new_entry = NULL;

    /* do we need to remove anything from the cache? */
    if (current_cache_size >= XA_CACHE_SIZE){
        xa_cache_entry_t oldest = cache_head;
        xa_cache_entry_t current = cache_head;

        /* find the least recently used entry */
        while (current != NULL){
            if (current->last_used < oldest->last_used){
                oldest = current;
            }
            current = current->next;
        }

        /* remove that entry */
        if (NULL == oldest->next && NULL == oldest->prev){  /* only entry */
            cache_head = NULL;
            cache_tail = NULL;
        }
        else if (NULL == oldest->next){  /* last entry */
            cache_tail = oldest->prev;
            oldest->prev->next = NULL;
        }
        else if (NULL == oldest->prev){  /* first entry */
            cache_head = oldest->next;
            oldest->next->prev = NULL;
        }
        else{  /* somewhere in the middle */
            oldest->prev->next = oldest->next;
            oldest->next->prev = oldest->prev;
        }

        /* free up memory */
        oldest->next = NULL;
        oldest->prev = NULL;
        free(oldest);

        current_cache_size--;
    }

    /* allocate memory for the new cache entry */
    new_entry = (xa_cache_entry_t) malloc(sizeof(struct xa_cache_entry));
    new_entry->last_used = time(NULL);
    new_entry->virt_address = virt_address;
    new_entry->mach_address = mach_address;
    new_entry->pid = pid;

    /* add it to the end of the list */
    if (NULL != cache_tail){
        cache_tail->next = new_entry;
    }
    new_entry->prev = cache_tail;
    cache_tail = new_entry;
    if (NULL == cache_head){
        cache_head = new_entry;
    }
    new_entry->next = NULL;
    current_cache_size++;

    return 1;
}

