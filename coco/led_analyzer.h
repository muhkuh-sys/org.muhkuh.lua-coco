/***************************************************************************
 *   Copyright (C) 2015 by Subhan Waizi                           		   *
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

/** \file led_analyzer.h

	 \brief led_analyzer handles all functionality on device level and provides the functions needed by the GUI application (header)
	 
The led_analyzer library will handle functionality to work with severeal color controller devices. It scans
for connected color controller devices and opens them. All devices that have been connected to will have handles. 
These handles will be stored in an array and can be used by all underlying color related functions. Furthermore this library
provides the functions which are required for the application CoCo App.
 
 */
 
#include "tcs3472.h"

#ifndef __LED_ANALYZER_H__
#define __LED_ANALYZER_H__

/** Vendor ID of 'Hilscher Gesellschaft für Systemautomation mbH' */
#define VID 0x1939
/** Product ID for the Color Controller "COLOR-CTRL" */
#define PID 0x0024

/** Maximum Length of characters a descriptor in the ftdi2232h eeprom can have */
#define MAX_DESCLENGTH 128

/** \brief Contains Errorcodes and Errorflags which indicate what kind of errors occured

The errorflags indicate what kind of error occured. They get ored with the erroflag of the sensors in order to
show what kind of error occured with what sensor. Furthermore there are errorcodes which indicate errors on 
usb, i2c and device level or if indexing outside array boundaries occur. */

enum E_ERROR
{
	/* Dont write anything with 0x80000000 as we have an int value and this would result in a negative value */
	
	/** Flag - Identification error occured, e.g. the ID register value couldn't be read */
	ERR_FLAG_ID 			= 0x40000000,
	/** Flag - The conversion was not complete at the time the ADC register was accessed */
	ERR_FLAG_INCOMPL_CONV   = 0x20000000,
	/** The maximum amount of clear level was reached, i.e. the sensor got digitally saturated */
	ERR_FLAG_EXCEEDED_CLEAR = 0x10000000,
	/** Errorcode - Fatal error on a device, writing / reading from a ftdi channel failed */
	ERR_DEVICE_FATAL		= 0x8000000,
	/** Errorcode - USB error on a device, which means that we read back a different number of bytes than we expected to read */
	ERR_USB 			    = 0x4000000,
	
	/* last positive error_flag can be     0x1000, values below this flag are used for specifying the exact sensornumber that failed */
	
	/** Indexing outside the handles array (apHandles) */
	ERR_INDEXING			= -100
};

int  scan_devices(char** asSerial, unsigned int uiLength);	
int  connect_to_devices(void** apHandles, int apHlength, char** asLength);
int  read_colors(void** apHandles, int devIndex, unsigned short *ausClear, unsigned short* ausRed,
	 unsigned short *ausGreen, unsigned short* ausBlue,
	 unsigned char *aucIntegrationtime, unsigned char* aucGain);
int  init_sensors(void** apHandles, int devIndex);
int  get_number_of_handles(void ** apHandles);
int  handleToDevice(int handle);
int	 set_gain(void** apHandles, int devIndex, unsigned char gain);
int	 set_gain_x(void** apHandles, int devIndex, unsigned int uiX, unsigned char gain);
int	 get_gain(void** apHandles, int devIndex, unsigned char* aucGains);	
int	 set_intTime(void** apHandles, int devIndex, unsigned char integrationtime);
int	 set_intTime_x(void** apHandles, int devIndex, unsigned int uiX, unsigned char integrationtime);
int	 get_intTime(void** apHandles, int devIndex, unsigned char* aucIntTimeSettings);
int  get_number_of_serials(char** asSerial);
int  swap_serialPos(char** asSerial, unsigned int swap1, unsigned int swap2);
int	 getSerialIndex(char** asSerial, char* curSerial);
int	 swap_up(char** asSerial, char* curSerial);
int	 swap_down(char** asSerial, char* curSerial);
void free_devices(void** apHandles);
void wait4Conversion(unsigned int uiWaitTime);

#endif	/*__LED_ANALYZER_H__*/

