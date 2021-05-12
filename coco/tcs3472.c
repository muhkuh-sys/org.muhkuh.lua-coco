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

 /**  \file tcs3472.c

	 \brief Library to operate with 16 AMS/TAOS Color Sensors (TCS3472)
	 
tcs3472 provides functionality to address the color sensor tcs3472, an RGBC sensor manufactured by TAOS. Functions
include identifying, turning on/off, setting integration time and gain, reading colors and checking colors
for their validity. The library intends to address 16 color sensors at once. In case any of the functions fail,
an return code will be returned which can be used to determine which specific sensor(s) failed. Most of the functions have
2 pointers to ftdi contexts as parameters. Each pointer represents a channel of the ftdi 2232h chip, thus controls 8 sensors.
 
 */
 
#include "tcs3472.h"



/** \brief reads the ID-Register of 16 sensors and compares the values to expected values.

Expected ID for the TCS3472 is 0x44, the expected ID for the TCS3471 is 0x14 
	@param ftdiA, ftdiB 	pointer to ftdi_context
	@param aucReadbuffer	stores the ID-values read back from the 16 sensors, must be able to hold 16 elements
	
	@retval 0  Succesful 
	@retval >0 Failed to identify one or more sensors, 
			   if the return code is 0b0000000000101100 for example, identification failed for sensor 3, sensor 4 and sensor 6 
	@retval <0 USB or i2c errors occured, check return value for further information 		   
	*/
	
int tcs_identify(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucReadbuffer)
{
    unsigned int uiErrorcounter = 0;
    unsigned int uiSuccesscounter = 0;
	int usErrorMask = 0;
	int iRetval;
    int i = 0;

    unsigned char aucTempbuffer[2] = {(TCS_ADDRESS<<1), TCS3472_ID_REG | TCS3472_COMMAND_BIT};
    unsigned char aucErrorbuffer[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        if((iRetval = i2c_read8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucReadbuffer, sizeof(aucReadbuffer))) < 0) return iRetval;
		
            for(i = 0; i<=15; i++)
            {
				/* 0x14 = ID for tcs3472        0x44 = ID for tcs3471 */
               if(aucReadbuffer[i] != (0x14) && aucReadbuffer[i] != 0x44)
               {
                    aucErrorbuffer[i] = i+1;
                    uiErrorcounter ++;
					usErrorMask |= (1<<i);
               }

               else uiSuccesscounter ++;

               if(uiSuccesscounter == 16)
               {       
                   return 0; // SUCCESFULL IDENTFICATION OF ALL SENSORS
               }
            }
			
        printf("Identification errors for following Sensors ...\n");
        for(i = 0; i<16; i++)
        {
            if(aucErrorbuffer[i] != 0xFF) printf("%d ", aucErrorbuffer[i]);
        }
        printf("\n");
        
		return usErrorMask;
		
}

   
/** \brief turns 16 tcs3472 sensors on, releasing them from their sleep state.

Function wakes 16 sensors on in case they were put to sleep before. If sensors are already active this function has
no effect. 
	@param ftdiA, ftdiB 	pointer to ftdi_context
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 		
	*/
	
int tcs_ON(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB)
{
   unsigned char aucTempbuffer[3] = {(TCS_ADDRESS<<1), TCS3472_ENABLE_REG | TCS3472_COMMAND_BIT, TCS3472_AIEN_BIT | TCS3472_AEN_BIT
                                    | TCS3472_PON_BIT };
   return i2c_write8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer));
}

/** \brief sets the integration time of 16 sensors at once.

Function sets the integration time of 16 sensors. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a longer integration time, the integration time for bright LEDs can be low. Refer to 
the sensor's datasheet for calculating the content of the integration time register. A few common values have already
been calculated and saved in enum tcs3472Integration_t.
	@param ftdiA, ftdiB 		pointer to ftdi_context
	@param uiIntegrationtime	integration time to be sent to the sensors
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 		
	*/
	
int tcs_setIntegrationTime(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Integration_t uiIntegrationtime)
{
    unsigned char aucTempbuffer[3] = {(TCS_ADDRESS<<1), TCS3472_ATIME_REG | TCS3472_COMMAND_BIT, uiIntegrationtime};
    return i2c_write8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer));
}

/** \brief sets the integration time of one sensor.

Function sets the integration time of one sensor. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a longer integration time, the integration time for bright LEDs can be low. Refer to 
the sensor's datasheet for calculating the content of the integration time register. A few common values have already
been calculated and saved in enum tcs3472Integration_t.
	@param ftdiA, ftdiB 		pointer to ftdi_context
	@param uiIntegrationtime	integration time to be sent to the sensor
	@param uiX					sensor which will get the new integration time ( 0 ... 15 )
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 		
*/

int tcs_setIntegrationTime_x(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Integration_t uiIntegrationtime, unsigned int uiX)
{
    unsigned char aucTempbuffer[3] = {(TCS_ADDRESS<<1), TCS3472_ATIME_REG | TCS3472_COMMAND_BIT, uiIntegrationtime};
    return i2c_write8_x(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), uiX);
}

/** \brief sets the gain of 16 sensors.

Function sets the gain of 16 sensors. This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to 
the sensor's datasheet for further information about gain.
	@param ftdiA, ftdiB 		pointer to ftdi_context
	@param gain					gain to be sent to the sensors
	
	@retval 0  Succesful  
	@retval <0 USB or i2c errors occured, check return value for further information 		
*/
int tcs_setGain(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Gain_t gain)
{
    unsigned char aucTempbuffer[3] = {(TCS_ADDRESS<<1), TCS3472_CONTROL_REG | TCS3472_COMMAND_BIT, gain};
    return i2c_write8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer));
}

/** \brief sets the gain of one sensor.

Function sets the gain of 1 sensors This setting can be used to capture both bright LEDs and dark
LEDs. Whereas dark LEDs require a greater gain factor, gain factor for bright LEDs can be low. Refer to 
the sensor's datasheet for further information about gain.
	@param ftdiA, ftdiB 		pointer to ftdi_context
	@param gain					gain to be sent to the sensor
	@param uiX					sensor which will get the new gain ( 0 ... 15 )
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 		
*/

int tcs_setGain_x(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, tcs3472Gain_t gain, unsigned int uiX)
{
    unsigned char aucTempbuffer[3] = {(TCS_ADDRESS<<1), TCS3472_CONTROL_REG | TCS3472_COMMAND_BIT, gain};
    return i2c_write8_x(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), uiX);
}


/** \brief checks if the ADCs for color measurement have already completed.

Function checks if the sensors have already completed a color measuremend. In case the measurements are completed the ADCs can be safely
read and the datasets are valid. This can be checked by reading a special register of the sensor, the status register. 
If TCS3472_AVALID_BIT is set in this register, the ADCs have completed color measurements. If measurements are not completed, the return 
code can be used to determine which of the 16 sensor(s) failed.
	@param ftdiA, ftdiB 	pointer to ftdi_context
	
	@retval 0  Succesful 
	@retval >0 One or more sensors have not completed the conversion cycle yet, 
			   if the return code is 0b0000000000101100 for example, we have uncompleted conversions for sensor 3, sensor 4 and sensor 6 
	@retval <0 USB or i2c errors occured, check return value for further information 		
	*/
int tcs_waitForData(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB)
{
    unsigned int uiErrorcounter = 0;
    unsigned int uiSuccesscounter = 0;
	
	int usErrorMask = 0;
    int i = 0;

    unsigned char aucTempbuffer[2] = {(TCS_ADDRESS<<1), TCS3472_STATUS_REG | TCS3472_COMMAND_BIT};
    unsigned char aucReadbuffer[16];
	unsigned char aucErrorbuffer[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

        i2c_read8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucReadbuffer, sizeof(aucReadbuffer));

            for(i = 0; i<=15; i++)
            {
               if((aucReadbuffer[i]&TCS3472_AVALID_BIT) != TCS3472_AVALID_BIT)
               {
                    aucErrorbuffer[i] = i+1;
					usErrorMask  |= (1<<i);
                    uiErrorcounter ++;
               }
               else uiSuccesscounter ++;

               if(uiSuccesscounter == 16)
               {
                   //printf("Conversions complete.\n");
                   return 0; 
               }
            }

        printf("Incomplete conversions for following Sensors ...\n");
        for(i = 0; i<16; i++)
        {
            if(aucErrorbuffer[i] != 0xFF) printf("%d ", aucErrorbuffer[i]);
        }
        printf("\n");
       			
		return usErrorMask;
		
}

/** \brief checks if the ADCs for color measurement have already completed. Takes the status register as parameter.

Function checks if the sensors have already completed a color measuremend. In case the measurements are completed the ADCs can be safely
read and used for further calculations. This can be checked by reading a special register of the sensor, the status register. 
If TCS3472_AVALID_BIT is set in this register, the ADCs have completed color measurements. If measurements are not completed, the return 
code can be used to determine which of the 16 sensor(s) failed.
	@param aucStatusRegister 	holds the values of the status register for all 16 sensors 
	
	@retval 0  Succesful 
	@retval >0 One or more sensors have not completed the conversion cycle yet, 
			   if the return code is 0b0000000000101100 for example, we have uncompleted conversions for sensor 3, sensor 4 and sensor 6 
	*/
	
int tcs_conversions_complete(unsigned char* aucStatusRegister)
{
    unsigned int uiErrorcounter = 0;
    unsigned int uiSuccesscounter = 0;
	
	int usErrorMask = 0;
    int i = 0;

	unsigned char aucErrorbuffer[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

            for(i = 0; i<=15; i++)
            {
               if((aucStatusRegister[i]&TCS3472_AVALID_BIT) != TCS3472_AVALID_BIT)
               {
                    aucErrorbuffer[i] = i+1;
					usErrorMask  |= (1<<i);
                    uiErrorcounter ++;
               }
               else uiSuccesscounter ++;

               if(uiSuccesscounter == 16)
               {
                   //printf("Conversions complete.\n");
                   return 0; 
               }
            }

        printf("Incomplete conversions for following Sensors ...\n");
        for(i = 0; i<16; i++)
        {
            if(aucErrorbuffer[i] != 0xFF) printf("%d ", aucErrorbuffer[i]);
        }
        printf("\n");
       			
		return usErrorMask;
		
}

/** \brief reads back 4 color sets of 16 sensors - Red / Green / Blue / Clear.

Function reads 16-Bit color values of 16 sensors. The color will be specified by the input parameter tcs_color_t color. 
	@param ftdiA, ftdiB 	pointer to ftdi_context
	@param ausColorArray	will contain color value read back from 16 sensors
	@param color			specifies the color to be read from the sensor (red, green, blue, clear)
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 	
	*/
int tcs_readColor(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned short* ausColorArray, tcs_color_t color)
{
    unsigned char aucTempbuffer[2] = {(TCS_ADDRESS<<1), TCS3472_AUTOINCR_BIT | TCS3472_COMMAND_BIT};

    switch(color)
    {
        case RED:
            aucTempbuffer[1] |= TCS3472_RDATA_REG;
            break;
        case GREEN:
            aucTempbuffer[1] |= TCS3472_GDATA_REG;
            break;
        case BLUE:
            aucTempbuffer[1] |= TCS3472_BDATA_REG;
            break;
        case CLEAR:
            aucTempbuffer[1] |= TCS3472_CDATA_REG;
            break;
        default:
            printf("Unknown color ... \n");
            break;
    }

    return i2c_read16(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), ausColorArray, 16);

}

/** \brief reads back 4 colours and the content of the status register in one i2c command.

This function will read out the contents of the color registers of tcs3472 as words (2 Bytes per colour) and the content
of the status register as one byte. The status register can be used in order to determine if color conversions
had already completed.
	@param ftdiA, ftdiB 	pointer to ftdi_context
	@param ausClear      	will contain color value read back from 16 sensors
	@param ausRed        	will contain color value read back from 16 sensors
	@param ausGreen      	will contain color value read back from 16 sensors
	@param ausBlue       	will contain color value read back from 16 sensors
	
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 
	@retval >0 One or more sensors have not completed the conversion cycle yet, 
			   if the return code is 0b0000000000101100 for example, we have uncompleted conversions for sensor 3, sensor 4 and sensor 6 
	*/
int tcs_readColors(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned short* ausClear, unsigned short* ausRed,
					unsigned short* ausGreen, unsigned short* ausBlue)
{
	int iRetval;
	/* Begin with the status registere, the autoincrement bit effects the reading of 9 bytes 
	these 9 bytes consist of the status register (1 byte) and 4 words, one for each colour */
	unsigned char aucTempbuffer[2] = {(TCS_ADDRESS<<1), TCS3472_AUTOINCR_BIT | TCS3472_COMMAND_BIT | TCS3472_STATUS_REG};												 
	unsigned char aucStatusRegister[16];
	
	if((iRetval = i2c_read72(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucStatusRegister, ausClear, ausRed, ausGreen, ausBlue, 16)) < 0)
	{
		/* Fatal error has occured */
		return iRetval;
	}
	
	/* Now check if conversions had already completed - 0 if so */
	return tcs_conversions_complete(aucStatusRegister);
	
}


/** \brief sends 16 sensors to sleep.

Function sends 16 color sensors to sleep state.
	@param ftdiA, ftdiB 	pointer to ftdi_context
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 	
	*/
int tcs_sleep(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB)
{
	int iRetval;
	
    unsigned char aucReadbuffer[16];
    unsigned char aucTempbuffer[2]  = {(TCS_ADDRESS<<1), TCS3472_COMMAND_BIT | TCS3472_ENABLE_REG};
    if((iRetval = i2c_read8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucReadbuffer, sizeof(aucReadbuffer)))<0) return iRetval;
    unsigned char aucTempbuffer2[3] = {(TCS_ADDRESS<<1), TCS3472_COMMAND_BIT | TCS3472_ENABLE_REG,  aucReadbuffer[0] & ~(TCS3472_PON_BIT | TCS3472_AEN_BIT)};
    if((iRetval = i2c_write8(ftdiA, ftdiB,  aucTempbuffer2, sizeof(aucTempbuffer2)))<0) return iRetval;

    return 0;
}

/** \brief wakes up 16 color sensors.

Function wakes 16 color sensors from sleep state.
	@param ftdiA, ftdiB 	pointer to ftdi_context
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 	
	*/
int tcs_wakeUp(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB)
{
	int iRetval;
	
    unsigned char aucReadbuffer[16];
    unsigned char aucTempbuffer[2]  = {(TCS_ADDRESS<<1), TCS3472_COMMAND_BIT | TCS3472_ENABLE_REG};
    if((iRetval = i2c_read8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucReadbuffer, sizeof(aucReadbuffer)))<0) return iRetval;
    unsigned char aucTempbuffer2[3] = {(TCS_ADDRESS<<1), TCS3472_COMMAND_BIT | TCS3472_ENABLE_REG,  aucReadbuffer[0] | TCS3472_PON_BIT};
    if((iRetval = i2c_write8(ftdiA, ftdiB,  aucTempbuffer2, sizeof(aucTempbuffer2)))<0) iRetval;

    return 0;
}

/** \brief checks the clear color read back from the sensors have exceeded maximum clear.

Functions checks if the clear color read back from 16 sensors have exceeded a maximum clear level and are thus invalid.
In case this maximum clear level is exceeded, one should consider lowering gain and/or integration time.
If clear levels have been exceeded, the return code can be used to determine which of the 16 sensor(s) failed.
	@param ftdiA, ftdiB 		pointer to ftdi_context
	@param ausClear				clear values of the 16 sensors which will be checked for maximum clear level exceedings
	@param aucIntegrationtime	current integration time setting of the 16 sensors - needed to check if maximum clear level has been exceeded
	
	@retval 0  Succesful 
	@retval >0 One or more sensors got saturated as they have reached the maximum amount in the clear data register, 
			   if the return code is 0b0000000000101100 for example, sensor 3, sensor 4 and sensor 6 got saturated 
	*/	 
int tcs_exClear(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned short* ausClear, unsigned char* aucIntegrationtime)
{
    int i, iFlag;
	iFlag = 0;
    unsigned int uiSuccesscounter = 0;
    unsigned char aucErrorbuffer[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	int usErrorMask = 0;
	
    for(i=0; i<16; i++)
    {

        switch(aucIntegrationtime[i])
        {
            case TCS3472_INTEGRATION_2_4ms:
                if(ausClear[i] >= 1024)
                {
                    aucErrorbuffer[i] = i+1;
					usErrorMask |= (1<<i);
                }
                else uiSuccesscounter++;
                break;

            case TCS3472_INTEGRATION_24ms:
                if(ausClear[i] >= 10240)
                {
                    aucErrorbuffer[i] = i+1;
					usErrorMask |= (1<<i);
                }
                else uiSuccesscounter++;
                break;

            case TCS3472_INTEGRATION_100ms:
                if(ausClear[i] >= 43008)
                {
                    aucErrorbuffer[i] = i+1;
					usErrorMask |= (1<<i);
                }
                else uiSuccesscounter++;
                break;

            case TCS3472_INTEGRATION_154ms:
                if(ausClear[i] >= 65535)
                {
                    aucErrorbuffer[i] = i+1;
					usErrorMask |= (1<<i);
                }
                else uiSuccesscounter++;
                break;

            case TCS3472_INTEGRATION_200ms:
                if(ausClear[i] >= 65535)
                {
                    aucErrorbuffer[i] = i+1;
					usErrorMask |= (1<<i);
                }
                else uiSuccesscounter++;
                break;

            case TCS3472_INTEGRATION_700ms:
                if(ausClear[i] >= 65535)
                {
                    aucErrorbuffer[i] = i+1;
					usErrorMask |= (1<<i);
                }               
			    else uiSuccesscounter++;
                break;
			default: 
				iFlag = 1;
        }

        if(uiSuccesscounter ==16)
        {
            //printf("All gain settings ok.\n");
            return 0;
        }

    }

    iFlag == 0 ? printf("Turn down gain for following Sensors ...\n") : printf("Integration Time not found.\n");
    for(i = 0; i<16; i++)
    {
       if(aucErrorbuffer[i] != 0xFF) printf("%d ", aucErrorbuffer[i]);
    }
    printf("\n");		
		
	return usErrorMask;
		
}


/* this function will not be used here anymore as it is implemented in the lua scripts */

/** \brief calculates Illuminance in unit LUX.

Functions calculates the illuminance level of 16 sensors. In photometry, illuminance is the total luminous flux incident on a surface, per unit area.
It is a measure of how much the incident light illuminates the surface, wavelength-weighted by the luminosity function to correlate with
human brightness perception.
Color temperature is related to the color with which a piece of metal glows when heated to a
particular temperature and is typically stated in terms of degrees Kelvin. The color temperature goes
from red at lower temperatures to blue at higher temperatures. Refer to Design Note 40 from AMS / Taos for further information about the calculations.
	@param aucGain 				current gain setting of 16 sensors
	@param aucIntegrationtime	current integration time setting of 16 sensors
	@param ausClear				contains clear value of 16 sensors
	@param ausRed				contains red value of 16 sensors
	@param ausGreen				contains green value of 16 sensors
	@param ausBlue				contains blue value of 16 sensors
	@param afLUX				will store the calculated LUX values of 16 sensors 
	@param CCT					will store the calculated CCT values of 16 sensors

*/
void tcs_calculate_CCT_Lux	(unsigned char* aucGain, unsigned char* aucIntegrationtime, unsigned short* ausClear, unsigned short* ausRed,
									 unsigned short* ausGreen, unsigned short* ausBlue, unsigned short* CCT, float* afLUX)
{
	
/* For some applications, the IR content is negligible and can be ignored. An example would be
measuring the color temperature of an LED backlight. However, in applications that need to measure
ambient light levels, incandescent lights and sunlight have strong IR contents. Most IR filters are
imperfect and allow small amounts of residual IR to pass through. For IR intensive light sources,
additional calculations are needed to remove the residual IR component. As the project's aim is to measure
LEDs, and LEDs colors commonly have very low / nearly no IR content, the IR Rejection part of the algorithm
described in DN40 can be neglected for LUX calculations, but should be considered for CCT calculations */

	float fTempRed;
	float fTempGreen;
	float fTempBlue;
	float fTempClear;
	float fIRContent;
	
	float CPL = 0;// counts per LUX
	float GA = 1.0; // device attenuation
	
	/* some magic numbers, retrieved from DN40 - Lux and CCT Calculations */
	float R_Coef = 0.136;
	float G_Coef = 1.0;
	float B_Coef = -0.444;
	
	unsigned int CT_Coef = 3810;
	unsigned int CT_Offset = 1391;
	
	float device_factor = 310.0;
	
	int i;

	/* Calculate the IR value so it can be removed from the colors */
	
	for(i=0; i<16; i++)
	{	
		fTempRed   = (float)ausRed[i];
		fTempGreen = (float)ausGreen[i];
		fTempBlue  = (float)ausBlue[i];
		fTempClear = (float)ausClear[i];
		
		fIRContent = (fTempRed + fTempGreen + fTempBlue - fTempClear) / 2;
		
		fTempRed   -= fIRContent;		// R' (removed IR content from R)
	    fTempGreen -= fIRContent;		// G' (removed IR content from G)
	    fTempBlue  -= fIRContent;		// B' (removed IR content from B)
		
		CPL = ((256 - (float)aucIntegrationtime[i]) * 2.4) * getGainDivisor(aucGain[i]) / device_factor;
		
		afLUX[i] = (R_Coef*fTempRed + G_Coef*fTempGreen + B_Coef*fTempBlue) / CPL;
		afLUX[i] = (afLUX[i] < 0) ? afLUX[i]*(-1) : afLUX[i];
		CCT[i]  = CT_Coef * (fTempBlue/fTempRed) + CT_Offset;
	}
	    
}



/** \brief clears the interrupt flag of 16 sensors.

Functions clears the interrupt flag of 16 sensors. The flag will be set if a color value has been exceeded, or the value
read back from the sensor has fallen below a certain color value. Both settings can be set up in the sensors' registers.
	@param ftdiA, ftdiB 	pointer to ftdi_context
	
	@return  0 : everything OK - conversions complete
	@return  1 : i2c-functions failed
	*/	 
int tcs_clearInt(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB)
{
    unsigned char aucTempbuffer[2]  = {(TCS_ADDRESS<<1), TCS3472_COMMAND_BIT | TCS3472_SPECIAL_BIT | TCS3472_INTCLEAR_BIT};

    return i2c_write8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer));
}


/** \brief reads the current gain setting of 16 sensors and stores them in an adequate buffer.

The function reads back the gain settings of 16 sensors. Refer to sensors' datasheet for further information about
gain settings.
	@param ftdiA, ftdiB 	pointer to ftdi_context
	@param aucGainSettings	pointer to buffer which will contain the gain settings of the 16 sensors
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 	 
*/
int tcs_getGain(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucGainSettings)
{
	unsigned char aucTempbuffer[2] = {(TCS_ADDRESS<<1), TCS3472_CONTROL_REG | TCS3472_COMMAND_BIT};
	return i2c_read8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucGainSettings, sizeof(aucGainSettings));
}

/** \brief reads the current integration time setting of 16 sensors and stores them in an adequate buffer.

The function reads back the integration time of 16 sensors. Refer to sensors' datasheet for further information about
integration time settings.
	@param ftdiA, ftdiB 		pointer to ftdi_context
	@param aucIntegrationtime	pointer to buffer which will store the integration time settings of the 16 sensors
	
	@retval 0  Succesful 
	@retval <0 USB or i2c errors occured, check return value for further information 	
*/
int tcs_getIntegrationtime(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucIntegrationtime)
{
	unsigned char aucTempbuffer[2] = {(TCS_ADDRESS<<1), TCS3472_ATIME_REG | TCS3472_COMMAND_BIT};	
	return i2c_read8(ftdiA, ftdiB, aucTempbuffer, sizeof(aucTempbuffer), aucIntegrationtime, sizeof(aucIntegrationtime));
}

/** \brief returns a divisor which corresponds to a specific gain setting.

As the actual gain value which will be written into the sensors' register does not match its enum name (i.e. TCS3472_GAIN_1X
has the value 0) this function is needed to determine the divisor that corresponds to a specific gain setting (i.e. returns 1 for
TCS3472_GAIN_1X)
	@param gain		enum value for gain setting
	@return			gain divisor value which corresponds to the enum value of the gain setting
*/
int getGainDivisor(tcs3472Gain_t gain)
{
		switch(gain)
		{
			case TCS3472_GAIN_1X:
				return 1;
				break;
			case TCS3472_GAIN_4X:
				return 4;
				break;
			case TCS3472_GAIN_16X:
				return 16;
				break;
			case TCS3472_GAIN_60X:
				return 60;
				break;
			default:
				printf("Can't calculate a gain divisor!\n");
				return 1;
				break;
		}
}
