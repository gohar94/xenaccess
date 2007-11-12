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
#define _GNU_SOURCE
#include <string.h>
#include <time.h>
#include "xa_private.h"

#define MAX_SYM_LEN 512

int xa_check_cache_sym (xa_instance_t *instance,
                        char *symbol_name,
                        int pid,
                        uint32_t *mach_address)
{
    xa_cache_entry_t current;
    int ret = 0;

    current = instance->cache_head;
    while (current != NULL){
        if ((strncmp(current->symbol_name, symbol_name, MAX_SYM_LEN) == 0) &&
            (current->pid == pid) && (current->mach_address)){
            current->last_used = time(NULL);
            *mach_address = current->mach_address;
            ret = 1;
            xa_dbprint("++Cache hit (%s --> 0x%.8x)\n",
                symbol_name, *mach_address);
            goto exit;
        }
        current = current->next;
    }

exit:
    return ret;
}

int xa_check_cache_virt (xa_instance_t *instance,
                         uint32_t virt_address,
                         int pid,
                         uint32_t *mach_address)
{
    xa_cache_entry_t current;
    int ret = 0;
    uint32_t lookup = virt_address & ~(instance->page_size - 1);

    current = instance->cache_head;
    while (current != NULL){
        if ((current->virt_address == lookup) &&
            (current->pid == pid) &&
            (current->mach_address)){
            current->last_used = time(NULL);
            *mach_address = (current->mach_address |
                (virt_address & (instance->page_size - 1)));
            ret = 1;
            xa_dbprint("++Cache hit (0x%.8x --> 0x%.8x, 0x%.8x)\n",
                virt_address, *mach_address, current->mach_address);
            goto exit;
        }
        current = current->next;
    }

exit:
    return ret;
}

int xa_update_cache (xa_instance_t *instance,
                     char *symbol_name,
                     uint32_t virt_address,
                     int pid,
                     uint32_t mach_address)
{
    xa_cache_entry_t new_entry = NULL;
    uint32_t vlookup = virt_address & ~(instance->page_size - 1);
    uint32_t mlookup = mach_address & ~(instance->page_size - 1);

    /* is cache enabled? */
    if (XA_CACHE_SIZE == 0){
        return 1;
    }

    /* does anything match the passed symbol_name? */
    /* if so, update other entries */
    if (symbol_name){
        xa_cache_entry_t current = instance->cache_head;
        while (current != NULL){
            if (strncmp(current->symbol_name, symbol_name, MAX_SYM_LEN) == 0){
                current->last_used = time(NULL);
                current->virt_address = 0;
                current->pid = pid;
                if (mach_address){
                    current->mach_address = mach_address;
                }
                else{
                    current->mach_address =
                        xa_translate_kv2p(instance, virt_address);
                }
                xa_dbprint("++Cache set (%s --> 0x%.8x)\n",
                    symbol_name, current->mach_address);
                goto exit;
            }
            current = current->next;
        }
    }

    /* does anything match the passed virt_address? */
    /* if so, update other entries */
    if (virt_address){
        xa_cache_entry_t current = instance->cache_head;
        while (current != NULL){
            if (current->virt_address == vlookup){
                current->last_used = time(NULL);
                current->pid = pid;
                current->mach_address = mlookup;
                xa_dbprint("++Cache set (0x%.8x --> 0x%.8x)\n",
                    vlookup, mlookup);
                goto exit;
            }
            current = current->next;
        }
    }

    /* do we need to remove anything from the cache? */
    if (instance->current_cache_size >= XA_CACHE_SIZE){
        xa_cache_entry_t oldest = instance->cache_head;
        xa_cache_entry_t current = instance->cache_head;

        /* find the least recently used entry */
        while (current != NULL){
            if (current->last_used < oldest->last_used){
                oldest = current;
            }
            current = current->next;
        }

        /* remove that entry */
        if (NULL == oldest->next && NULL == oldest->prev){  /* only entry */
            instance->cache_head = NULL;
            instance->cache_tail = NULL;
        }
        else if (NULL == oldest->next){  /* last entry */
            instance->cache_tail = oldest->prev;
            oldest->prev->next = NULL;
        }
        else if (NULL == oldest->prev){  /* first entry */
            instance->cache_head = oldest->next;
            oldest->next->prev = NULL;
        }
        else{  /* somewhere in the middle */
            oldest->prev->next = oldest->next;
            oldest->next->prev = oldest->prev;
        }

        /* free up memory */
        if (oldest->symbol_name){
            free(oldest->symbol_name);
        }
        oldest->next = NULL;
        oldest->prev = NULL;
        free(oldest);

        instance->current_cache_size--;
    }

    /* allocate memory for the new cache entry */
    new_entry = (xa_cache_entry_t) malloc(sizeof(struct xa_cache_entry));
    new_entry->last_used = time(NULL);
    if (symbol_name){
        new_entry->symbol_name = strndup(symbol_name, MAX_SYM_LEN);
        new_entry->virt_address = 0;
        if (mach_address){
            new_entry->mach_address = mach_address;
        }
        else{
            new_entry->mach_address =
                xa_translate_kv2p(instance, virt_address);
        }
        xa_dbprint("++Cache set (%s --> 0x%.8x)\n",
            symbol_name, new_entry->mach_address);
    }
    else{
        new_entry->symbol_name = strndup("", MAX_SYM_LEN);
        new_entry->virt_address = vlookup;
        new_entry->mach_address = mlookup;
        xa_dbprint("++Cache set (0x%.8x --> 0x%.8x)\n", vlookup, mlookup);
    }
    new_entry->pid = pid;

    /* add it to the end of the list */
    if (NULL != instance->cache_tail){
        instance->cache_tail->next = new_entry;
    }
    new_entry->prev = instance->cache_tail;
    instance->cache_tail = new_entry;
    if (NULL == instance->cache_head){
        instance->cache_head = new_entry;
    }
    new_entry->next = NULL;
    instance->current_cache_size++;

exit:
    return 1;
}

int xa_destroy_cache (xa_instance_t *instance)
{
    xa_cache_entry_t current = instance->cache_head;
    xa_cache_entry_t tmp = NULL;
    while (current != NULL){
        tmp = current->next;
        free(current);
        current = tmp;
    }

    instance->cache_head = NULL;
    instance->cache_tail = NULL;
    instance->current_cache_size = 0;
    return 0;
}
