/* plusd.c: Routines for handling the +D interface
   Copyright (c) 1999-2007 Stuart Brady, Fredrick Meunier, Philip Kendall,
   Dmitry Sanarin, Darren Salt

   $Id: plusd.c 3942 2009-01-10 14:18:46Z pak21 $

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

#include <config.h>

#include <libspectrum.h>

#include <string.h>

#include "compat.h"
#include "machine.h"
#include "module.h"
#include "plusd.h"
#include "printer.h"
#include "settings.h"
#include "ui/ui.h"
#include "wd_fdc.h"

int plusd_available = 0;
int plusd_active = 0;

static int plusd_index_pulse;

static int index_event;

#define PLUSD_NUM_DRIVES 2

static wd_fdc *plusd_fdc;
static wd_fdc_drive plusd_drives[ PLUSD_NUM_DRIVES ];

static libspectrum_byte plusd_ram[ 0x2000 ];

static void plusd_reset( int hard_reset );
static void plusd_memory_map( void );
static void plusd_enabled_snapshot( libspectrum_snap *snap );
static void plusd_from_snapshot( libspectrum_snap *snap );
static void plusd_to_snapshot( libspectrum_snap *snap );
static void plusd_event_index( libspectrum_dword last_tstates, int type,
			       void *user_data );

static module_info_t plusd_module_info = {

  plusd_reset,
  plusd_memory_map,
  plusd_enabled_snapshot,
  plusd_from_snapshot,
  plusd_to_snapshot,

};

static libspectrum_byte plusd_control_register;

void
plusd_page( void )
{
  plusd_active = 1;
  machine_current->ram.romcs = 1;
  machine_current->memory_map();
}

void
plusd_unpage( void )
{
  plusd_active = 0;
  machine_current->ram.romcs = 0;
  machine_current->memory_map();
}

static void
plusd_memory_map( void )
{
  if( !plusd_active ) return;

  memory_map_read[ 0 ] = memory_map_write[ 0 ] = memory_map_romcs[ 0 ];
  memory_map_read[ 1 ] = memory_map_write[ 1 ] = memory_map_romcs[ 1 ];
}

const periph_t plusd_peripherals[] = {
  /* ---- ---- 1110 0011 */
  { 0x00ff, 0x00e3, plusd_sr_read, plusd_cr_write },
  /* ---- ---- 1110 1011 */
  { 0x00ff, 0x00eb, plusd_tr_read, plusd_tr_write },
  /* ---- ---- 1111 0011 */
  { 0x00ff, 0x00f3, plusd_sec_read, plusd_sec_write },
  /* ---- ---- 1111 1011 */
  { 0x00ff, 0x00fb, plusd_dr_read, plusd_dr_write },

  /* ---- ---- 1110 1111 */
  { 0x00ff, 0x00ef, NULL, plusd_cn_write },
  /* ---- ---- 1110 0111 */
  { 0x00ff, 0x00e7, plusd_mem_read, plusd_mem_write },
  /* 0000 0000 1111 0111 */
  { 0x00ff, 0x00f7, plusd_printer_read, plusd_printer_write },
};

const size_t plusd_peripherals_count =
  sizeof( plusd_peripherals ) / sizeof( periph_t );

int
plusd_init( void )
{
  int i;
  wd_fdc_drive *d;

  plusd_fdc = wd_fdc_alloc_fdc( WD1770, 0, WD_FLAG_NONE );

  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    d = &plusd_drives[ i ];
    fdd_init( &d->fdd, FDD_SHUGART, 0, 0 );	/* drive geometry 'autodetect' */
    d->disk.flag = DISK_FLAG_NONE;
  }

  plusd_fdc->current_drive = &plusd_drives[ 0 ];
  fdd_select( &plusd_drives[ 0 ].fdd, 1 );
  plusd_fdc->dden = 1;
  plusd_fdc->set_intrq = NULL;
  plusd_fdc->reset_intrq = NULL;
  plusd_fdc->set_datarq = NULL;
  plusd_fdc->reset_datarq = NULL;
  plusd_fdc->iface = NULL;

  index_event = event_register( plusd_event_index, "+D index" );

  module_register( &plusd_module_info );

  return 0;
}

static void
plusd_reset( int hard_reset )
{
  int i;
  wd_fdc_drive *d;

  plusd_active = 0;
  plusd_available = 0;

  event_remove_type( index_event );

  if( !periph_plusd_active )
    return;

  machine_load_rom_bank( memory_map_romcs, 0, 0,
			 settings_current.rom_plusd,
			 settings_default.rom_plusd, 0x2000 );

  memory_map_romcs[0].source = MEMORY_SOURCE_PERIPHERAL;

  memory_map_romcs[1].page = plusd_ram;
  memory_map_romcs[1].source = MEMORY_SOURCE_PERIPHERAL;

  machine_current->ram.romcs = 0;

  memory_map_romcs[ 0 ].writable = 0;
  memory_map_romcs[ 1 ].writable = 1;

  plusd_available = 1;
  plusd_active = 1;
  plusd_index_pulse = 0;

  if( hard_reset )
    memset( plusd_ram, 0, 0x2000 );

  wd_fdc_master_reset( plusd_fdc );

  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    d = &plusd_drives[ i ];

    d->index_pulse = 0;
    d->index_interrupt = 0;
  }

  /* We can eject disks only if they are currently present */
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_EJECT,
		    plusd_drives[ PLUSD_DRIVE_1 ].fdd.loaded );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_WP_SET,
		    !plusd_drives[ PLUSD_DRIVE_1 ].fdd.wrprot );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_EJECT,
		    plusd_drives[ PLUSD_DRIVE_2 ].fdd.loaded );
  ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_WP_SET,
		    !plusd_drives[ PLUSD_DRIVE_2 ].fdd.wrprot );

  plusd_fdc->current_drive = &plusd_drives[ 0 ];
  fdd_select( &plusd_drives[ 0 ].fdd, 1 );
  machine_current->memory_map();
  plusd_event_index( 0, index_event, NULL );

  ui_statusbar_update( UI_STATUSBAR_ITEM_DISK, UI_STATUSBAR_STATE_INACTIVE );
}

void
plusd_end( void )
{
  plusd_available = 0;
}

libspectrum_byte
plusd_sr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !plusd_available ) return 0xff;

  *attached = 1;
  return wd_fdc_sr_read( plusd_fdc );
}

void
plusd_cr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !plusd_available ) return;

  wd_fdc_cr_write( plusd_fdc, b );
}

libspectrum_byte
plusd_tr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !plusd_available ) return 0xff;

  *attached = 1;
  return wd_fdc_tr_read( plusd_fdc );
}

void
plusd_tr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !plusd_available ) return;

  wd_fdc_tr_write( plusd_fdc, b );
}

libspectrum_byte
plusd_sec_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !plusd_available ) return 0xff;

  *attached = 1;
  return wd_fdc_sec_read( plusd_fdc );
}

void
plusd_sec_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !plusd_available ) return;

  wd_fdc_sec_write( plusd_fdc, b );
}

libspectrum_byte
plusd_dr_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !plusd_available ) return 0xff;

  *attached = 1;
  return wd_fdc_dr_read( plusd_fdc );
}

void
plusd_dr_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  if( !plusd_available ) return;

  wd_fdc_dr_write( plusd_fdc, b );
}

void
plusd_cn_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b )
{
  int drive, side;
  int i;

  if( !plusd_available ) return;

  plusd_control_register = b;

  drive = ( b & 0x03 ) == 2 ? 1 : 0;
  side = ( b & 0x80 ) ? 1 : 0;

  /* TODO: set current_drive to NULL when bits 0 and 1 of b are '00' or '11' */
  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    fdd_set_head( &plusd_drives[ i ].fdd, side );
  }
  fdd_select( &plusd_drives[ (!drive) ].fdd, 0 );
  fdd_select( &plusd_drives[ drive ].fdd, 1 );

  if( plusd_fdc->current_drive != &plusd_drives[ drive ] ) {
    if( plusd_fdc->current_drive->fdd.motoron ) {            /* swap motoron */
      fdd_motoron( &plusd_drives[ (!drive) ].fdd, 0 );
      fdd_motoron( &plusd_drives[ drive ].fdd, 1 );
    }
    plusd_fdc->current_drive = &plusd_drives[ drive ];
  }

  printer_parallel_strobe_write( b & 0x40 );
}

libspectrum_byte
plusd_mem_read( libspectrum_word port GCC_UNUSED, int *attached GCC_UNUSED )
{
  if( !plusd_available ) return 0xff;

  /* should we set *attached = 1? */

  plusd_page();
  return 0;
}

void
plusd_mem_write( libspectrum_word port GCC_UNUSED, libspectrum_byte b GCC_UNUSED )
{
  if( !plusd_available ) return;

  plusd_unpage();
}

libspectrum_byte
plusd_printer_read( libspectrum_word port GCC_UNUSED, int *attached )
{
  if( !plusd_available ) return 0xff;

  *attached = 1;

  /* bit 7 = busy. other bits high? */

  if(!settings_current.printer)
    return(0xff); /* no printer attached */

  return(0x7f);   /* never busy */
}

void
plusd_printer_write( libspectrum_word port, libspectrum_byte b )
{
  if( !plusd_available ) return;

  printer_parallel_write( port, b );
}

int
plusd_disk_insert( plusd_drive_number which, const char *filename,
		   int autoload )
{
  int error;
  wd_fdc_drive *d;

  if( which >= PLUSD_NUM_DRIVES ) {
    ui_error( UI_ERROR_ERROR, "plusd_disk_insert: unknown drive %d",
	      which );
    fuse_abort();
  }

  d = &plusd_drives[ which ];

  /* Eject any disk already in the drive */
  if( d->fdd.loaded ) {
    /* Abort the insert if we want to keep the current disk */
    if( plusd_disk_eject( which, 0 ) ) return 0;
  }

  if( filename ) {
    error = disk_open( &d->disk, filename, 0 );
    if( error != DISK_OK ) {
      ui_error( UI_ERROR_ERROR, "Failed to open disk image: %s",
				disk_strerror( error ) );
      return 1;
    }
  } else {
    error = disk_new( &d->disk, 2, 80, DISK_DENS_AUTO, DISK_UDI );
    if( error != DISK_OK ) {
      ui_error( UI_ERROR_ERROR, "Failed to create disk image: %s",
				disk_strerror( error ) );
      return 1;
    }
  }

  fdd_load( &d->fdd, &d->disk, 0 );

  /* Set the 'eject' item active */
  switch( which ) {
  case PLUSD_DRIVE_1:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_EJECT, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_WP_SET,
		      !plusd_drives[ PLUSD_DRIVE_1 ].fdd.wrprot );
    break;
  case PLUSD_DRIVE_2:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_EJECT, 1 );
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_WP_SET,
		      !plusd_drives[ PLUSD_DRIVE_2 ].fdd.wrprot );
    break;
  }

  if( filename && autoload ) {
    /* XXX */
  }

  return 0;
}

int
plusd_disk_eject( plusd_drive_number which, int write )
{
  wd_fdc_drive *d;

  if( which >= PLUSD_NUM_DRIVES )
    return 1;

  d = &plusd_drives[ which ];

  if( d->disk.type == DISK_TYPE_NONE )
    return 0;

  if( write ) {

    if( ui_plusd_disk_write( which ) ) return 1;

  } else {

    if( d->disk.dirty ) {

      ui_confirm_save_t confirm = ui_confirm_save(
	"Disk in +D drive %c has been modified.\n"
	"Do you want to save it?",
	which == PLUSD_DRIVE_1 ? '1' : '2'
      );

      switch( confirm ) {

      case UI_CONFIRM_SAVE_SAVE:
	if( ui_plusd_disk_write( which ) ) return 1;
	break;

      case UI_CONFIRM_SAVE_DONTSAVE: break;
      case UI_CONFIRM_SAVE_CANCEL: return 1;

      }
    }
  }

  fdd_unload( &d->fdd );
  disk_close( &d->disk );

  /* Set the 'eject' item inactive */
  switch( which ) {
  case PLUSD_DRIVE_1:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_EJECT, 0 );
    break;
  case PLUSD_DRIVE_2:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_EJECT, 0 );
    break;
  }
  return 0;
}

int
plusd_disk_writeprotect( plusd_drive_number which, int wrprot )
{
  wd_fdc_drive *d;

  if( which >= PLUSD_NUM_DRIVES )
    return 1;

  d = &plusd_drives[ which ];

  if( !d->fdd.loaded )
    return 1;

  fdd_wrprot( &d->fdd, wrprot );

  /* Update the 'write protect' menu item */
  switch( which ) {
  case PLUSD_DRIVE_1:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_1_WP_SET,
		      !plusd_drives[ PLUSD_DRIVE_1 ].fdd.wrprot );
    break;
  case PLUSD_DRIVE_2:
    ui_menu_activate( UI_MENU_ITEM_MEDIA_DISK_PLUSD_2_WP_SET,
		      !plusd_drives[ PLUSD_DRIVE_2 ].fdd.wrprot );
    break;
  }
  return 0;
}

int
plusd_disk_write( plusd_drive_number which, const char *filename )
{
  wd_fdc_drive *d = &plusd_drives[ which ];
  int error;
  
  d->disk.type = DISK_TYPE_NONE;
  error = disk_write( &d->disk, filename );

  if( error != DISK_OK ) {
    ui_error( UI_ERROR_ERROR, "couldn't write '%s' file: %s", filename,
	      disk_strerror( error ) );
    return 1;
  }

  return 0;
}

static void
plusd_event_index( libspectrum_dword last_tstates, int type GCC_UNUSED,
                   void *user_data GCC_UNUSED )
{
  int next_tstates;
  int i;

  plusd_index_pulse = !plusd_index_pulse;
  for( i = 0; i < PLUSD_NUM_DRIVES; i++ ) {
    wd_fdc_drive *d = &plusd_drives[ i ];

    d->index_pulse = plusd_index_pulse;
    if( !plusd_index_pulse && d->index_interrupt ) {
      wd_fdc_set_intrq( plusd_fdc );
      d->index_interrupt = 0;
    }
  }
  next_tstates = ( plusd_index_pulse ? 10 : 190 ) *
    machine_current->timings.processor_speed / 1000;
  event_add( last_tstates + next_tstates, index_event );
}

static libspectrum_byte *
alloc_and_copy_page( libspectrum_byte* source_page )
{
  libspectrum_byte *buffer;
  buffer = malloc( MEMORY_PAGE_SIZE );
  if( !buffer ) {
    ui_error( UI_ERROR_ERROR, "Out of memory at %s:%d", __FILE__,
              __LINE__ );
    return 0;
  }

  memcpy( buffer, source_page, MEMORY_PAGE_SIZE );
  return buffer;
}

static void
plusd_enabled_snapshot( libspectrum_snap *snap )
{
  if( libspectrum_snap_plusd_active( snap ) )
    settings_current.plusd = 1;
}

static void
plusd_from_snapshot( libspectrum_snap *snap )
{
  if( !libspectrum_snap_plusd_active( snap ) ) return;

  if( libspectrum_snap_plusd_custom_rom( snap ) &&
      libspectrum_snap_plusd_rom( snap, 0 ) &&
      machine_load_rom_bank_from_buffer(
                             memory_map_romcs, 0, 0,
                             libspectrum_snap_plusd_rom( snap, 0 ),
                             MEMORY_PAGE_SIZE,
                             1 ) )
    return;

  if( libspectrum_snap_plusd_ram( snap, 0 ) ) {
    memcpy( plusd_ram,
            libspectrum_snap_plusd_ram( snap, 0 ), 0x2000 );
  }

  plusd_fdc->direction = libspectrum_snap_beta_direction( snap );

  plusd_cr_write ( 0x00e3, libspectrum_snap_plusd_status ( snap ) );
  plusd_tr_write ( 0x00eb, libspectrum_snap_plusd_track  ( snap ) );
  plusd_sec_write( 0x00f3, libspectrum_snap_plusd_sector ( snap ) );
  plusd_dr_write ( 0x00fb, libspectrum_snap_plusd_data   ( snap ) );
  plusd_cn_write ( 0x00ef, libspectrum_snap_plusd_control( snap ) );

  if( libspectrum_snap_plusd_paged( snap ) ) {
    plusd_page();
  } else {
    plusd_unpage();
  }
}

static void
plusd_to_snapshot( libspectrum_snap *snap GCC_UNUSED )
{
  libspectrum_byte *buffer;

  if( !periph_plusd_active ) return;

  libspectrum_snap_set_plusd_active( snap, 1 );

  buffer = alloc_and_copy_page( memory_map_romcs[0].page );
  if( !buffer ) return;
  libspectrum_snap_set_plusd_rom( snap, 0, buffer );
  if( memory_map_romcs[0].source == MEMORY_SOURCE_CUSTOMROM )
    libspectrum_snap_set_plusd_custom_rom( snap, 1 );

  buffer = alloc_and_copy_page( plusd_ram );
  if( !buffer ) return;
  libspectrum_snap_set_plusd_ram( snap, 0, buffer );

  libspectrum_snap_set_plusd_paged ( snap, plusd_active );
  libspectrum_snap_set_plusd_direction( snap, plusd_fdc->direction );
  libspectrum_snap_set_plusd_status( snap, plusd_fdc->status_register );
  libspectrum_snap_set_plusd_track ( snap, plusd_fdc->track_register );
  libspectrum_snap_set_plusd_sector( snap, plusd_fdc->sector_register );
  libspectrum_snap_set_plusd_data  ( snap, plusd_fdc->data_register );
  libspectrum_snap_set_plusd_control( snap, plusd_control_register );
}
