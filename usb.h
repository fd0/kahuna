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

#ifndef __USB_H
#define __USB_H

/* api functions */

/* initialize the usb stack, D+ and D- pins MUST be configured as input, with
 * pull-ups disabled, interrupts MUST be enabled AFTER calling usb_init() */
void usb_init(void);

/* poll at least every 50ms */
void usb_poll(void);

void usb_disable(void);
void usb_enable(void);

#endif
