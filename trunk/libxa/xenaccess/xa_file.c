/*
 * The libxa library provides access to resources in domU machines.
 * 
 * Copyright (C) 2005 - 2008  Bryan D. Payne (bryan@thepaynes.cc)
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
 * This file contains utility functions for access data from a file
 * that contains the memory state from a running machine.
 *
 * File: xa_file.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 */

#include <stdio.h>

void *xa_map_file_range (xa_instance_t *instance, int prot, unsigned long pfn)
{
    void *memory = NULL;
    FILE *f = instance->mode.file.fhandle;
    long address = pfn << instance->page_shift;

/*
    if (fseek(f, address, SEEK_SET) != 0){
        return NULL;
    }
    if ((memory = malloc(instance->page_size)) == NULL){
        return NULL;
    }
    if (fread(memory, instance->page_size, 1, f) < instance->page_size){
        free(memory);
        return NULL;
    }
    return memory;
*/
    if ((memory = mmap(0, instance->page_size, prot, MAP_FILE, fildes, address) == -1){
        return NULL;
    }
    return memory;
}
