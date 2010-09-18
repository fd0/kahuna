/* configuration file for kahuna */

#include "macros.h"

/* uncomment this if you want a usb echo function (for communication testing) */
//#define ENABLE_ECHO_FUNC

/* uncomment this for debug information via uart */
//#define DEBUG_UART

#ifdef HARDWARE_kahuna
    /* isp pins */
    #define SPI_PORTNAME    B
    #define SPI_SS      PB2
    #define SPI_MOSI    PB3
    #define SPI_MISO    PB4
    #define SPI_SCK     PB5
    #define SPI_CS      PB0

    /* led pins */
    #define LED1_PORTNAME   C
    #define LED1_PIN        PC1
    #define LED2_PORTNAME   C
    #define LED2_PIN        PC0

    /* usb */
    #define USB_CFG_IOPORTNAME      D
    #define USB_CFG_DMINUS_BIT      4
    #define USB_CFG_DPLUS_BIT       3
    /* use int1 */
    #define USB_INTR_CFG_SET        ((1 << ISC10) | (1 << ISC11))
    #define USB_INTR_ENABLE_BIT     INT1
    #define USB_INTR_PENDING_BIT    INTF1
    #define USB_INTR_VECTOR         INT1_vect

#elif defined(HARDWARE_rumpus)
    /* isp pins */
    #define SPI_PORTNAME    B
    #define SPI_SS      PB2
    #define SPI_MOSI    PB3
    #define SPI_MISO    PB4
    #define SPI_SCK     PB5
    #define SPI_CS      PB2

    /* led pins */
    #define LED1_PORTNAME   C
    #define LED1_PIN        PC4
    #define LED2_PORTNAME   D
    #define LED2_PIN        PD3

    /* usb */
    #define USB_CFG_IOPORTNAME      D
    #define USB_CFG_DMINUS_BIT      4
    #define USB_CFG_DPLUS_BIT       2
    #define USB_CFG_PULLUP_IOPORTNAME   B
    #define USB_CFG_PULLUP_BIT          0

#else
    #error "unknown hardware platform!"
#endif

/* try to connect to device at lowest spi data rate */
#define SPI_MAX_TRIES_HW    32
#define SPI_MAX_TRIES_SW    8

/* maximum eeprom and flash write timeouts (for _delay_loop_2) */
#define EEPROM_TIMEOUT  (F_CPU/100/4)       /* 10ms */
#define EEPROM_POLL_TIMEOUT (F_CPU/10000/4) /* 100uS */
#define EEPROM_POLL_TRIES   100             /* 100 times */
#define FLASH_TIMEOUT   (F_CPU/200/4)       /* 5ms */
#define FLASH_POLL_TIMEOUT  (F_CPU/10000/4) /* 100uS */
#define FLASH_POLL_TRIES    50              /* 50 times */
#define FLASH_PAGE_TIMEOUT   (F_CPU/100/4)       /* 10ms */
#define FLASH_PAGE_POLL_TIMEOUT  (F_CPU/10000/4) /* 100uS */
#define FLASH_PAGE_POLL_TRIES    100             /* 100 times */

#define DEFAULT_SPI_SW_DELAY    150 /* default delay for software spi, -> 26-33khz (16-20MHz) */

/* more macros */
#define SPI_PORT    _OUTPORT(SPI_PORTNAME)
#define SPI_DDR     _DDRPORT(SPI_PORTNAME)
#define SPI_PIN     _INPORT(SPI_PORTNAME)

#define LED1_DDR    _DDRPORT(LED1_PORTNAME)
#define LED1_PORT   _OUTPORT(LED1_PORTNAME)
#define LED2_DDR    _DDRPORT(LED2_PORTNAME)
#define LED2_PORT   _OUTPORT(LED2_PORTNAME)

#define LED1_ON()   LED1_PORT |= _BV(LED1_PIN)
#define LED1_OFF()  LED1_PORT &= ~_BV(LED1_PIN)
#define LED1_TOGGLE()   LED1_PORT ^= _BV(LED1_PIN)
#define LED2_ON()   LED2_PORT |= _BV(LED2_PIN)
#define LED2_OFF()  LED2_PORT &= ~_BV(LED2_PIN)
#define LED2_TOGGLE()   LED2_PORT ^= _BV(LED2_PIN)

/* usb serial number configuration */
#define CONFIG_USB_SERIAL_LEN 16
