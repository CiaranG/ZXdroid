## Process this file with automake to produce Makefile.in
## Copyright (c) 2003-2007 Philip Kendall

## $Id: Makefile.am 3283 2007-11-09 12:21:06Z fredm $

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##
## Author contact information:
##
## E-mail: philip-fuse@shadowmagic.org.uk

AM_CPPFLAGS += -I$(srcdir)/compat

DIST_SUBDIRS += compat/amiga \
	       compat/morphos \
	       compat/unix \
	       compat/win32

EXTRA_fuse_SOURCES += compat/dirname.c \
		compat/getopt.c \
		compat/getopt1.c \
		compat/mkstemp.c

noinst_HEADERS += compat/getopt.h
