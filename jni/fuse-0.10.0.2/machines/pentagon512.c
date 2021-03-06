/* pentagon512.c: Pentagon 512K specific routines. This machine is expected to
                  be a post-1996 Pentagon (a 512k v1.x 1024SL?). It is
                  different to the Pentagon 128k model as we want to be able to
                  exchange snapshots with emulators that do not support this
                  model but do support the older style Pentagon (SPIN,
                  Spectaculator, xzx-pro etc. etc.)..
   Copyright (c) 1999-2007 Philip Kendall and Fredrick Meunier

   $Id: pentagon512.c 3599 2008-04-09 13:16:13Z fredm $

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

#include <config.h>

#include <libspectrum.h>

#include "compat.h"
#include "disk/beta.h"
#include "joystick.h"
#include "machine.h"
#include "machines.h"
#include "memory.h"
#include "pentagon.h"
#include "periph.h"
#include "settings.h"
#include "spec128.h"
#include "ula.h"

static int pentagon_reset( void );
static int pentagon_memory_map( void );

int 
pentagon512_init( fuse_machine_info *machine )
{
  machine->machine = LIBSPECTRUM_MACHINE_PENT512;
  machine->id = "pentagon512";

  machine->reset = pentagon_reset;

  machine->timex = 0;
  machine->ram.port_from_ula  = pentagon_port_from_ula;
  machine->ram.contend_delay  = spectrum_contend_delay_none;
  machine->ram.contend_delay_no_mreq = spectrum_contend_delay_none;

  machine->unattached_port = spectrum_unattached_port_none;

  machine->shutdown = NULL;

  machine->memory_map = pentagon_memory_map;

  return 0;
}

static int
pentagon_reset(void)
{
  int error;
  int i;

  error = machine_load_rom( 0, 0, settings_current.rom_pentagon512_0,
                            settings_default.rom_pentagon512_0, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 2, 1, settings_current.rom_pentagon512_1,
                            settings_default.rom_pentagon512_1, 0x4000 );
  if( error ) return error;
  error = machine_load_rom( 4, 2, settings_current.rom_pentagon512_3,
                            settings_default.rom_pentagon512_3, 0x4000 );
  if( error ) return error;
  error = machine_load_rom_bank( memory_map_romcs, 0, 0,
                                 settings_current.rom_pentagon512_2,
                                 settings_default.rom_pentagon512_2, 0x4000 );
  if( error ) return error;

  error = spec128_common_reset( 0 );
  if( error ) return error;

  error = periph_setup( pentagon_peripherals, pentagon_peripherals_count );
  if( error ) return error;
  periph_setup_kempston( PERIPH_PRESENT_OPTIONAL );
  periph_setup_beta128( PERIPH_PRESENT_ALWAYS );
  periph_update();

  beta_builtin = 1;
  beta_active = 1;

  machine_current->ram.last_byte2 = 0;
  machine_current->ram.special = 0;

  /* Mark the least 384K as present/writeable */
  for( i = 16; i < 64; i++ )
    memory_map_ram[i].writable = 1;

  return 0;
}

static int
pentagon_memory_map( void )
{
  int rom, page, screen;
  size_t i;

  screen = ( machine_current->ram.last_byte & 0x08 ) ? 7 : 5;
  if( memory_current_screen != screen ) {
    display_update_critical( 0, 0 );
    display_refresh_main_screen();
    memory_current_screen = screen;
  }

  if( beta_active && !( machine_current->ram.last_byte & 0x10 ) ) {
    rom = 2;
  } else {
    rom = ( machine_current->ram.last_byte & 0x10 ) >> 4;
  }

  machine_current->ram.current_rom = rom;

  spec128_select_rom( rom );

  page = machine_current->ram.last_byte & 0x07;

  page += ( machine_current->ram.last_byte & 0xC0 ) >> 3;

  spec128_select_page( page );
  machine_current->ram.current_page = page;

  for( i = 0; i < 8; i++ )
    memory_map_read[i] = memory_map_write[i] = *memory_map_home[i];

  memory_romcs_map();

  return 0;
}
