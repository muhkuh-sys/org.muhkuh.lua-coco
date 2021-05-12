/***************************************************************************
 *   Copyright (C) 2015 by Subhan Waizi                                    *
 *                                                                         *
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
 
/** \file led_analyzer.c

     \brief led_analyzer handles all functionality on device level and provides the functions needed by the GUI application.

The led_analyzer library will handle functionality to work with severeal color controller devices. It scans
for connected color controller devices and opens them. Handles will be assigned to all opened color controller devices.
These handles will be stored in an array and can be used by all underlying color related functions. Furthermore this library
provides the functions which are required for the application CoCo App.

*/

#include "led_analyzer.h"

/* This is for the "malloc" function. */
#include <stdlib.h>
/* This is for the "strcpy" and "memset" functions. */
#include <string.h>
/* This is for the "sleep_ms" macro. */
#include "sleep_ms.h"

/** \brief scans for connected color controller devices and stores their serial numbers in an array.

Functions scans for all color controller devices with a given VID and PID that are connected via USB. A device which has "COLOR-CTRL" 
as description will be counted as a color controller. Function prints manufacturer, description and serialnumber of connected devices.
Furthermore the serialnumber(s) will be stored in an array and can be used by functions that open a connected device by a serialnumber. 
    @param asSerial stores the serial numbers of all connected color controller devices
    @param asLength maximum number of elements the serial number array can contain 

    @retval  0 no color controller device detected
    @retval <0 error with ftdi functions or insufficient space for storing all serial numbers in asSerial
    @retval >0 number of connected color controller devices with given VID, PID and "COLOR-CTRL" as description
*/
int scan_devices(char** asSerial, unsigned int asLength)
{
	int i;
	int f;
	int numbOfDevs = 0;
	int numbOfSerials = 0;
	const char sMatch[] = "COLOR-CTRL";
	char manufacturer[MAX_DESCLENGTH], description[MAX_DESCLENGTH], serial[MAX_DESCLENGTH];
	struct ftdi_device_list *devlist, *curdev;
	struct ftdi_context *ftdi;


	if( asLength<1 )
	{
		printf("error - length of serialnumber array too small ... \n");
		return -1;
	}

	ftdi = ftdi_new();
	if( ftdi==NULL )
	{
		fprintf(stderr, "... ftdi_new failed\n");
		ftdi_list_free(&devlist);
		ftdi_free(ftdi);
		return -1;
	}

	numbOfDevs = ftdi_usb_find_all(ftdi, &devlist, VID, PID);
	if(numbOfDevs == 0)
	{
		printf("... no color controller detected ... quitting.\n");
		ftdi_list_free(&devlist);
		ftdi_free(ftdi);
		return 0;
	}

	printf("\n");

	i = 0;
	for(curdev=devlist; curdev!=NULL; i++)
	{
		printf("Scanning device %d\n", i);

		f = ftdi_usb_get_strings(ftdi, curdev->dev, manufacturer, 128, description, 128, serial, 128);
		if( f<0 )
		{
			fprintf(stderr, "... ftdi_usb_get_strings failed: %d (%s) ... installed libusbK driver ?\n", f, ftdi_get_error_string(ftdi));
			ftdi_list_free(&devlist);
			ftdi_free(ftdi);
			return -2;
		}
		printf("Manufacturer: %s, Description: %s, Serial: %s\n\n", manufacturer, description, serial);

		if(strcmp(sMatch, description) == 0)
		{
			numbOfSerials++;
			asSerial[i] = (char*) malloc(sizeof(serial));
			strcpy(asSerial[i], serial);
		}

		curdev = curdev->next;
	}

	ftdi_list_free(&devlist);
	ftdi_free(ftdi);

	if( numbOfSerials==0 )
	{
		printf("... color controller(s) with given VID, PID detected, but description doesn't match.\n");
		ftdi_list_free(&devlist);
		ftdi_free(ftdi);
	}
	
	return numbOfSerials;
}



/** \brief connects to all USB devices with a given serial number.

Function opens all USB devices which have a serial number that equals one of the serial numbers given in asSerial.
Furthermore it initializes the devices by configuring the right channels with the right modes. After having successfully opened
a device, the handle of the opened device will be stored in apHandles. As each (ftdi2232h) color controller device has 2 channels
each connected color controller will get 2 handles in apHandles.
    @param apHandles    stores the handles of all opened USB color controller devices
    @param apHlength    maximum number of handles apHandles can store
    @param asSerial     stores the serial numbers of all connected color controller devices

    @retval  0 opened no color controller device
    @retval -1 error with ftdi functions or insufficient length of apHandles
    @retval >0 number of opened color controller devices
*/

int connect_to_devices(void** apHandles, int apHlength, char** asSerial)
{
	int numbOfDevs;
	int iArrayPos;
	int devCounter;
	int f;


	numbOfDevs = get_number_of_serials(asSerial);
	printf("Number of Color Controllers found: %d\n\n", numbOfDevs);

	if( 2*numbOfDevs>apHlength )
	{
		printf("handlearray too small for number of color controllers found\n");
		printf("needed: %d, got: %d\n", numbOfDevs*2, apHlength);
		return -1;
	}

	memset(apHandles, 0, sizeof(void*) * apHlength);

	devCounter = 0;
	iArrayPos = 0;
	while( devCounter<numbOfDevs )
	{
		printf("Connecting to device %d - %s\n", devCounter, asSerial[devCounter]);

		if( iArrayPos+2<=apHlength )
		{
			/* Ch A */
			apHandles[iArrayPos] = ftdi_new();
			if( apHandles[iArrayPos]==0 )
			{
				fprintf(stderr, "... ftdi_new failed!\n");
				return -1;
			}

			f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_A);
			if( f<0 )
			{
				fprintf(stderr, "... unable to attach to device %d interface A: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}

			if( asSerial[devCounter]==NULL)
			{
				printf("... serial number non-existent ... make sure a color controller device is connected\n");
				ftdi_deinit(apHandles[iArrayPos]);
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}

			f = ftdi_usb_open_desc(apHandles[iArrayPos], VID, PID, NULL, asSerial[devCounter]);
			if( f<0 )
			{
				fprintf(stderr, "... unable to open device %d interface A: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_deinit(apHandles[iArrayPos]);
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}
			else
			{
				 printf("color controller %d Channel A - open succeeded\n", devCounter);
			}

			f=ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE);
			if( f<0 )
			{
				fprintf(stderr, "... unable to set the mode on device %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}
			else
			{
				printf("enabling MPSSE mode on device %d Channel A\n", devCounter);
			}

			f=ftdi_usb_purge_buffers(apHandles[iArrayPos]);
			if( f<0 )
			{
				fprintf(stderr, "... unable to purge buffers on device %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}


			iArrayPos ++;

			/* Ch B */
			apHandles[iArrayPos] = ftdi_new();
			if( apHandles[iArrayPos]==0 )
			{
				fprintf(stderr, "... ftdi_new failed!\n");
				return -1;
			}

			f = ftdi_set_interface(apHandles[iArrayPos], INTERFACE_B);
			if( f<0 )
			{
				fprintf(stderr, "... unable to attach to device %d interface B: %d, (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}

			if( asSerial[devCounter]==NULL )
			{
				printf("... serial number non-existent ... make sure a correct color controller device is connected\n");
				ftdi_deinit(apHandles[iArrayPos]);
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}

			f = ftdi_usb_open_desc(apHandles[iArrayPos],VID, PID, NULL, asSerial[devCounter]);
			if( f<0 )
			{
				fprintf(stderr, "... unable to open device %d interface B: %d (%s)\n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_deinit(apHandles[iArrayPos]);
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}
			else
			{
				printf("color controller %d Channel B - open succeeded\n", devCounter);
			}

			f = ftdi_set_bitmode(apHandles[iArrayPos], 0xFF, BITMODE_MPSSE);
			if( f<0 )
			{
				fprintf(stderr, "unable to set the mode on device %d Channel B: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}
			else
			{
				printf("enabling MPSSE mode on device %d Channel B\n", devCounter);
			}

			f = ftdi_usb_purge_buffers(apHandles[iArrayPos]);
			if( f<0 )
			{
				fprintf(stderr, "... unable to purge buffers on device %d Channel A: %d (%s) \n", devCounter, f, ftdi_get_error_string(apHandles[iArrayPos]));
				ftdi_free(apHandles[iArrayPos]);
				return -1;
			}

			iArrayPos ++;
		}
		/* Go to the next device found */
		devCounter ++;

		printf("\n");
	}

	printf("\n");

	return devCounter;
}



/** \brief returns number of serial numbers stored in the serial number array.
    @param asSerial     stores the serial numbers of all connected color controller devices

    @return             number of elements in the serial number array
*/
int get_number_of_serials(char** asSerial)
{
	int counter = 0;


	while( asSerial[counter]!=NULL )
	{
		counter++;
	}

	return counter;
}


/** \brief returns number of handles stored in the handle array.
    @param apHandles array that stores the handles

    @return number of elements in the handle array
*/

int get_number_of_handles(void ** apHandles)
{
	int counter = 0;


	while( apHandles[counter]!=NULL )
	{
		counter++;
	}

	return counter;
}


/** \brief returns the device number corresponding to a certain handleIndex.

As each device has 2 handles, the device index and the handle index are not the same. Following functions
provides an easy way to get the device number if a handle index is given 
    @param handleIndex  index of the handle

    @return             device index that corresponds to the handle index
*/
int handleToDevice(int handleIndex)
{
	return (int)(handleIndex/2);
}





/** \brief initializes the sensors of a color controller device.

Function initializes the 16 sensors of a color controller device. Initializing includes turning the sensors on
clearing their interrupt flags and identifying them to be sure that the i2c-protocol works and following color readings
are valid.
    @param apHandles       array that stores ftdi2232h handles
    @param devIndex        device index of current color controller device

    @retval 0  Succesful
    @retval >0 Flag in DWORD HIGH marks what kind of error occured, 16 bits in DWORD LOW mark which of the 16 sensors failed
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int init_sensors(void** apHandles, int devIndex)
{
	/* 2 handles per device, device 0 has handles 0,1 .. device 1 has handles 2,3 and so on*/
	int handleIndex = devIndex * 2;
	int iErrorcode = 0;
	unsigned char aucTempbuffer[16];

	int iHandleLength = get_number_of_handles(apHandles);


	if( handleIndex>=iHandleLength )
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		return ERR_INDEXING;
	}

	if(tcs_clearInt(apHandles[handleIndex], apHandles[handleIndex+1]) != 0)
	{
		printf("... failed to clear interrupt channel on device %d...\n", devIndex);
	}

	if(tcs_ON(apHandles[handleIndex], apHandles[handleIndex+1]) != 0)
	{
		printf("... failed to turn the sensors on on device %d...\n", devIndex);
	}

	iErrorcode = tcs_identify(apHandles[handleIndex], apHandles[handleIndex+1], aucTempbuffer);
	if( iErrorcode>0 )
	{
		return (iErrorcode | ERR_FLAG_ID);
	}

	return iErrorcode;
}



/** \brief reads the RGBC colors of all sensors under a device and checks if the colors are valid

Function reads the colors red, green, blue and clear of all 16 sensors under a device and stores them in adequate buffers.
Furthermore the function will check if maximum clear levels have been exceeded. If so, the color reading won't be valid, as the sensor has
been saturated, and color reading for failing sensor(s) should be redone. If maximum clear levels are beeing exceeded too often, one
should consider lowering gain and/or integration time settings. The function will return a returncode which can be used to determine
which of the color sensors have exceeded maximum clear levels. Furthermore the function will store the sensors' measured
LUX level in an array. This level is calculated by a formula given in AMS / TAOS Designer's Note 40.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device
    @param ausClear             stores 16 clear colors
    @param ausRed               stores 16 red colors
    @param ausGreen             stores 16 green colors
    @param ausBlue              stores 16 blue colors
    @param aucIntegrationtime   stores 16 integration time values of the sensors
    @param aucGain              stores 16 gain values of the sensors

    @retval 0  Succesful
    @retval >0 Flag in DWORD HIGH marks what kind of error occured, 16 bits in DWORD LOW mark which of the 16 sensors failed
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int read_colors(void** apHandles, int devIndex, unsigned short* ausClear, unsigned short* ausRed,
                unsigned short* ausGreen, unsigned short* ausBlue,
                unsigned char* aucIntegrationtime, unsigned char* aucGain)
{
	int iHandleLength;
	int handleIndex;
	int iErrorcode;
	unsigned char aucTempbuffer[16];
	int iResult;


	iErrorcode = 0;

	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	/* Transform device index into handle index, as each device has two handles */
	if( handleIndex>=iHandleLength )
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		tcs_getIntegrationtime(apHandles[handleIndex], apHandles[handleIndex+1], aucIntegrationtime);
		tcs_getGain(apHandles[handleIndex], apHandles[handleIndex+1], aucGain);

		iErrorcode = tcs_readColors(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, ausRed, ausGreen, ausBlue);
		/* Fatal error has occured as we could not read from channel A and channel B */
		if(iErrorcode <= -1 && iErrorcode >= -4)
		{
			iResult = ERR_DEVICE_FATAL;
		}
		/* Usb error has occured - read different amount of bytes than expected */
		else if(iErrorcode <= -5 && iErrorcode >= -6)
		{
			iResult = ERR_USB;
		}
		/* Some sensors have not finished their conversion cycle yet */
		else if(iErrorcode >  0)
		{
			iResult = iErrorcode | ERR_FLAG_INCOMPL_CONV;
		}
		else
		{
			/* Clear levels have been exceeded on some sensors */
			iErrorcode = tcs_exClear(apHandles[handleIndex], apHandles[handleIndex+1], ausClear, aucIntegrationtime);
			if( iErrorcode>0 )
			{
				iResult = iErrorcode | ERR_FLAG_EXCEEDED_CLEAR;
			}
			else
			{
				iResult = iErrorcode;
			}
		}
	}
	
	return iResult;
}



/** \brief frees the memory of all connected opened color controller devices.

Function iterates over all handle elements in apHandles and frees the memory. Freeing includes closing
the usb_device and freeing the memory allocated by the device handle.
    @param apHandles            array that stores ftdi2232h handles
*/

void free_devices(void** apHandles)
{
	int index;
	int iHandleLength;


	iHandleLength = get_number_of_handles(apHandles);
	printf("Number of handles to delete: %d\n", iHandleLength);

	index = 0;
	while( index<iHandleLength )
	{
		printf("Freeing handle # %d on device # %d\n", index, handleToDevice(index));
		ftdi_usb_close(apHandles[index]);
		ftdi_free(apHandles[index]);
		apHandles[index] = NULL;
		index ++;
	}
}



/** \brief sets the integration time of one sensor.

Function sets the integration time of one sensor. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a longer integration time, the integration time for bright LEDs can be low. Refer to
the sensor's datasheet for calculating the content of the integration time register. A few common values have already
been calculated and saved in tcs3472Integration_t.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device 
    @param uiX                  sensor which will get the new integration time ( 0 ... 15 )
    @param integrationtime      integration time to be sent to the sensor

    @retval 0  Succesful
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int set_intTime_x(void** apHandles, int devIndex, unsigned int uiX, unsigned char integrationtime)
{
	int iHandleLength;
	int handleIndex;
	int iResult;


	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		iResult = tcs_setIntegrationTime_x(apHandles[handleIndex], apHandles[handleIndex+1], integrationtime, uiX);
	}

	return iResult;
}



/** \brief sets the gain of one sensor.

Function sets the gain of 1 sensor. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to
the sensor's datasheet for further information about gain.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device
    @param uiX                  sensor which will get the new gain ( 0 ... 15 )
    @param gain                 gain to be sent to the sensor

    @retval 0  Succesful 
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int set_gain_x(void** apHandles, int devIndex, unsigned int uiX, unsigned char gain)
{
	int iHandleLength;
	int handleIndex;
	int iResult;


	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		iResult = tcs_setGain_x(apHandles[handleIndex], apHandles[handleIndex+1], gain, uiX);
	}

	return iResult;
}



/** \brief sets the gain of 16 sensors under a device.

Function sets the gain of 16 sensors of a device. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to
the sensor's datasheet for further information about gain.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device
    @param gain                 gain to be sent to the sensors

    @retval 0  Succesful 
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int set_gain(void** apHandles, int devIndex, unsigned char gain)
{
	int iHandleLength;
	int handleIndex;
	int iResult;


	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		iResult = tcs_setGain(apHandles[handleIndex], apHandles[handleIndex+1], gain);
	}

	return iResult;
}


/** \brief reads the current gain setting of 16 sensors under a device and stores them in an adequate buffer.

The function reads back the gain settings of 16 sensors. Refer to sensors' datasheet for further information about
gain settings.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device
    @param aucGains             buffer which will contain the gain settings of the 16 sensors

    @retval 0  Succesful 
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int get_gain(void** apHandles, int devIndex, unsigned char* aucGains)
{
	int iHandleLength;
	int handleIndex;
	int iResult;


	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		iResult = tcs_getGain(apHandles[handleIndex], apHandles[handleIndex+1], aucGains);
	}

	return iResult;
}



/** \brief sets the integration time of 16 sensors under a device.

Function sets the integration time of 16 sensors. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a longer integration time, the integration time for bright LEDs can be low. Refer to
the sensor's datasheet for calculating the content of the integration time register. A few common values have already
been calculated and saved in enum tcs3472Integration_t.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device
    @param integrationtime      integration time to be sent to the sensors

    @retval 0  Succesful 
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/
int set_intTime(void** apHandles, int devIndex, unsigned char integrationtime)
{
	int iHandleLength;
	int handleIndex;
	int iResult;


	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		iResult = tcs_setIntegrationTime(apHandles[handleIndex], apHandles[handleIndex+1], integrationtime);
	}

	return iResult;
}



/** \brief reads the integration time setting of 16 sensors under a device and stores them in an adequate buffer.

The function reads back the integration time of 16 sensors. Refer to sensors' datasheet for further information about
integration time settings.
    @param apHandles            array that stores ftdi2232h handles
    @param devIndex             device index of current color controller device
    @param aucIntegrationtime   pointer to buffer which will store the integration time settings of the 16 sensors

    @retval 0  Succesful
    @retval <0 USB errors, i2c errors or indexing errors occured, check return value for further information
*/

int get_intTime(void** apHandles, int devIndex, unsigned char* aucIntegrationtime)
{
	int iHandleLength;
	int handleIndex;
	int iResult;


	iHandleLength = get_number_of_handles(apHandles);
	handleIndex = devIndex * 2;
	if(handleIndex >= iHandleLength)
	{
		printf("Exceeded maximum amount of handles ... \n");
		printf("Amount of handles: %d trying to index: %d\n", iHandleLength, handleIndex);
		iResult = ERR_INDEXING;
	}
	else
	{
		iResult = tcs_getIntegrationtime(apHandles[handleIndex], apHandles[handleIndex+1], aucIntegrationtime);
	}

	return iResult;
}



/** waits for a time specified in uiWaitTime ]1 ms ... 10 s] max 
    @param uiWaitTime   time in milliseconds to wait in order to let the sensors complete their ADC measurements
*/
void wait4Conversion(unsigned int uiWaitTime)
{
	if( (uiWaitTime<=0) || (uiWaitTime>10000) )
	{
		uiWaitTime = 200;
	}

	sleep_ms(uiWaitTime);
}


/** swaps the position of two serial numbers located in an array of serial numbers.

Function swaps the location of two serial numbers which are located in an array of serial numbers. This function
will be used for having control over the opening order of usb devices, as the current algorithm iterates over
the serial number array and opens the devices with those serial numbers. Thus a color controller device with a serial
number  at location 0 will be opened before a device with a serial number located at index 1. The handles which correspond
to the opened color controller devices will be stored in the same order, as the order of opening. If the function
has finished successfully the serial number which was in pos1 will be in position 2 and
    @param asSerial     array which contains serial numbers of color controller devices
    @param pos1         current position of one swap operand
    @param pos2         target position of the swap operand

    @retval   0   OK - swapping successful
    @return  -1   reaching out of the serial number array
*/

int swap_serialPos(char** asSerial, unsigned int pos1, unsigned int pos2)
{
	int numbOfDevs;
	int iResult;
	char temp[MAX_DESCLENGTH];


	numbOfDevs = get_number_of_serials(asSerial);
	if( (pos1>=numbOfDevs) || (pos1<0) )
	{
		printf("Reaching out of seralnumber array ... cannot swap\n");
		iResult = -1;
	}
	else if( (pos2>=numbOfDevs) || (pos2<0) )
	{
		printf("Reaching out of seralnumber array ... cannot swap\n");
		iResult = -1;
	}
	else
	{
		/* Temporary store of Serials old position */
		strcpy(temp, asSerial[pos1]);
		/* store the serial number into the new position */
		strcpy(asSerial[pos1], asSerial[pos2]);
		/* Restore the old position */
		strcpy(asSerial[pos2], temp);
		/* OK */
		iResult = 0;
	}

	return iResult;
}



/** \brief returns the index of the serial number described by curSerial.

Function returns the serial number that curSerial currently has in the serial number array asSerial.
    @param asSerial      array which contains serial numbers of color controller devices
    @param curSerial     current serial number

    @retval              index / position of current serial number in asSerial
*/
int getSerialIndex(char** asSerial, char* curSerial)
{
	int numbOfDevs;
	int i;
	int iCmp;
	int iResult;


	iResult = -1;
	numbOfDevs = get_number_of_serials(asSerial);
	for(i=0; i<numbOfDevs; i++)
	{
		iCmp = strcmp(asSerial[i], curSerial);
		if( iCmp==0 )
		{
			iResult = i;
			break;
		}
	}

	if( iResult==-1 )
	{
		printf("... serial not found - cannot return an index\n");
	}

	return iResult;
}



/** \brief swaps the serial number described by curSerial up by one position.

Function swaps the serial number described by curSerial up by one position. The serial number
that got pushed away will be in [oldPosition - 1].
    @retval 0  OK - swap successful or no need to swap
    @retval -1 swapping failed
*/
int swap_up(char** asSerial, char* curSerial)
{
	int curIndex;
	int iResult;


	curIndex = getSerialIndex(asSerial, curSerial);
	if( curIndex<0 )
	{
		printf("... cannot swap serial position up\n");
		iResult = -1;
	}
	else if( curIndex==0 )
	{
		printf("... no need to swap up, serial number already in first position\n");
		iResult = 0;
	}
	else
	{
		iResult = swap_serialPos(asSerial, curIndex, curIndex-1);
	}

	return iResult;
}



/** \brief swaps the serial number described by curSerial down by one position.

Function swaps the serial number described by curSerial down by one position. The serial number
that got pushed away will be in [oldPosition + 1].
    @retval 0  OK - swap successful or no need to swap
    @retval -1 swapping failed 
*/
int swap_down(char** asSerial, char* curSerial)
{
	int curIndex;
	int numbOfDevs;
	int iResult;


	numbOfDevs = get_number_of_serials(asSerial);
	curIndex = getSerialIndex(asSerial, curSerial);
	if( curIndex<0 )
	{
		printf("... cannot swap serial position down\n");
		iResult = -1;
	}
	else if( curIndex==(numbOfDevs-1) )
	{
		printf("... no need to swap down, serial number already in last position\n");
		iResult = 0;
	}
	else
	{
		iResult = swap_serialPos(asSerial, curIndex, curIndex+1);
	}

	return iResult;
}

