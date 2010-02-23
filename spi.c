/*
 * kahuna -- simple USBasp compatible isp programmer
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

#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include "spi.h"
#include "config.h"
#include "debug.h"

#define ISP_READY       0xF0
#define ISP_READ_FLASH  0x20
#define ISP_READ_EEPROM 0xA0
#define ISP_WRITE_EEPROM 0xC0
#define ISP_WRITE_FLASH 0x40
#define ISP_WRITE_PAGE  0x4C

struct spi_state_t {
    enum {
        HARDWARE = 0,
        SOFTWARE,
    } mode;
    uint16_t delay;
};

struct spi_state_t spi;

static void spi_device_reset(void)
{
    /* set SCK low */
    SPI_PORT &= ~_BV(SPI_SCK);

    /* un-reset device, wait, reset device again */
    SPI_PORT |= _BV(SPI_CS);
    _delay_loop_2(spi.delay*2);
    SPI_PORT &= ~_BV(SPI_CS);
}

static uint8_t spi_magicbytes(void)
{
    uint8_t echo;

    /* reset device */
    spi_device_reset();

    /* send magic byte sequence */
    spi_send(0xAC);
    spi_send(0x53);
    /* if everything works, the next bytes echoes 0x53 */
    echo = spi_send(0);
    spi_send(0);

    return echo;
}

static void spi_enable_hardware(void)
{
    /* initialize spi hardware, prescaler 128 */
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0) | _BV(SPR1);
    SPSR = _BV(SPIF);

    /* set delay (for spi_device_reset, multiplied by 10,
     * used with _delay_loop_2()) */
    spi.delay = F_CPU/1000000;
}

void spi_enable(void)
{
    /* configure MOSI, SCK and CS as output and MISO as input */
    SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_CS);
    SPI_DDR &= ~_BV(SPI_MISO);

    /* set CS high, SCK and MOSI low and MISO pullup off */
    SPI_PORT |= _BV(SPI_CS);
    SPI_PORT &= ~(_BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_MISO));

    /* if CS pin is not on SS pin, enable pullup for SS pin */
#if SPI_CS != SPI_SS
    SPI_PORT |= _BV(SPI_SS);
#endif

    /* reset device */
    SPI_PORT &= ~_BV(SPI_CS);
}

static void spi_disable_hardware(void)
{
    /* disable spi hardware */
    SPCR = 0;
}

void spi_disable(void)
{
    spi_disable_hardware();

    /* configure all pins as inputs */
    SPI_DDR &= ~(_BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_CS) | _BV(SPI_MISO));
    SPI_PORT &= ~(_BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_CS) | _BV(SPI_MISO));
}

uint8_t spi_send(uint8_t data)
{
    if (spi.mode == HARDWARE) {
        SPDR = data;
        while(!(SPSR & _BV(SPIF)));
        return SPDR;
    } else {
        uint8_t recv = 0;

        for (uint8_t i = 0; i < 8; i++) {

            if (data & _BV(7))
                /* set MOSI high -> send one */
                SPI_PORT |= _BV(SPI_MOSI);
            else
                /* set MOSI low -> send zero */
                SPI_PORT &= ~_BV(SPI_MOSI);

            /* read data at MISO pin */
            recv <<= 1;
            if (SPI_PIN & _BV(SPI_MISO))
                recv |= 1;

            /* rising edge */
            SPI_PORT |= _BV(SPI_SCK);
            _delay_loop_2(spi.delay);
            /* falling edge */
            SPI_PORT &= ~_BV(SPI_SCK);
            _delay_loop_2(spi.delay);

            data <<= 1;
        }

        return recv;
    }
}

/* returns true if device has been put into programming mode, false otherwise */
static bool isp_attach_hardware(void)
{
    uint8_t success = 0;

    /* try to connect with lowest spi frequency first */
    uint8_t count = SPI_MAX_TRIES_HW;
    do {
        if (spi_magicbytes() == 0x53) {
            /* device has been put into programming mode */
            success = 1;
            break;
        }
    } while (--count);

    if (!success)
        /* device cannot be reached */
        return false;

#if (_BV(SPR0) | _BV(SPR1)) != 3
#warning "spi prescaler bits SPR0 and SPR1 are not aligned!"
#warning "automatic spi frequency detection is not possible"
#else
    /* try to decrease prescaler */
    uint8_t prescaler = _BV(SPR0) | _BV(SPR1);  /* decimal value: 3 */
    for (uint8_t i = 0; i < 4; i++) {
        SPCR = _BV(SPE) | _BV(MSTR) | prescaler;

        debug_putc(prescaler);

        /* test device */
        if (spi_magicbytes() != 0x53) {
            /* frequency too high, stop here */
            prescaler++;
            SPCR = _BV(SPE) | _BV(MSTR) | prescaler;
            debug_putc('B');
            break;
        }

        prescaler--;
    }
    /* test again, if this prescaler works */
    if (spi_magicbytes() != 0x53)
        /* device cannot be reached */
        return false;

    debug_putc('b');
    debug_putc(SPCR & (_BV(SPR0) | _BV(SPR1)));
#endif

    return true;
}

/* returns true if device has been put into programming mode, false otherwise */
static bool isp_attach_software(void)
{
    /* try to connect */
    for (uint8_t count = 0; count < SPI_MAX_TRIES_SW; count++) {
        if (spi_magicbytes() == 0x53) {
            /* device has been put into programming mode */
            return true;
        }
    }

    /* device does not react */
    return false;
}

/* returns true if device has been put into programming mode, false otherwise */
bool isp_attach(uint8_t freq)
{
    if (freq == 0) {
        /* try auto */
        debug_putc('A');

        /* try hardware (hardware is enabled and configured after call to this function) */
        spi_enable_hardware();
        spi.mode = HARDWARE;
        debug_putc('H');
        if (isp_attach_hardware()) {
            debug_putc('t');
            return true;
        }

        /* else disable hardware */
        spi_disable_hardware();
        spi.mode = SOFTWARE;
        debug_putc('S');

        /* and try software (with frequency F_CPU/4/150 =~ 26-33khz) */
        spi.delay = DEFAULT_SPI_SW_DELAY;
        if (isp_attach_software()) {
            spi.mode = SOFTWARE;
            debug_putc('t');
            return true;
        }
    } else {
        /* manual spi, in software */
        debug_putc('M');
        debug_putc(freq);

        spi_disable_hardware();
        spi.mode = SOFTWARE;

        /* find correct delay value:
         * USBASP_ISP_SCK_AUTO   0
         * USBASP_ISP_SCK_0_5    1    500 Hz
         * USBASP_ISP_SCK_1      2      1 kHz
         * USBASP_ISP_SCK_2      3      2 kHz
         * USBASP_ISP_SCK_4      4      4 kHz
         * USBASP_ISP_SCK_8      5      8 kHz
         * USBASP_ISP_SCK_16     6     16 kHz
         * USBASP_ISP_SCK_32     7     32 kHz
         * USBASP_ISP_SCK_93_75  8     93.75 kHz
         * USBASP_ISP_SCK_187_5  9    187.5  kHz
         * USBASP_ISP_SCK_375    10   375 kHz
         * USBASP_ISP_SCK_750    11   750 kHz
         * USBASP_ISP_SCK_1500   12   1.5 MHz
         */
        switch (freq) {
            case 1:     spi.delay = F_CPU/4/500; break;
            case 2:     spi.delay = F_CPU/4/1000; break;
            case 3:     spi.delay = F_CPU/4/2000; break;
            case 4:     spi.delay = F_CPU/4/4000; break;
            case 5:     spi.delay = F_CPU/4/8000; break;
            case 6:     spi.delay = F_CPU/4/16000; break;
            case 7:     spi.delay = F_CPU/4/32000; break;
            case 8:     spi.delay = F_CPU/4/93750; break;
            case 9:     spi.delay = F_CPU/4/187500; break;
            case 10:    spi.delay = F_CPU/4/375000; break;
            case 11:    spi.delay = F_CPU/4/750000; break;
            default:    spi.delay = F_CPU/4/1500000; break;
        }
        debug_putc(HI8(spi.delay));
        debug_putc(LO8(spi.delay));

        if (isp_attach_software()) {
            spi.mode = SOFTWARE;
            debug_putc('t');
            return true;
        }
    }

    return 0;
}

bool isp_busy(void)
{
    spi_send(ISP_READY);
    spi_send(0);
    spi_send(0);
    return (spi_send(0) & 1);
}

uint8_t isp_read_flash(uint16_t address)
{
    /* send 0x20 if low byte is to be read,
     * send 0x28 if high byte is to be read */
    spi_send(ISP_READ_FLASH | (address & 1) << 3);

    /* just transmit the word address */
    address >>= 1;
    spi_send(HI8(address));
    spi_send(LO8(address));
    return spi_send(0);
}

uint8_t isp_read_eeprom(uint16_t address)
{
    spi_send(ISP_READ_EEPROM);
    spi_send(HI8(address));
    spi_send(LO8(address));
    return spi_send(0);
}

void isp_write_eeprom(uint16_t address, uint8_t data)
{
    spi_send(ISP_WRITE_EEPROM);
    spi_send(HI8(address));
    spi_send(LO8(address));
    spi_send(data);

    /* poll until byte has been written */
    if (data == 0xff)
        _delay_loop_2(EEPROM_TIMEOUT);
    else {
        for (uint8_t i = 0; i < EEPROM_POLL_TRIES; i++) {
            if (isp_read_eeprom(address) == data)
                break;
            _delay_loop_2(EEPROM_POLL_TIMEOUT);
        }
    }
}

void isp_write_flash_page(uint16_t address, uint8_t data, uint8_t poll)
{
    /* send 0x40 if low byte is to be written,
     * send 0x48 if high byte is to be written */
    spi_send(ISP_WRITE_FLASH | (address & 1) << 3);

    /* just transmit the word address */
    uint16_t word_address = (address >> 1);
    spi_send(HI8(word_address));
    spi_send(LO8(word_address));
    spi_send(data);

    if (!poll)
        return;

    if (data == 0xff)
        /* just wait the maximum time */
        _delay_loop_2(FLASH_TIMEOUT);
    else {
        for (uint8_t i = 0; i < FLASH_POLL_TRIES; i++) {
            if (isp_read_flash(address) != 0xff)
                break;
            _delay_loop_2(FLASH_POLL_TIMEOUT);
        }
    }
}

void isp_save_flash_page(uint16_t address)
{
    spi_send(ISP_WRITE_PAGE);

    /* just send word address */
    address >>= 1;
    spi_send(HI8(address));
    spi_send(LO8(address));
    spi_send(0);

    for (uint8_t i = 0; i < FLASH_PAGE_POLL_TRIES; i++) {
        if (isp_read_flash(address) != 0xff)
            break;
        _delay_loop_2(FLASH_PAGE_POLL_TIMEOUT);
    }
}
