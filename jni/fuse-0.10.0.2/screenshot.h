/* screenshot.h: Routines for saving .png screenshots
   Copyright (c) 2002 Philip Kendall

   $Id: screenshot.h 2889 2007-05-26 17:45:08Z zubzero $

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_SCREENSHOT_H
#define FUSE_SCREENSHOT_H

#ifndef SCALER_H
#include "ui/scaler/scaler.h"
#endif				/* #ifndef SCALER_H */

#ifdef USE_LIBPNG

int screenshot_write( const char *filename, scaler_type scaler );
int screenshot_write_fast( const char *filename, scaler_type scaler );
int screenshot_available_scalers( scaler_type scaler );

#endif				/* #ifdef USE_LIBPNG */

int screenshot_scr_write( const char *filename );
int screenshot_scr_read( const char *filename );

extern char screenshot_movie_name[];
extern char screenshot_movie_file[];
extern long int screenshot_movie_frame;
extern int screenshot_movie_record;
extern scaler_type screenshot_movie_scaler;

#define SCREENSHOT_MOVIE_FILE_MAX 256

#endif				/* #ifndef FUSE_SCREENSHOT_H */
