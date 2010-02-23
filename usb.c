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

#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include "config.h"
#include "usb.h"
#include "usbdrv/usbdrv.h"
#include "spi.h"
#include "debug.h"

/* USBasp requests, taken from the original USBasp sourcecode */
#define USBASP_FUNC_CONNECT     1
#define USBASP_FUNC_DISCONNECT  2
#define USBASP_FUNC_TRANSMIT    3
#define USBASP_FUNC_READFLASH   4
#define USBASP_FUNC_ENABLEPROG  5
#define USBASP_FUNC_WRITEFLASH  6
#define USBASP_FUNC_READEEPROM  7
#define USBASP_FUNC_WRITEEEPROM 8
#define USBASP_FUNC_SETLONGADDRESS 9
#define USBASP_FUNC_SETISPSCK   10

#define PROG_BLOCKFLAG_FIRST    1
#define PROG_BLOCKFLAG_LAST     2

#define USBASP_ISP_SCK_AUTO   0
#define USBASP_ISP_SCK_0_5    1   /* 500 Hz */
#define USBASP_ISP_SCK_1      2   /*   1 kHz */
#define USBASP_ISP_SCK_2      3   /*   2 kHz */
#define USBASP_ISP_SCK_4      4   /*   4 kHz */
#define USBASP_ISP_SCK_8      5   /*   8 kHz */
#define USBASP_ISP_SCK_16     6   /*  16 kHz */
#define USBASP_ISP_SCK_32     7   /*  32 kHz */
#define USBASP_ISP_SCK_93_75  8   /*  93.75 kHz */
#define USBASP_ISP_SCK_187_5  9   /* 187.5  kHz */
#define USBASP_ISP_SCK_375    10  /* 375 kHz   */
#define USBASP_ISP_SCK_750    11  /* 750 kHz   */
#define USBASP_ISP_SCK_1500   12  /* 1.5 MHz   */

/* additional functions */
#define FUNC_ECHO               0x17

/* supply custom usbDeviceConnect() and usbDeviceDisconnect() macros
 * which turn the interrupt on and off at the right times,
 * and prevent the execution of an interrupt while the pullup resistor
 * is switched off */
#ifdef USB_CFG_PULLUP_IOPORTNAME
#undef usbDeviceConnect
#define usbDeviceConnect()      do { \
                                    USB_PULLUP_DDR |= (1<<USB_CFG_PULLUP_BIT); \
                                    USB_PULLUP_OUT |= (1<<USB_CFG_PULLUP_BIT); \
                                    USB_INTR_ENABLE |= (1 << USB_INTR_ENABLE_BIT); \
                                   } while(0);
#undef usbDeviceDisconnect
#define usbDeviceDisconnect()   do { \
                                    USB_INTR_ENABLE &= ~(1 << USB_INTR_ENABLE_BIT); \
                                    USB_PULLUP_DDR &= ~(1<<USB_CFG_PULLUP_BIT); \
                                    USB_PULLUP_OUT &= ~(1<<USB_CFG_PULLUP_BIT); \
                                   } while(0);
#endif

/* programmer state and options */
enum mode_t {
    IDLE = 0,
    READ_FLASH,
    WRITE_FLASH,
    READ_EEPROM,
    WRITE_EEPROM,
};

struct options_t {
    uint16_t address;
    uint16_t bytecount;
    uint16_t pagesize;
    uint8_t blockflags;
    uint16_t pagecounter;
    uint8_t address_mode; /* 0 for old, 1 for new mode */
    enum mode_t mode;
    uint8_t freq;
};

struct options_t opts;


usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t *req = (void *)data;
    uint8_t len = 0;
    static uint8_t buf[4];

    /* set global data pointer to local buffer */
    usbMsgPtr = buf;

    if (req->bRequest == USBASP_FUNC_CONNECT) {
        debug_putc('E');

        /* reset options */
        opts.address = 0;
        opts.address_mode = 0;
        opts.mode = IDLE;

        spi_enable();
        LED1_ON();
    } else if (req->bRequest == USBASP_FUNC_DISCONNECT) {
        debug_putc('e');
        spi_disable();
        LED1_OFF();
    } else if (req->bRequest == USBASP_FUNC_TRANSMIT) {
        buf[0] = spi_send(data[2]);
        buf[1] = spi_send(data[3]);
        buf[2] = spi_send(data[4]);
        buf[3] = spi_send(data[5]);
        len = 4;
    } else if (req->bRequest == USBASP_FUNC_READFLASH) {

        /* load old address, if requested */
        if (!opts.address_mode == 0)
            opts.address = req->wValue.word;

        opts.bytecount = req->wLength.word;
        opts.mode = READ_FLASH;

        debug_putc('R');

        /* call usbFunctionRead() */
        return USB_NO_MSG;
    } else if (req->bRequest == USBASP_FUNC_ENABLEPROG) {
        debug_putc('p');
        buf[0] = !isp_attach(opts.freq);
        len = 1;
    } else if (req->bRequest == USBASP_FUNC_WRITEFLASH) {

        debug_putc('W');

        /* load old address, if requested */
        if (!opts.address_mode == 0)
            opts.address = req->wValue.word;

        opts.pagesize = req->wIndex.bytes[0];
        opts.blockflags = req->wIndex.bytes[1] & 0x0F;
        opts.pagesize += ((uint16_t)(req->wIndex.bytes[1] & 0xF0)) << 4;

        if (opts.blockflags & PROG_BLOCKFLAG_FIRST)
            opts.pagecounter = opts.pagesize;

        opts.bytecount = req->wLength.word;
        opts.mode = WRITE_FLASH;

        return USB_NO_MSG;
    } else if (req->bRequest == USBASP_FUNC_READEEPROM) {

        /* load old address, if requested */
        if (!opts.address_mode == 0)
            opts.address = req->wValue.word;

        opts.bytecount = req->wLength.word;
        opts.mode = READ_EEPROM;

        debug_putc('R');

        /* call usbFunctionRead() */
        return USB_NO_MSG;
    } else if (req->bRequest == USBASP_FUNC_WRITEEEPROM) {

        /* load old address, if requested */
        if (!opts.address_mode == 0)
            opts.address = req->wValue.word;

        opts.bytecount = req->wLength.word;
        opts.mode = WRITE_EEPROM;

        debug_putc('W');

        /* call usbFunctionWrite() */
        return USB_NO_MSG;
    } else if (req->bRequest == USBASP_FUNC_SETLONGADDRESS) {
        opts.address_mode = 1;
        opts.address = req->wValue.word;
    } else if (req->bRequest == USBASP_FUNC_SETISPSCK) {
        opts.freq = data[2];
        buf[0] = 0;
        len = 1;
#ifdef ENABLE_ECHO_FUNC
    } else if (req->bRequest == FUNC_ECHO) {
        buf[0] = req->wValue.bytes[0];
        buf[1] = req->wValue.bytes[1];
        len = 2;
#endif
    }

    return len;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
    uint8_t ret = 0;

    if (opts.bytecount < len)
        len = opts.bytecount;

    for (uint8_t i = 0; i < len; i++) {
        if (opts.mode == WRITE_FLASH) {
            if (opts.pagesize == 0)
                isp_write_flash_page(opts.address, *data, 1);
            else {
                isp_write_flash_page(opts.address, *data, 0);
                opts.pagecounter--;

                /* if a whole flash page is filled, save */
                if (opts.pagecounter == 0) {
                    isp_save_flash_page(opts.address);
                    opts.pagecounter = opts.pagesize;
                }
            }
        } else
            isp_write_eeprom(opts.address, *data);

        opts.bytecount--;

        if (opts.bytecount == 0) {
            /* if this is the last block, and an incomplete page has not yet been
             * written, do it now */
            if (opts.blockflags & PROG_BLOCKFLAG_LAST
                    && opts.pagecounter != opts.pagesize) {
                isp_save_flash_page(opts.address);
            }

            ret = 1;
        }

        opts.address++;
        data++;
    }

    return ret;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
    if (opts.bytecount < len)
        len = opts.bytecount;

    for (uint8_t i = 0; i < len; i++) {
        if (opts.mode == READ_FLASH) {
            *data++ = isp_read_flash(opts.address++);
        } else {
            *data++ = isp_read_eeprom(opts.address++);
        }
    }

    opts.bytecount -= len;

    return len;
}

void usb_init(void)
{
    usbInit();
}

void usb_poll(void)
{
    usbPoll();
}

void usb_disable(void)
{
    usbDeviceDisconnect();
}

void usb_enable(void)
{
    usbDeviceConnect();
    opts.freq = USBASP_ISP_SCK_AUTO;
}
