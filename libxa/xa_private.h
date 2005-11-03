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
 * This file contains function headers for items that are not exported
 * outside of the library for public use.  These functions are intended
 * to only be used inside the library.
 *
 * File: xa_private.h
 *
 * Author(s): Bryan D. Payne (bryan@thepaynes.cc)
 *
 * $Id$
 * $Date$
 */
#ifndef XA_PRIVATE_H
#define XA_PRIVATE_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <xenctrl.h>

/**
 * Prints out the hex and ascii version of a chunk of bytes. The
 * output is similar to what you would get with 'od -h' with the
 * additional ascii information on the right side of the display.
 *
 * @param[in] data The bytes that will be printed to stdout
 * @param[in] length The length (in bytes) of data
 */
void print_hex (unsigned char *data, int length);

/**
 * Prints out the data in a xc_dominfo_t struct to stdout.
 *
 * @param[in] info The struct to print
 */
void print_dominfo (xc_dominfo_t info);

#endif /* XA_PRIVATE_H */
