/***************************************************************************
 *   Copyright (C) 2014 by Subhan Waizi                              		   *
 *                                     									   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
/**  \file i2c_routines.h

	 \brief Software I2C Functions for the FTDI 2232H Chip (header)
	 
i2c_routines is simple library which provides basic i2c-functionality. Functions include sending 1 or more bytes, and reading 1 Byte / 2 Bytes.
The structure of the buffers which will be sent consists of [address - register - data]. This i2c-library can be used
for simple i2c-slaves which do not have the ability of clock stretching.

\warning clock stretching and multi-master-mode is not supported
 
 */
 
 #include "io_operations.h"

/** MSB after 7 address bits should be low to trigger a i2c-write access */
#define SDA_WRITE 0x0
/** MSB after 7 address bits should be high to trigger a i2c-read access */
#define SDA_READ  0x55555555
/** mask which maps to all clock lines of the ftdi 2232h chip */
#define SCL 	  0xAAAAAAAA

/** mask which sets all data lines of channel AD (lowbyte) as output */
#define SDA_0_OUTPUT 0x55
/** mask which sets all data lines of channel AD (lowbyte) as input */
#define SDA_0_INPUT  0x00

/** mask which sets all data lines of channel AC (highbyte) as output */
#define SDA_1_OUTPUT 0x5500
/** mask which sets all data lines of channel AC (highbyte) as input */
#define SDA_1_INPUT  0x00

/** mask which sets all data lines of channel BD (lowbyte) as output */
#define SDA_2_OUTPUT 0x550000
/** mask which sets all data lines of channel BD (lowbyte) as output */
#define SDA_2_INPUT  0x00

/** mask which sets all data lines of channel BC (highbyte) as output */
#define SDA_3_OUTPUT 0x55000000
/** mask which sets all data lines of channel BC (highbyte) as input */
#define SDA_3_INPUT  0x00


void i2c_startCond   ();

void i2c_stopCond    ();

void i2c_clock       (unsigned long ulDataToSend);

void i2c_clockInput  (unsigned long ulDataToSend);

void i2c_giveAck     ();

void i2c_clock_forACK(unsigned long ulDataToSend);

void i2c_getAck      ();

int  i2c_read16      (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength,
                      unsigned short* ausReadBuffer, unsigned char ucRecLength);
					  
int i2c_read72		 (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength, 
					  unsigned char*  aucStatusRegister,
					  unsigned short* ausReadBuffer1, unsigned short* ausReadBuffer2,
					  unsigned short* ausReadBuffer3, unsigned short* ausReadBuffer4, unsigned char ucRecLength);
					  
int  i2c_read8       (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength,
                      unsigned char* aucRecBuffer, unsigned char ucRecLength);
					  
int  i2c_write8      (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength);

int  i2c_write8_x    (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength, unsigned int uiX);