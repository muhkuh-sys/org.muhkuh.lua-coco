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

/** \file io_operations.h 
	\brief provides functions to manipulate ftdi 2232h's I/O-Pins (header)
	
Once the ftdi2232h is set into BITMODE_MPSSE simple USB commands can be sent to it in order to manipulate its input and output pins.
Special commands (for example found in AN_108) can be used to set the 32 GPIO Pins of the ftdi device as either input or output, and once done,
values can be assigned to the output pins and data can be read back from the input pins. These functions will be used to provide software i2c functionality.
*/

#include <stdio.h>
#include "ftdi.h"
#include "libusb.h" 

/** Write command for lowbyte (AD, BD) */
#define W_LOWBYTE 0x80
/** Write command for highbyte (AC, BC) */
#define W_HIGHBYTE 0x82 
/** Read command for lowbyte (AD, BD) */
#define R_LOWBYTE 0x81
/** Read command for highbyte (AC, BC) */
#define R_HIGHBYTE 0x83  

/** Mask for lowbyte of channel A */
#define MASK_ALOW  0x000000FF
/** Mask for higbyte of Channel A */
#define MASK_AHIGH 0x0000FF00
/** Mask for lowbyte of channel B */
#define MASK_BLOW  0x00FF0000
/** Mask for highbyte of channel B */
#define MASK_BHIGH 0xFF000000

/** Set 8 Bits as output */
#define OUTPUT 0xFF
/** Set all Bits as input */
#define MYINPUT 0x00 

/** Error writing to channel A */
#define WRITE_ERR_CH_A -1 
/** Error writing to channel B */
#define WRITE_ERR_CH_B -2 
/** Error reading from channel A */
#define READ_ERR_CH_A -3 
/** Error reading from channel B */
#define READ_ERR_CH_B -4 
/** Error - received a different amount of bytes than expected */
#define ERR_INCORRECT_AMOUNT -5 

int writeOutputs           (struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, const unsigned long ulOutput);

int readInputs             (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, const unsigned char* readBack);

void process_pins          (unsigned long ulIOMask, unsigned long ulOutput);

void process_pins_databack (unsigned long ulIOMask, unsigned long ulOutput);

int send_package_write8    (struct ftdi_context *ftdiA, struct ftdi_context *ftdiB);

int send_package_read8     (struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, unsigned char* aucReadBuffer, unsigned char ucReadBufferLength);

int send_package_read16    (struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, unsigned short* ausReadBuffer, unsigned char ucReadBufferLength);

int send_package_read72  (struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, unsigned char* aucReadBuffer, unsigned short* ausReadBuffer1, unsigned short* ausReadBuffer2,
						    unsigned short* ausReadBuffer3, unsigned short* ausReadBuffer4, unsigned char ucReadBufferLength);



