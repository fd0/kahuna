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

#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <avr/io.h>

/* uart */
#if !defined(UBRR0H) && defined(UBRRH)
#define UBRR0H UBRRH
#endif

#if !defined(UBRR0L) && defined(UBRRL)
#define UBRR0L UBRRL
#endif

#if !defined(UCSR0A) && defined(UCSRA)
#define UCSR0A UCSRA
#endif

#if !defined(UCSR0B) && defined(UCSRB)
#define UCSR0B UCSRB
#endif

#if !defined(UCSR0C) && defined(UCSRC)
#define UCSR0C UCSRC
#endif

#if !defined(UCSZ00) && defined(UCSZ0)
#define UCSZ00 UCSZ0
#endif

#if !defined(UCSZ01) && defined(UCSZ1)
#define UCSZ01 UCSZ1
#endif

#if !defined(TXEN0) && defined(TXEN)
#define TXEN0 TXEN
#endif

#if !defined(UDRE0) && defined(UDRE)
#define UDRE0 UDRE
#endif

#if !defined(UDR0) && defined(UDR)
#define UDR0 UDR
#endif

#if !defined(U2X0) && defined(U2X)
#define U2X0 U2X
#endif

/* timer */
#if !defined(OCR2A) && defined(OCR2)
#define OCR2A OCR2
#endif

#if !defined(TCCR2A) && defined(TCCR2)
#define TCCR2A TCCR2
#endif

#if !defined(TCCR2B) && defined(TCCR2)
#define TCCR2B TCCR2
#endif

#if !defined(TIMSK2) && defined(TIMSK)
#define TIMSK2 TIMSK
#endif

#if !defined(OCIE2A) && defined(OCIE2)
#define OCIE2A OCIE2
#endif

#endif
