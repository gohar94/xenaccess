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
 * This file contains functions that are generally useful for use
 * throughout the rest of the library.
 *
 * File: xa_util.c
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id: xa_template 36 2006-11-30 01:38:20Z bdpayne $
 * $Date: 2006-11-29 20:38:20 -0500 (Wed, 29 Nov 2006) $
 */

#include <stdio.h>
#include <stdarg.h>
#include "xenaccess.h"
#include "xa_private.h"

int xa_get_bit (unsigned long reg, int bit)
{
    unsigned long mask = 1 << bit;
    if (reg & mask){
        return 1;
    }
    else{
        return 0;
    }
}

#ifndef XA_DEBUG
/* Nothing */
#else
void xa_dbprint(char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}
#endif
