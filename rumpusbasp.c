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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>

#include "config.h"
#include "timer.h"
#include "usb.h"
#include "debug.h"

int main(void)
{
    debug_init();

    /* init led pins */
    LED1_DDR |= _BV(LED1_PIN);
    LED2_DDR |= _BV(LED2_PIN);
    LED1_OFF();
    LED2_OFF();

    /* initialize usb pins */
    usb_init();

    /* enable interrupts */
    sei();

    /* disconnect for ~500ms, so that the host re-enumerates this device */
    usb_disable();
    for (uint8_t i = 0; i < 38; i++)
        _delay_loop_2(0); /* 0 means 0x10000, 38*1/f*0x10000 =~ 498ms */
    usb_enable();

    /* init timer */
    timer_init();

    timer_t blink_timer;
    timer_set(&blink_timer, 50);
    while(1) {
        usb_poll();

        /* do some led blinking, so that it is visible that the programmer is running */
        if (timer_expired(&blink_timer)) {
            LED2_TOGGLE();
            timer_set(&blink_timer, 50);
        }
    }
}
