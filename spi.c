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

#define spi_delay() _delay_loop_2(F_CPU/1000000)

static void spi_device_reset(void)
{
    /* set SCK low */
    SPI_PORT &= ~_BV(SPI_SCK);

    /* un-reset device, wait, reset device again */
    SPI_PORT |= _BV(SPI_SS);
    spi_delay();
    SPI_PORT &= ~_BV(SPI_SS);
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

void spi_enable(void)
{
    /* configure MOSI, SCK and SS as output and MISO as input */
    SPI_DDR |= _BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_SS);
    SPI_DDR &= ~_BV(SPI_MISO);

    /* set SS high, SCK and MOSI low and MISO pullup off */
    SPI_PORT |= _BV(SPI_SS);
    SPI_PORT &= ~(_BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_MISO));

    /* initialize spi hardware, prescaler 128 */
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0) | _BV(SPR1);
    SPSR = _BV(SPIF);

    /* reset device */
    SPI_PORT &= ~_BV(SPI_SS);
}

void spi_disable(void)
{
    /* disable spi hardware */
    SPCR = 0;

    /* configure all pins as inputs */
    SPI_DDR &= ~(_BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_SS) | _BV(SPI_MISO));
    SPI_PORT &= ~(_BV(SPI_MOSI) | _BV(SPI_SCK) | _BV(SPI_SS) | _BV(SPI_MISO));
}

/* returns 0 if device has been put into programming mode, 1 otherwise */
uint8_t isp_attach(void)
{
    uint8_t success = 0;

    /* try to connect with lowest spi frequency first */
    uint8_t count = SPI_MAX_TRIES;
    do {
        if (spi_magicbytes() == 0x53) {
            /* device has been put into programming mode */
            success = 1;
            break;
        }
    } while (--count);

    if (!success)
        /* device cannot be reached */
        return 1;

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
        return 1;

    debug_putc('b');
    debug_putc(SPCR & (_BV(SPR0) | _BV(SPR1)));
#endif

    return 0;
}

uint8_t spi_send(uint8_t data)
{
    SPDR = data;
    while(!(SPSR & _BV(SPIF)));
    return SPDR;
}

uint8_t isp_busy(void)
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
