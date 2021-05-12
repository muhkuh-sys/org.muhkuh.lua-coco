/***************************************************************************
 *   Copyright (C) 2015 by Subhan Waizi                             		   *
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
 
 /** \file tcs3472.h

	 \brief Library to operate with 16 AMS/TAOS Color Sensors (TCS3472) (header)
	 
tcs3472 provides functionality to address the color sensor tcs3472, an RGBC sensor manufactured by TAOS. Functions
include identifying, turning on/off, setting integration time and gain, reading colors and checking read colors
for their validity. The library is written to address 16 color sensors at once. In case any of the functions fail,
an return code will be returned which can be used to determine which specific sensor(s) failed.
 
 */
 
#include "i2c_routines.h"


/** i2c-address of the tcs3472 color sensor */
#define TCS_ADDRESS 0x29
/** Enable register */
#define TCS3472_ENABLE_REG 0x00
/** integration time register */
#define TCS3472_ATIME_REG 0x01
/** wait time register */
#define TCS3472_WTIME_REG 0x03
/** RGBC interrupt low threshold low byte */
#define TCS3472_AILTL_REG 0x04
/** RGBC interrupt low threshold high byte */
#define TCS3472_AILTH_REG 0x05
/** RGBC interrupt high threshold low byte */
#define TCS3472_AIHTL_REG 0x06
/** RGBC interrupt high threshold high byte */
#define TCS3472_AIHTH_REG 0x07
/** interrupt persistence filters */
#define TCS3472_PERS_REG 0x0C
/** configuration register */
#define TCS3472_CONFIG_REG 0x0D
/** gain control register */
#define TCS3472_CONTROL_REG 0x0F
/** device ID register */
#define TCS3472_ID_REG 0x12
/** device status register */
#define TCS3472_STATUS_REG 0x13
/** clear ADC low data register */
#define TCS3472_CDATA_REG 0x14
/** clear ADC high data register */
#define TCS3472_CDATAH_REG 0x15
/** red ADC low data register */
#define TCS3472_RDATA_REG 0x16
/** red ADC high data register */
#define TCS3472_RDATAH_REG 0x17
/** green ADC low data register */
#define TCS3472_GDATA_REG 0x18
/** green ADC high data register */
#define TCS3472_GDATAH_REG 0x19
/** blue ADC low data register */
#define TCS3472_BDATA_REG 0x1A
/** blue ADC high data register */
#define TCS3472_BDATAH_REG 0x1B

/** command bit */
#define TCS3472_COMMAND_BIT 0x80
/** autoincrement bit */
#define TCS3472_AUTOINCR_BIT 0x20
/** special bit */
#define TCS3472_SPECIAL_BIT 0x60
/** interrupt clear bit */
#define TCS3472_INTCLEAR_BIT 0x06

/** RGBC interrupt enable bit */
#define TCS3472_AIEN_BIT 0x10
/** device wait enable bit */
#define TCS3472_WEN_BIT 0x08
/** RGBC enable bit */
#define TCS3472_AEN_BIT 0x02
/** power on bit - activates oscillator */
#define TCS3472_PON_BIT 0x01

/** wait long bit */
#define TCS3472_WLONG_BIT 0x02

/** ID register value for tcs347215 */
#define TCS3472_1_5_VALUE 0x14
/** ID register value for tcs347237 */
#define TCS3472_3_7_VALUE 0x1D

/** RGBC clear channel interrupt bit */
#define TCS3472_AINT_BIT 0x10
/** RGBC valid bit - indicates that RGBC have completed an integration cycle */
#define TCS3472_AVALID_BIT 0x01

/** \brief contains the gain setting commands for the sensor

Gain setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to 
the sensor's datasheet for further information about gain setting.

*/
typedef enum
{
	/** 1X  Gain */
    TCS3472_GAIN_1X = 0x00, 	
	/** 4X  Gain */
    TCS3472_GAIN_4X = 0x01,		
	/** 16X Gain */
    TCS3472_GAIN_16X = 0x02,
	/** 60X Gain */
    TCS3472_GAIN_60X = 0x03,
	/** ERROR	 */
    TCS3472_GAIN_ERROR = 0xFF
}
tcs3472Gain_t;


/** \brief contains the integration time setting commands for the sensor

Integration time setting can be used to capture both bright LEDs and dark LEDs. Whereas dark LEDs require a longer integration
time, integration time for bright LEDs can be low. Refer to the sensor's datasheet for further information about 
the integration time setting. Calculation of the register content for a specific integration time can be found
in the datasheet as well. This enum contains 6 different integration time settings, though there's any setting possible
starting from 2.4ms to 700ms (in 2.4ms steps).

*/
typedef enum
{
	/** command for 2.4 milliseconds integration time */
    TCS3472_INTEGRATION_2_4ms 		 = 0xFF, 
	/** command for 24	 milliseconds integration time */
    TCS3472_INTEGRATION_24ms 		 = 0xF6,
	/** command for 100 milliseconds integration time */
    TCS3472_INTEGRATION_100ms 		 = 0xD6,
	/** command for 154 milliseconds integration time */
    TCS3472_INTEGRATION_154ms 		 = 0xC0, 
	/** command for 200 milliseconds integration time */
    TCS3472_INTEGRATION_200ms		 = 0xAD, 
	/** command for 700 milliseconds integration time */
    TCS3472_INTEGRATION_700ms        = 0x00 
}
tcs3472Integration_t;

/** \brief commands for different color readings (Red, Green, Blue, Clear)
*/
typedef enum
{
	/** Command for Red   Color Reading */
	RED       = 0x00, 
	 /** Command for Green Color Reading */
	GREEN     = 0x01, 
	/** Command for Blue  Color Reading */
	BLUE      = 0x02,
	/** Command for Clear Color Reading */
	CLEAR     = 0x03  
}
 tcs_color_t;
 
int tcs_identify			(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucReadbuffer);
int tcs_waitForData			(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB);
int tcs_conversions_complete(unsigned char* aucStatusRegister);
int tcs_readColor			(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned short* ausColourArray, tcs_color_t color);
int tcs_sleep				(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB);
int tcs_wakeUp				(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB);
int tcs_ON					(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB);
int tcs_exClear				(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned short* ausClear, unsigned char* aucIntegrationtime);
int tcs_clearInt			(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB);
int getGainDivisor			(tcs3472Gain_t gain);
int tcs_getIntegrationtime	 (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucIntegrationtime);
int tcs_getGain				 (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucGainSettings);
int tcs_setIntegrationTime   (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Integration_t integration);
int tcs_setIntegrationTime_x (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Integration_t integration, unsigned int uiX);
int tcs_setGain  		     (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Gain_t gain);
int tcs_setGain_x 			 (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Gain_t gain, unsigned int uiX); 
int tcs_readColors 		     (struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned short* ausClear, unsigned short* ausRed,
											 unsigned short* ausGreen, unsigned short* ausBlue);
void tcs_calculate_CCT_Lux	(unsigned char* aucGain, unsigned char* aucIntegrationtime, unsigned short* ausClear, unsigned short* ausRed,
											 unsigned short* ausGreen, unsigned short* ausBlue, unsigned short* CCT, float* afLUX);
									
