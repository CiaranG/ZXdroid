/* fuller.h: Routines for handling the Fuller Box
   Copyright (c) 2007-2009 Stuart Brady, Fredrick Meunier

   $Id: fuller.h 4030 2009-06-07 14:38:38Z fredm $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   Philip: philip-fuse@shadowmagic.org.uk

   Stuart: sdbrady@ntlworld.com

*/

#ifndef FUSE_FULLER_H
#define FUSE_FULLER_H

#include <libspectrum.h>

#include "periph.h"

extern const periph_t fuller_peripherals[];
extern const size_t fuller_peripherals_count;

int fuller_init( void );

#endif				/* #ifndef FUSE_FULLER_H */
