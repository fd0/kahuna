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

#ifndef __SPI_H
#define __SPI_H

#include <stdint.h>

void spi_enable(void);
void spi_disable(void);

uint8_t spi_send(uint8_t data);

/* returns 0 if device has been put into programming mode, 1 otherwise */
uint8_t isp_attach(void);
uint8_t isp_busy(void);
uint8_t isp_read_flash(uint16_t address);
uint8_t isp_read_eeprom(uint16_t address);
void isp_write_eeprom(uint16_t address, uint8_t data);
void isp_write_flash_page(uint16_t address, uint8_t data, uint8_t poll);
void isp_save_flash_page(uint16_t address);

#endif
