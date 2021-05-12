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
/**  \file i2c_routines.c

     \brief Software I2C Functions for the FTDI 2232H Chip

i2c_routines is a simple library which provides basic i2c-functionality. Functions include sending 1 or more bytes, and reading 1 Byte / 2 Bytes.
The structure of the buffers which will be sent consists of [address - register - data]. This i2c-library can be used
for simple i2c-slaves which do not have the ability of clock stretching.

\warning clock stretching and multi-master-mode is not supported

*/

#include "i2c_routines.h"


/** \brief sends a start condition on all 16 i2c-busses.
*/
void i2c_startCond()
{
	/* Set clocklines low, datalines high */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, (!SCL) | SDA_0_OUTPUT | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT);
	/* Set clocklines high, datalines high */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SCL | SDA_0_OUTPUT | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT);
	/* Set clocklines high, datalines low */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SCL | !SDA_0_OUTPUT | !SDA_1_OUTPUT | !SDA_2_OUTPUT | !SDA_3_OUTPUT);
	/* Set clocklines low, datalines low */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, (!SCL) | !SDA_0_OUTPUT | !SDA_1_OUTPUT | !SDA_2_OUTPUT | !SDA_3_OUTPUT);
}


/** \brief sends a stop condition on all 16 i2c-busses. 
*/
void i2c_stopCond()
{
	/* Set all lines low */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, 0);
	/* Set clocklines high, datalines low */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SCL | !SDA_0_OUTPUT | !SDA_1_OUTPUT | !SDA_2_OUTPUT | !SDA_3_OUTPUT);
	/* Set clocklines high, datalines high */
	process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL);
}


/** \brief i2c-function sends the content of aucSendBuffer on all 16 i2c-busses.

ftdiA and ftdiB represent Channel A and Channel B of a ftdi device and each of these channels has 8 i2c-busses. This function
can send one byte to a 8 Bit Register of a i2c-slave. Though the name is i2c_write8, the function can send out as many bytes as wanted.
The number of bytes to be sent can be passed in the parameter ucLength.
    @param ftdiA, ftdiB  pointer to ftdi_context
    @param aucSendBuffer pointer to the buffer which contains address, register and data
    @param ucLength      sizeof aucSendbuffer in bytes

    @return    0 if succesful, errorcode if not 
        - @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
*/

int i2c_write8(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength)
{
	unsigned int uiBufferIndex = 0;
	unsigned char ucMask = 0x80;
	unsigned char ucBitnumber = 7;
	unsigned long ucDataToSend = 0;
	unsigned long ulDataToSend = 0;


	i2c_startCond(ftdiA, ftdiB);

	/* Send Adress leave Bit0 for WR Bit */
	while(ucMask!=1)
	{
		/* Begin with MSB and iterate through all elements until LSB is reached */
		ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
		ulDataToSend = ucDataToSend << 0U | ucDataToSend <<  2U| ucDataToSend <<  4U| ucDataToSend << 6U |
		               ucDataToSend << 8U | ucDataToSend << 10U| ucDataToSend << 12U| ucDataToSend <<14U |
		               ucDataToSend <<16U | ucDataToSend << 18U| ucDataToSend << 20U| ucDataToSend <<22U |
		               ucDataToSend <<24U | ucDataToSend << 26U| ucDataToSend << 28U| ucDataToSend <<30U;

		process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
		i2c_clock(ulDataToSend);

		ucMask >>= 1U;
		ucBitnumber--;
	}


	/* 0 write 1 read */
	process_pins( SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_WRITE);
	i2c_clock(SDA_WRITE);
	i2c_getAck(ftdiA, ftdiB);
	uiBufferIndex++;
	ucMask = 128;
	ucBitnumber = 7;

	/* Iterate to all elements of aucBuffer, Index 1 will be the register written to / read from, index 2+ will be data */
	while( ucLength>1 )
	{
		/* Process the byte with indexnumber uiBufferIndex - apply masks and put the byte into the buffer beginning with msb */
		while(ucMask)
		{
			ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
			ulDataToSend = ucDataToSend << 0U | ucDataToSend <<  2U| ucDataToSend <<  4U| ucDataToSend << 6U | // DA0-3
			               ucDataToSend << 8U | ucDataToSend << 10U| ucDataToSend << 12U| ucDataToSend <<14U | // DA4-7
			               ucDataToSend <<16U | ucDataToSend << 18U| ucDataToSend << 20U| ucDataToSend <<22U | // DA8-11
			               ucDataToSend <<24U | ucDataToSend << 26U| ucDataToSend << 28U| ucDataToSend <<30U;  // DA12-15

			process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
			i2c_clock(ulDataToSend);

			ucMask >>= 1;
			ucBitnumber--;
		}

		i2c_getAck(ftdiA, ftdiB);
		ucMask = 128;
		ucBitnumber = 7;
		ucLength--;
		uiBufferIndex++;
	}

	i2c_stopCond(ftdiA, ftdiB);

	return send_package_write8(ftdiA, ftdiB);
 
}


/** \brief i2c-function sends the content of aucSendBuffer on all 16 i2c-busses.

Sends the amount of bytes specified in ucLength over one of the 16 i2c-busses.
The number of the i2c-bus which shall send the data will be given in uiX,
which ranges from 0 ... 15.
    @param ftdiA, ftdiB  pointer to ftdi_context
    @param aucSendBuffer pointer to the buffer which contains address, register and data
    @param ucLength      sizeof aucSendbuffer in bytes
    @param uiX           number of i2c-bus which should send the data (0 ... 15)

    @return    0 if succesful, errorcode if not 
        - @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
*/
int i2c_write8_x(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength, unsigned int uiX)
{
	unsigned int uiBufferIndex = 0;
	unsigned char ucMask       = 0x80;
	unsigned char ucBitnumber  = 7;
	unsigned long ucDataToSend = 0;
	unsigned long ulDataToSend = 0;
	int sensorToDataline       = (int)(uiX*2);


	i2c_startCond(ftdiA, ftdiB);

	/* Send Adress leave Bit0 for WR Bit */
	while(ucMask!=1)
	{
		ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);

		/* Write to a single dataline */
		ulDataToSend = ucDataToSend << (sensorToDataline);

		process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
		i2c_clock(ulDataToSend);

		ucMask >>= 1;
		ucBitnumber--;
	}


    /* 8th bit of the first byte --> 0 write 1 read */
    process_pins( SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_WRITE);
    i2c_clock(SDA_WRITE);
    i2c_getAck(ftdiA, ftdiB);
    uiBufferIndex++;
    ucMask = 128;
    ucBitnumber = 7;

    /* Iterate to all elements of aucBuffer, Index 1 will be the register written to / read from, index 2+ will be data */
    while(ucLength > 1)
    {
       /* Process the byte with indexnumber uiBufferIndex - apply masks and put the byte into the buffer beginning with msb */
        while(ucMask)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            
			/* Write to a single dataline */
			ulDataToSend = ucDataToSend << (sensorToDataline);

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }
        // an ack bit should come here
        //i2c_fakeAck(ftdiA, ftdiB);
        i2c_getAck(ftdiA, ftdiB);
        ucMask = 128;
        ucBitnumber = 7;
        ucLength--;
        uiBufferIndex++;
    }

    i2c_stopCond(ftdiA, ftdiB);

	return send_package_write8(ftdiA, ftdiB); // 3 Acknowladges expected
;

}


/** \brief i2c-function reads the slaves connected to all 16 i2c-busses and stores the information in aucRecBuffer.

ftdiA and ftdiB represent Channel A and Channel B of a ftdi device and each of these channels has 8 i2c-busses. This function
can read one byte from an i2c-slave. 
	@param ftdiA, ftdiB  pointer to ftdi_context
	@param aucSendBuffer pointer to the buffer which contains address and register to read from
	@param ucLength		 sizeof aucSendbuffer in bytes
	@param aucRecBuffer	 buffer which will store the information read back from the slaves
	@param aucRecBuffer	 as we have 16 i2c-busses aucRecBuffer must be able to hold at least 16 bytes.
	@param ucRecLength	 sizeof aucRecBuffer in bytes
	
	@return	0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
*/

int i2c_read8(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength,
              unsigned char* aucRecBuffer, unsigned char ucRecLength)
{
    unsigned int uiBufferIndex = 0;
    unsigned char ucMask = 0x80;
    unsigned char ucBitnumber = 7;
    unsigned long ucDataToSend = 0;
    unsigned long ulDataToSend = 0;

    i2c_startCond(ftdiA, ftdiB);

        /* Send Adress - leave Bit0 for RW */
        while(ucMask!=1)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend << 28| ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

    /* 0 write 1 read */
    process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL , SDA_WRITE );
    i2c_clock(SDA_WRITE);
    i2c_getAck(ftdiA, ftdiB);

    uiBufferIndex++;
    ucMask = 128;
    ucBitnumber = 7;

    /* Iterate to all elements of aucBuffer, Index 1 will be the register written to / read from, index 0+ 2and on will be data */
    while(ucLength > 1)
    {
       /* Process the byte with indexnumber uiBufferIndex - apply masks and put the byte into the buffer beginning with msb */
        while(ucMask)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend << 28| ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }
        // an ack bit should come here
        //i2c_fakeAck(ftdiA, ftdiB);
        i2c_getAck(ftdiA, ftdiB);
        ucMask = 128;
        ucBitnumber = 7;
        ucLength--;
        uiBufferIndex++;
    }

    uiBufferIndex = 0;
    ucMask = 0x80;
    ucBitnumber = 7;
    ucDataToSend = 0;
    ulDataToSend = 0;


    /* Send a repeated start condition */
    i2c_startCond(ftdiA, ftdiB);

        /* Send Adress again with following RW Bit */
        while(ucMask!=1)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend <<28 | ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

    process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_READ);
    i2c_clock(SDA_READ);
    i2c_getAck(ftdiA, ftdiB);

    /* Now the bytes can be read back beginning from the MSB */

    int counter = 8;

    while(counter--)
    {
        i2c_clockInput(0);
        if((counter  % 8 == 0) && counter != 8) i2c_giveAck(ftdiA, ftdiB);
    }

    i2c_stopCond(ftdiA, ftdiB);

    return send_package_read8(ftdiA, ftdiB, aucRecBuffer, ucRecLength);

}


/** \brief i2c-function reads the slaves connected to all 16 i2c-busses and stores the information in an adequate buffer.

ftdiA and ftdiB represent Channel A and Channel B of a ftdi device and each of these channels has 8 i2c-busses. This function
can read 2 bytes from an i2c-slave. 
	@param ftdiA, ftdiB 	pointer to ftdi_context
	@param aucSendBuffer 	pointer to the buffer which contains slave address and register to read from
	@param ucLength		 	sizeof aucSendbuffer in bytes
	@param ausReadBuffer 	stores the information read back from the slaves
	@param ausReadBuffer 	as we have 16 i2c-busses ausReadBuffer must be able to hold at least 16 elements.
	@param ucRecLength	 	number of expected bytes to read from the slaves
	
	@return	0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
*/


int i2c_read16(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength,
              unsigned short* ausReadBuffer, unsigned char ucRecLength)
{
    unsigned int uiBufferIndex = 0;
    unsigned char ucMask = 0x80;
    unsigned char ucBitnumber = 7;
    unsigned long ucDataToSend = 0;
    unsigned long ulDataToSend = 0;

    i2c_startCond(ftdiA, ftdiB);

        /* Send Adress / RW Bit */
        while(ucMask!=1)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend <<28 | ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL , ulDataToSend );
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

    /* 0 write 1 read */
    process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL , SDA_WRITE);
    i2c_clock(SDA_WRITE);
    i2c_getAck(ftdiA, ftdiB);

    uiBufferIndex++;
    ucMask = 128;
    ucBitnumber = 7;

    /* Iterate to all elements of aucBuffer, Index 1, will be the register written to / read from, index 0+ 2and on will be data */
    while(ucLength > 1)
    {
       /* Process the byte with indexnumber uiBufferIndex - apply masks and put the byte into the buffer beginning with msb */
        while(ucMask)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend << 28| ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

        i2c_getAck(ftdiA, ftdiB);
        ucMask = 128;
        ucBitnumber = 7;
        ucLength--;
        uiBufferIndex++;
    }

    uiBufferIndex = 0;
    ucMask = 0x80;
    ucBitnumber = 7;
    ucDataToSend = 0;
    ulDataToSend = 0;


    /* Send a repeated start condition */
    i2c_startCond(ftdiA, ftdiB);

        /* Send Adress again / RW Bit */
        while(ucMask!=1)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend << 28| ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

    process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_READ );
    i2c_clock(SDA_READ);


    i2c_getAck(ftdiA, ftdiB);

    /* Now the bytes can be read back beginning from the MSB */

    int counter = 16;

    while(counter--)
    {
        i2c_clockInput(0);
        if((counter  % 8 == 0) && counter != 16) i2c_giveAck(ftdiA, ftdiB);
    }

    i2c_stopCond(ftdiA, ftdiB);


    return send_package_read16(ftdiA, ftdiB, ausReadBuffer, ucRecLength);


}

/** \brief i2c-function reads the slaves connected to all 16 i2c-busses and stores the information in adequates buffer.

ftdiA and ftdiB represent Channel A and Channel B of a ftdi device and each of these channels has 8 i2c-busses. This function
can read 4 times 2 bytes from an i2c-slave. Internally it reads one more Byte (the status register) in order to check if 
conversions have already completed. 
	@param ftdiA, ftdiB 	pointer to ftdi_context
	@param aucSendBuffer 	pointer to the buffer which contains slave address and register to read from
	@param ucLength		 	sizeof aucSendbuffer in bytes
	@param aucStatusRegister stores the content of the status register read back from the slaves 
	@param ausReadBuffer1 	stores the first 16 Bit information read back from the slaves
	@param ausReadBuffer2 	stores the second 16 Bit information read back from the slaves
	@param ausReadBuffer3 	stores the third 16 Bit information read back from the slaves
	@param ausReadBuffer4 	stores the fourth 16 Bit information read back from the slaves
	@param ucRecLength	 	number of expected bytes to read from the slaves
	
	@return	0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
*/

int i2c_read72(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, unsigned char* aucSendBuffer, unsigned char ucLength, 
				unsigned char*  aucStatusRegister,
				unsigned short* ausReadBuffer1, unsigned short* ausReadBuffer2,
				unsigned short* ausReadBuffer3, unsigned short* ausReadBuffer4, unsigned char ucRecLength)
{
    unsigned int uiBufferIndex = 0;
    unsigned char ucMask = 0x80;
    unsigned char ucBitnumber = 7;
    unsigned long ucDataToSend = 0;
    unsigned long ulDataToSend = 0;

    i2c_startCond(ftdiA, ftdiB);

        /* Send Adress / RW Bit */
        while(ucMask!=1)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend <<28 | ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL , ulDataToSend );
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

    /* 0 write 1 read */
    process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL , SDA_WRITE);
    i2c_clock(SDA_WRITE);
    i2c_getAck(ftdiA, ftdiB);

    uiBufferIndex++;
    ucMask = 128;
    ucBitnumber = 7;

    /* Iterate to all elements of aucBuffer, Index 1, will be the register written to / read from, index 0+ 2and on will be data */
    while(ucLength > 1)
    {
       /* Process the byte with indexnumber uiBufferIndex - apply masks and put the byte into the buffer beginning with msb */
        while(ucMask)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend << 28| ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

        i2c_getAck(ftdiA, ftdiB);
        ucMask = 128;
        ucBitnumber = 7;
        ucLength--;
        uiBufferIndex++;
    }

    uiBufferIndex = 0;
    ucMask = 0x80;
    ucBitnumber = 7;
    ucDataToSend = 0;
    ulDataToSend = 0;


    /* Send a repeated start condition */
    i2c_startCond(ftdiA, ftdiB);

        /* Send Adress again / RW Bit */
        while(ucMask!=1)
        {
            ucDataToSend = ((aucSendBuffer[uiBufferIndex] & ucMask)>>ucBitnumber);
            ulDataToSend = ucDataToSend << 0 | ucDataToSend << 2 | ucDataToSend << 4 | ucDataToSend << 6  // DA0-3
                         | ucDataToSend << 8 | ucDataToSend << 10| ucDataToSend << 12| ucDataToSend <<14  // DA4-7
                         | ucDataToSend <<16 | ucDataToSend << 18| ucDataToSend << 20| ucDataToSend <<22  // DA8-11
                         | ucDataToSend <<24 | ucDataToSend << 26| ucDataToSend << 28| ucDataToSend <<30; // DA12-15

            process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, ulDataToSend);
            i2c_clock(ulDataToSend);

            ucMask>>=1;
            ucBitnumber--;
        }

    process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_READ );
    i2c_clock(SDA_READ);


    i2c_getAck(ftdiA, ftdiB);

    /* Now the bytes can be read back beginning from the MSB */

    int counter = 72;

    while(counter--)
    {
        i2c_clockInput(0);
        if((counter  % 8 == 0) && counter != 72) i2c_giveAck(ftdiA, ftdiB);
    }

    i2c_stopCond(ftdiA, ftdiB);

    return send_package_read72(ftdiA, ftdiB, aucStatusRegister, ausReadBuffer1, ausReadBuffer2, ausReadBuffer3, ausReadBuffer4, ucRecLength);

}

/** \brief triggers a clock cycle on all clock lines, while sendindg out data on the data lines.
	@param ulDataToSend  	data which is going to be clocked on all lines set as output
 */
void i2c_clock(unsigned long ulDataToSend)
{
      process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SCL | ulDataToSend);
      process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, (!SCL) | ulDataToSend);
}


/* Trigger a clock cycle on the clock lines while expecting data from the slave on the data lines */

/** \brief triggers a clock cycle on the clock lines while expecting data from the slave on the data lines.

ulDataToSend is the value which is going to be written to all pins set as output, if no pin is set as output, nothing happens. All pins
which are set as input will capute their value on the negative clock edge.
	@param ulDataToSend 	data which is going to be clocked on all lines set as output
*/
void i2c_clockInput(unsigned long ulDataToSend)
{
	  process_pins(SDA_0_INPUT | SDA_1_INPUT | SDA_2_INPUT | SDA_3_INPUT | SCL, 0); //added this line 
      process_pins_databack(SDA_0_INPUT | SDA_1_INPUT | SDA_2_INPUT | SDA_3_INPUT | SCL, SCL | ulDataToSend);
      process_pins_databack(SDA_0_INPUT | SDA_1_INPUT | SDA_2_INPUT | SDA_3_INPUT | SCL, !SCL | ulDataToSend);
}

/** \brief master gives an acknowledge on all data lines. 
*/
void i2c_giveAck()
{
      process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, SDA_READ );
	  process_pins(SDA_0_OUTPUT  | SDA_1_OUTPUT | SDA_2_OUTPUT | SDA_3_OUTPUT | SCL, 0);
      i2c_clock(0);
}



/** \brief clocks the acknowledge bit given by the slave. 
	@param ulDataToSend		data which is going to be clocked on lines set as output
*/
void i2c_clock_forACK(unsigned long ulDataToSend)
{
      process_pins_databack(SDA_0_INPUT | SDA_1_INPUT | SDA_2_INPUT | SDA_3_INPUT | SCL, SCL | ulDataToSend);
      process_pins_databack(SDA_0_INPUT | SDA_1_INPUT | SDA_2_INPUT | SDA_3_INPUT | SCL, !SCL | ulDataToSend);

}

/** \brief expects and clocks an acknowledge bit given by the slave.
*/
void i2c_getAck()
{

    process_pins(SDA_0_INPUT | SDA_1_INPUT | SDA_2_INPUT | SDA_3_INPUT | SCL, 0);
    i2c_clock_forACK(0);
}
