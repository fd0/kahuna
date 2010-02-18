/*
 * simple USBasp compatible isp programmer
 * for the rumpus development board
 *
 *   by Alexander Neumann <alexander@lochraster.org>
 *
 * inspired by USBasp by Thomas Fischl,
 * see http://www.obdev.at/products/avrusb/usbasploader.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdint.h>

#ifdef DEBUG
void debug_init(void);
void debug_putc(uint8_t data);
#else
#define debug_init()
#define debug_putc(x)
#endif

#endif
