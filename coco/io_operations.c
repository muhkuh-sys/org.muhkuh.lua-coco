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

/** \file io_operations.c
	\brief provides functions to manipulate ftdi 2232h's I/O-Pins
	
Once the ftdi2232h is set into BITMODE_MPSSE simple USB commands can be sent to it in order to manipulate its input and output pins.
Special commands (for example found in AN_108) can be used to set the 32 GPIO Pins of the ftdi device as either input or output, and once done,
values can be assigned to the output pins and data can be read back from the input pins. These functions will be used to provide software i2c functionality.
*/


/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE ----------------------->

  For now we wait between a usb write to and read from ftdi chip command, with a Sleep(1) function. This works
  well, but cannot guarantee that expected data will really arrive after that one second ... this should be changed 
  in future code 

  NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE -----------------------> */

/* Info - How the code works */
/* Each process pins databack functions reads two bytes, namely the lowbyte and the highbyte of each channel,
thus 2 of these commands result in 4 Bytes read back ... as we only want to evaluate the value on the bus on negative
clock edge we read a byte (lowbyte) increment the bytenumber in the rec buffer by one, then read the highbyte. then if we icremented by one
we would read back the value on positive clock cycle of lowbyte, increment by another one would get the value on positive clock edge
of highbyte and increment by another one would result in reading the next lowbyte value on the negative clock edge ... 
thus: read lowbyte - neg clock cycle, increment bytenumber , read highbyte - neg clockcycle, increment bytenumber by 3, read next lowbyte and so on 

readIndexA and readIndexB mark the number of bytes we expect. As every i2c-read command expects 12 Bytes acknowledge 
+ x times 4 Bytes Data (x marks the number of data bytes expected from the slave ... x = 8 for i2c_read8) + 2 bytes status information of the ftdi.
Thus we always expect to read back readIndexA+2 and readIndexB+2 bytes with a usb command. If we read less, that means that not all
command sent to the ftdi chip had been processed yet or that the usb package has been sent to it too late. 

*/

 
#include "io_operations.h"
#include "sleep_ms.h"


/** global arrayindex for Channel A, it marks the current index of aucBufferA */
unsigned int indexA = 0;
/** global arrayindex for Channel B, it marks the current index of aucBufferB */
unsigned int indexB = 0; 
/** global readIndex for Channel A, incremented everytime a byte is expected to be read back from Channel A*/
unsigned int readIndexA = 0; 
/** global readIndex for Channel B, incremented everytime a byte is expected to be read back from Channel B*/
unsigned int readIndexB = 0;
/** global Buffer stores the commands for channel A */
unsigned char aucBufferA[4096]; 
/** global Buffer stores the commands for channel B */
unsigned char aucBufferB[4096];


/** \brief writes a value to the ftdi 2232h output pins.
	@param[in] 		ftdiA, ftdiB pointer to a ftdi_context
	@param[in] 		ulOutput	 a 32 Bit value to be written to the ftdi pins
	@param[in] 		ulOutput     Bit0 will be assigned to AD0, Bit31 to BC7

	@return 		>0 : number of bytes written to the chip
	@return			<0 : USB functions failed 
*/

int writeOutputs(struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, const unsigned long ulOutput)
{

    int uiWritten;
    unsigned char aucBuffer[6];

    aucBuffer[0] = W_LOWBYTE; // lowbyte ist ad //highbyte ist ac
    aucBuffer[1] = (unsigned char)(ulOutput&MASK_ALOW);
    aucBuffer[2] = OUTPUT; //output
    aucBuffer[3] = W_HIGHBYTE;
    aucBuffer[4] = (unsigned char)((ulOutput&MASK_AHIGH)>>8);
    aucBuffer[5] = OUTPUT; //output



    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->in_ep, aucBuffer, sizeof(aucBuffer), &uiWritten, ftdiA->usb_write_timeout)<0)
    {
        printf("usb bulk write failed");
        return -1;
    }

    printf("Lowbyte (AD) 0x%02x   Highbyte (AC) 0x%02x on interface %s\n ", aucBuffer[1], aucBuffer[4], ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));

    aucBuffer[0] = W_LOWBYTE;
    aucBuffer[1] = (unsigned char)((ulOutput&MASK_BLOW)>>16);
    aucBuffer[2] = OUTPUT; //output
    aucBuffer[3] = W_HIGHBYTE;
    aucBuffer[4] = (unsigned char)((ulOutput&MASK_BHIGH)>>24);
    aucBuffer[5] = OUTPUT; //output

    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->in_ep, aucBuffer, sizeof(aucBuffer), &uiWritten, ftdiB->usb_write_timeout)<0)
    {
        printf("usb bulk write failed\n");
        return -1;
    }
    printf("Lowbyte (BD) 0x%02x   Highbyte (BC) 0x%02x on interface %s\n ", aucBuffer[1], aucBuffer[4], ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));



    return uiWritten;
}

/** \brief reads the input pins of both ftdi channels.
	@param[in] 		ftdiA, ftdiB pointer to a ftdi_context
	@param[in,out] 	readBack	 contains the bytes read back from the input pins

	@return 		>0 : number of bytes written to the chip
	@return			<0 : USB functions failed 
*/

int readInputs(struct ftdi_context* ftdiA, struct ftdi_context* ftdiB, const unsigned char* readBack)
{
     int uiRead;
     int uiWritten;
    unsigned char aucBuffer[8];


    aucBuffer[0] = W_LOWBYTE; // AD
    aucBuffer[1] = 0xFF;
    aucBuffer[2] = MYINPUT; //MYINPUT
    aucBuffer[3] = W_HIGHBYTE; // AC
    aucBuffer[4] = 0xFF;
    aucBuffer[5] = MYINPUT; //MYINPUT
    aucBuffer[6] = R_LOWBYTE;
    aucBuffer[7] = R_HIGHBYTE;


    /* Set the needed pins as MYINPUT */

    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->in_ep, aucBuffer, sizeof(aucBuffer), &uiWritten, ftdiA->usb_write_timeout)<0)
    {
        printf("usb bulk write failed");
        return -1;
    }

    /* Read back the needed pins */

    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->out_ep, aucBuffer, sizeof(aucBuffer), &uiRead, ftdiA->usb_write_timeout)<0)
    {
        printf("Reading back failed");
        return -2;
    }
     printf("Read back from channel %s - Lowbyte (AD): 0x%02x Highbyte (AC): 0x%02x\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":"error - invalid channel"), aucBuffer[2],aucBuffer[3]);


    aucBuffer[0] = W_LOWBYTE;
    aucBuffer[1] = 0xFF;
    aucBuffer[2] = MYINPUT; //MYINPUT
    aucBuffer[3] = W_HIGHBYTE;
    aucBuffer[4] = 0xFF;
    aucBuffer[5] = MYINPUT; //MYINPUT
    aucBuffer[6] = R_LOWBYTE;
    aucBuffer[7] = R_HIGHBYTE;


         /* Set ftdiB needed pins as MYINPUT */

    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->in_ep, aucBuffer, sizeof(aucBuffer), &uiWritten, ftdiB->usb_write_timeout)<0)
    {
        printf("usb bulk write failed");
        return -1;
    }

    /* Read back the needed pins */

    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->out_ep, aucBuffer, sizeof(aucBuffer), &uiRead, ftdiB->usb_write_timeout)<0)
    {
        printf("Reading back failed");
        return -2;
    }
     printf("Read back from channel %s - Lowbyte (BD): 0x%02x Highbyte (BC): 0x%02x\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":"error - invalid channel"), aucBuffer[2],aucBuffer[3]);


    return uiRead;
}



/** \brief stores a ftdi write command in a global buffer for later sending.

This function gets called repeatedly by i2c functions. It stores the commands in global Buffers
(aucBufferA and aucBufferB). The commands consist of a mask which determines which pins are configured as output and input
plus the actual output value to be written to the pins. All stored commands can be sent by the send_package_xx functions
which form the software i2c protocol.
	@param[in] 		ulIOMask	 input / output mask to set pin functionality
	@param[in]		ulOutput	 value to be assigned to pins set as output 

*/

void process_pins(unsigned long ulIOMask, unsigned long ulOutput)
{

    aucBufferA[indexA++] = W_LOWBYTE; //
    aucBufferA[indexA++] = (unsigned char)(ulOutput&MASK_ALOW); 
    aucBufferA[indexA++] = (unsigned char)(ulIOMask&MASK_ALOW);
    aucBufferA[indexA++] = W_HIGHBYTE; // AC
    aucBufferA[indexA++] = (unsigned char)((ulOutput&MASK_AHIGH)>>8);
    aucBufferA[indexA++] = (unsigned char)((ulIOMask&MASK_AHIGH)>>8); 

    /* Now Channel B first configure the channels for IO */
    aucBufferB[indexB++] = W_LOWBYTE; //
    aucBufferB[indexB++] = (unsigned char)((ulOutput&MASK_BLOW)>>16);
    aucBufferB[indexB++] = (unsigned char)((ulIOMask&MASK_BLOW)>>16);
    aucBufferB[indexB++] = W_HIGHBYTE; // BC
    aucBufferB[indexB++] = (unsigned char)((ulOutput&MASK_BHIGH)>>24); // BC
    aucBufferB[indexB++] = (unsigned char)((ulIOMask&MASK_BHIGH)>>24);
	
}


/** \brief stores a ftdi write command in a global buffer for later sending.

This function gets called repeatedly by i2c functions. It stores the commands in global Buffers
(aucBufferA and aucBufferB). The commands consist of a mask which determines which pins are set as input and output and and output value
which will be written to the pins set as output. All stored commands can be sent by the send_package_xx functions
which form the software i2c protocol.
	@param[in] 		ulIOMask	 input / output mask to set pin direction
	@param[in]		ulOutput	 value to be assigned to pins set as output 

*/
void process_pins_databack(unsigned long ulIOMask, unsigned long ulOutput)
{

    aucBufferA[indexA++] = W_LOWBYTE; //
    aucBufferA[indexA++] = (unsigned char)(ulOutput&MASK_ALOW); 
    aucBufferA[indexA++] = (unsigned char)(ulIOMask&MASK_ALOW);
    aucBufferA[indexA++] = W_HIGHBYTE; // AC
    aucBufferA[indexA++] = (unsigned char)((ulOutput&MASK_AHIGH)>>8);
    aucBufferA[indexA++] = (unsigned char)((ulIOMask&MASK_AHIGH)>>8); 
    aucBufferA[indexA++] = R_LOWBYTE;
    aucBufferA[indexA++] = R_HIGHBYTE;
    readIndexA+=2;

 
    /* Now Channel B first configure the channels for IO */
    aucBufferB[indexB++] = W_LOWBYTE; //
    aucBufferB[indexB++] = (unsigned char)((ulOutput&MASK_BLOW)>>16);
    aucBufferB[indexB++] = (unsigned char)((ulIOMask&MASK_BLOW)>>16);
    aucBufferB[indexB++] = W_HIGHBYTE; // BC
    aucBufferB[indexB++] = (unsigned char)((ulOutput&MASK_BHIGH)>>24); // BC
    aucBufferB[indexB++] = (unsigned char)((ulIOMask&MASK_BHIGH)>>24);
    aucBufferB[indexB++] = R_LOWBYTE;
    aucBufferB[indexB++] = R_HIGHBYTE;
    readIndexB+=2;
	
}


/** \brief sends the content of the global buffers to the ftdi chip. 

This function sends the content of the global Buffers aucBufferA and aucBufferB to the ftdi chip 
Furthermore it reads back the data of pins which were configured as input. In case of i2c these read back pins
can be acknowledge bits or data send back by the device. 
	@param[in] 		ftdiA, ftdiB pointer to a ftdi_context
	@return			0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
*/
int send_package_write8(struct ftdi_context *ftdiA, struct ftdi_context *ftdiB)
{

    unsigned int uiWritten;
    unsigned int uiRead;

	
	/* Send to Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->in_ep, aucBufferA, indexA, &uiWritten, ftdiA->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_A;
    }
	
	/* Send to chanel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->in_ep, aucBufferB, indexB, &uiWritten, ftdiB->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_B;
    }
	
	/* Wait until all commands are sent and processed by the chip */
	sleep_ms(1);

	/* Read from Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->out_ep, aucBufferA, sizeof(aucBufferA), &uiRead, ftdiA->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_A;
    }
	
	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexA + 2 ))
	{
		printf("Reading from Channel A failed! Expected %d bytes, read %d bytes!\n", (readIndexA+2), uiRead);
		ftdi_usb_purge_buffers(ftdiA);
		return ERR_INCORRECT_AMOUNT;
	}
	
	/* Read from Channel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->out_ep, aucBufferB, sizeof(aucBufferB),&uiRead, ftdiB->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_B;
    }

	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexB + 2 ))
	{
		printf("Reading from Channel B failed! Expected %d bytes, read %d bytes!\n", (readIndexB+2), uiRead);
		ftdi_usb_purge_buffers(ftdiB);
		return ERR_INCORRECT_AMOUNT;
	}
	
	/* Reset the index Counters for channel A and channel B */
    indexA = 0;
    indexB = 0;
    readIndexA = 0;
    readIndexB = 0;

    return 0;

}


/** \brief sends the content of the global buffers to the ftdi chip. 

This function sends the content of the global Buffers aucBufferA and aucBufferB to the ftdi chip. 
Furthermore it reads back the data of pins which were configured as input. The function returns a value which equals the amount of read back bytes.
The parameter aucReadbuffer will be used for storing 16 read back unsigned char values
	@param[in] 		ftdiA, ftdiB pointer to a ftdi_context
	@param[in, out] aucReadBuffer pointer to array of unsigned char values
	@param[in]		ucReadBufferLength number of elements to be stored in aucReadbuffer
	@return			0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
					

*/

int send_package_read8(struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, unsigned char* aucReadBuffer, unsigned char ucReadBufferLength)
{

    unsigned int uiWritten;
    unsigned int uiRead;

    /* Fill your readBuffer with zeroes, so nothing can go wrong mate ! */
    int i = 0;
    for(i; i<16; i++)
    {
        aucReadBuffer[i] = 0;
    }
		
	/* Send to Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->in_ep, aucBufferA, indexA, &uiWritten, ftdiA->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_A;
    }
	
	/* Send to chanel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->in_ep, aucBufferB, indexB, &uiWritten, ftdiB->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_B;
    }
	
	/* Wait until all commands are sent and processed by the chip */
	sleep_ms(1);

	/* Read from Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->out_ep, aucBufferA, sizeof(aucBufferA), &uiRead, ftdiA->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_A;
    }
	
	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexA + 2 ))
	{
		printf("Reading from Channel A failed! Expected %d bytes, read %d bytes!\n", (readIndexA+2), uiRead);
		ftdi_usb_purge_buffers(ftdiA);
		return ERR_INCORRECT_AMOUNT;
	}
	
	/* Read from Channel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->out_ep, aucBufferB, sizeof(aucBufferB),&uiRead, ftdiB->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_B;
    }

	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexB + 2 ))
	{
		printf("Reading from Channel B failed! Expected %d bytes, read %d bytes!\n", (readIndexB+2), uiRead);
		ftdi_usb_purge_buffers(ftdiB);
		return ERR_INCORRECT_AMOUNT;
	}
	

/* ucBitnumber marks the start of data in the buffer read out from usb->ep
            with 3 Acknowledges expected

*/

    unsigned int uiBytenumber = 14;
    unsigned char ucMask = 7;
    unsigned char ucBufferIndexA = 0;
    unsigned char ucBufferIndexB = 8;
    unsigned int uiCounter = 8;

    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;

        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        aucReadBuffer[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        aucReadBuffer[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        aucReadBuffer[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA4 */
        aucReadBuffer[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }

	
	/* Reset the index counters for Channel A and channel B */
    indexA = 0;
    indexB = 0;
    readIndexA = 0;
    readIndexB = 0;	
    return 0;

}


/** \brief sends the content of the global buffers to the ftdi chip. 

This function sends the content of the global Buffers aucBufferA and aucBufferB to the ftdi chip. 
Furthermore it reads back the data of pins which were configured as input. The function returns a value which equals the amount of read back bytes.
The parameter ausReadbuffer will be used for storing 16 read back unsigned short int values of 16 sensors 
	@param[in] 		ftdiA, ftdiB pointer to a ftdi_context
	@param[in, out] ausReadBuffer pointer to array of unsigned short values
	@param[in]		ucReadBufferLength number of elements to be stored in ausReadbuffer
	@return			0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT
					
*/
int send_package_read16(struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, unsigned short* ausReadBuffer, unsigned char ucReadBufferLength)
{

    unsigned int uiWritten;
    unsigned int uiRead;

    /* Fill your readBuffer with zeroes, so nothing can go wrong mate ! */
    int i = 0;
    for(i; i<16; i++)
    {
        ausReadBuffer[i] = 0;
    }	
	
 	/* Send to Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->in_ep, aucBufferA, indexA, &uiWritten, ftdiA->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_A;
    }
	
	/* Send to chanel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->in_ep, aucBufferB, indexB, &uiWritten, ftdiB->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_B;
    }
	
	/* Wait until all commands are sent and processed by the chip */
	sleep_ms(1);

	/* Read from Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->out_ep, aucBufferA, sizeof(aucBufferA), &uiRead, ftdiA->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_A;
    }
	
	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexA + 2 ))
	{
		printf("Reading from Channel A failed! Expected %d bytes, read %d bytes!\n", (readIndexA+2), uiRead);
		ftdi_usb_purge_buffers(ftdiA);
		return ERR_INCORRECT_AMOUNT;
	}
	
	/* Read from Channel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->out_ep, aucBufferB, sizeof(aucBufferB),&uiRead, ftdiB->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_B;
    }

	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexB + 2 ))
	{
		printf("Reading from Channel B failed! Expected %d bytes, read %d bytes!\n", (readIndexB+2), uiRead);
		ftdi_usb_purge_buffers(ftdiB);
		return ERR_INCORRECT_AMOUNT;
	}
	

    unsigned int uiBytenumber = 14;
    unsigned char ucMask = 7;
    unsigned char ucBufferIndexA = 0;
    unsigned char ucBufferIndexB = 8;
    unsigned int uiCounter = 8;


    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        ausReadBuffer[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA12 */
        ausReadBuffer[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA13 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA14 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA15 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
		
    }

    ucMask = 7;
    uiCounter = 8;

    /* Process colorH */
    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA1 */
        ausReadBuffer[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA2 */
        ausReadBuffer[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA3 */
        ausReadBuffer[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA9 */
        ausReadBuffer[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA10 */
        ausReadBuffer[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA11 */
        ausReadBuffer[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);


        uiBytenumber += 1;

        /* Channel AC */
        /* DA4 */
        ausReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA5 */
        ausReadBuffer[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA6 */
        ausReadBuffer[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA7 */
        ausReadBuffer[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel AD */
        /* DA12 */
        ausReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA13 */
        ausReadBuffer[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA14 */
        ausReadBuffer[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA15 */
        ausReadBuffer[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }

	
	/* Reset the index counters for Channel A and channel B */
    indexA = 0;
    indexB = 0;
    readIndexA = 0;
    readIndexB = 0;
	
    return 0;

}



/** \brief sends the content of the global buffers to the ftdi chip. 

This function sends the content of the global Buffers aucBufferA and aucBufferB to the ftdi chip. 
Furthermore it reads back the data of pins which were configured as input. The function returns a value which equals the amount of read back bytes.
The parameter ausReadbuffer will be used for storing 16 read back unsigned short int values of 16 sensors 
	@param[in] 		ftdiA, ftdiB pointer to a ftdi_context
	@param[in, out] aucReadBuffer  pointer to array of unsigned char  values
	@param[in, out] ausReadBuffer1 pointer to array of unsigned short values
	@param[in, out] ausReadBuffer2 pointer to array of unsigned short values
	@param[in, out] ausReadBuffer3 pointer to array of unsigned short values
	@param[in, out] ausReadBuffer4 pointer to array of unsigned short values
	@param[in]		ucReadBufferLength number of elements to be stored in ausReadbuffer
	
	@return			0 if succesful, errorcode if not 
		- @ref WRITE_ERR_CH_A
        - @ref WRITE_ERR_CH_B
        - @ref READ_ERR_CH_A
        - @ref READ_ERR_CH_B
        - @ref ERR_INCORRECT_AMOUNT					

*/
int send_package_read72(struct ftdi_context *ftdiA, struct ftdi_context *ftdiB, unsigned char* aucReadBuffer, unsigned short* ausReadBuffer1, unsigned short* ausReadBuffer2,
						  unsigned short* ausReadBuffer3, unsigned short* ausReadBuffer4, unsigned char ucReadBufferLength)
{
    unsigned int uiWritten;
    unsigned int uiRead;

    /* Fill your readBuffer with zeroes, so nothing can go wrong mate ! */
    int i = 0;
    for(i; i<16; i++)
    {
		aucReadBuffer[i]  = 0;
        ausReadBuffer1[i] = 0;
        ausReadBuffer2[i] = 0;
        ausReadBuffer3[i] = 0;
        ausReadBuffer4[i] = 0;
    }
	
 	/* Send to Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->in_ep, aucBufferA, indexA, &uiWritten, ftdiA->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_A;
    }
	
	/* Send to chanel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->in_ep, aucBufferB, indexB, &uiWritten, ftdiB->usb_write_timeout)<0)
    {
        printf("Writing to Channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return WRITE_ERR_CH_B;
    }
	
	/* Wait until all commands are sent and processed by the chip */
	sleep_ms(1);

	/* Read from Channel A */
    if(libusb_bulk_transfer(ftdiA->usb_dev, ftdiA->out_ep, aucBufferA, sizeof(aucBufferA), &uiRead, ftdiA->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiA->interface==0?"A":(ftdiA->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_A;
    }
	
	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexA + 2 ))
	{
		printf("Reading from Channel A failed! Expected %d bytes, read %d bytes!\n", (readIndexA+2), uiRead);
		ftdi_usb_purge_buffers(ftdiA);
		return ERR_INCORRECT_AMOUNT;
	}
	
	/* Read from Channel B */
    if(libusb_bulk_transfer(ftdiB->usb_dev, ftdiB->out_ep, aucBufferB, sizeof(aucBufferB),&uiRead, ftdiB->usb_read_timeout) < 0)
	{
        printf("Reading from channel %s failed!\n", ftdiB->interface==0?"A":(ftdiB->interface==1?"B":" error - invalid channel"));
        return READ_ERR_CH_B;
    }

	/* Compare expected number of bytes with the actual number of bytes */
	if(uiRead != (readIndexB + 2 ))
	{
		printf("Reading from Channel B failed! Expected %d bytes, read %d bytes!\n", (readIndexB+2), uiRead);
		ftdi_usb_purge_buffers(ftdiB);
		return ERR_INCORRECT_AMOUNT;
	}
	
	/* Index - Start of data */
	unsigned int uiBytenumber = 14;
	
    unsigned char ucMask = 7;
    unsigned char ucBufferIndexA = 0;
    unsigned char ucBufferIndexB = 8;
    unsigned int uiCounter = 8;
	
    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;

        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        aucReadBuffer[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        aucReadBuffer[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        aucReadBuffer[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        aucReadBuffer[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA4 */
        aucReadBuffer[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        aucReadBuffer[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }	
	
	
	
	
	/* First Word - Clear Colour */
    ucMask = 7;
    ucBufferIndexA = 0;
    ucBufferIndexB = 8;
    uiCounter = 8;

    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer1[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer1[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        ausReadBuffer1[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA12 */
        ausReadBuffer1[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA13 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA14 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA15 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
		
    }

    ucMask = 7;
    uiCounter = 8;

    /* Process colorH */
    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA1 */
        ausReadBuffer1[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA2 */
        ausReadBuffer1[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA3 */
        ausReadBuffer1[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA9 */
        ausReadBuffer1[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA10 */
        ausReadBuffer1[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA11 */
        ausReadBuffer1[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);


        uiBytenumber += 1;

        /* Channel AC */
        /* DA4 */
        ausReadBuffer1[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA5 */
        ausReadBuffer1[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA6 */
        ausReadBuffer1[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA7 */
        ausReadBuffer1[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel AD */
        /* DA12 */
        ausReadBuffer1[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA13 */
        ausReadBuffer1[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA14 */
        ausReadBuffer1[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA15 */
        ausReadBuffer1[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }

	/* Second Word */

    ucMask = 7;
    ucBufferIndexA = 0;
    ucBufferIndexB = 8;
    uiCounter = 8;


    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer2[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer2[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        ausReadBuffer2[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA12 */
        ausReadBuffer2[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA13 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA14 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA15 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
		
    }

    ucMask = 7;
    uiCounter = 8;

    /* Process colorH */
    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA1 */
        ausReadBuffer2[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA2 */
        ausReadBuffer2[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA3 */
        ausReadBuffer2[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA9 */
        ausReadBuffer2[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA10 */
        ausReadBuffer2[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA11 */
        ausReadBuffer2[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);


        uiBytenumber += 1;

        /* Channel AC */
        /* DA4 */
        ausReadBuffer2[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA5 */
        ausReadBuffer2[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA6 */
        ausReadBuffer2[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA7 */
        ausReadBuffer2[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel AD */
        /* DA12 */
        ausReadBuffer2[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA13 */
        ausReadBuffer2[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA14 */
        ausReadBuffer2[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA15 */
        ausReadBuffer2[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }
	
	/* Third Word */
    ucMask = 7;
    ucBufferIndexA = 0;
    ucBufferIndexB = 8;
    uiCounter = 8;


    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer3[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer3[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        ausReadBuffer3[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA12 */
        ausReadBuffer3[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA13 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA14 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA15 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
		
    }

    ucMask = 7;
    uiCounter = 8;

    /* Process colorH */
    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA1 */
        ausReadBuffer3[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA2 */
        ausReadBuffer3[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA3 */
        ausReadBuffer3[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA9 */
        ausReadBuffer3[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA10 */
        ausReadBuffer3[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA11 */
        ausReadBuffer3[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);


        uiBytenumber += 1; // Increment Bitnumber to get the Highbyte 

        /* Channel AC */
        /* DA4 */
        ausReadBuffer3[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA5 */
        ausReadBuffer3[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA6 */
        ausReadBuffer3[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA7 */
        ausReadBuffer3[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel AD */
        /* DA12 */
        ausReadBuffer3[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA13 */
        ausReadBuffer3[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA14 */
        ausReadBuffer3[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA15 */
        ausReadBuffer3[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }
	
	/* Fourth Word */
    ucMask = 7;
    ucBufferIndexA = 0;
    ucBufferIndexB = 8;
    uiCounter = 8;


    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer4[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA1 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA2 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA3 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer4[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA9 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA10 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA11 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);


        uiBytenumber += 1; 

        /* Channel AC */
        /* DA4 */
        ausReadBuffer4[ucBufferIndexA++] |= ((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask);
        /* DA5 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA6 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA7 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask);

        /* Channel BC */
        /* DA12 */
        ausReadBuffer4[ucBufferIndexB++] |= ((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask);
        /* DA13 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask);
        /* DA14 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask);
        /* DA15 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
		
    }

    ucMask = 7;
    uiCounter = 8;

    /* Process colorH */
    while(uiCounter>0)
    {
        ucBufferIndexA = 0;
        ucBufferIndexB = 8;
        /* Process parallel incoming bits of each channel */
        /* Channel AD */
        /* DA0 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA1 */
        ausReadBuffer4[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA2 */
        ausReadBuffer4[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA3 */
        ausReadBuffer4[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel BD */
        /* DA8 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA9 */
        ausReadBuffer4[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA10 */
        ausReadBuffer4[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA11 */
        ausReadBuffer4[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);


        uiBytenumber += 1;

        /* Channel AC */
        /* DA4 */
        ausReadBuffer4[ucBufferIndexA++] |= (((unsigned char)(aucBufferA[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA5 */
        ausReadBuffer4[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA6 */
        ausReadBuffer4[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA7 */
        ausReadBuffer4[ucBufferIndexA++] |= ((((unsigned char)(aucBufferA[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        /* Channel AD */
        /* DA12 */
        ausReadBuffer4[ucBufferIndexB++] |= (((unsigned char)(aucBufferB[uiBytenumber]&0x01)<<ucMask)<<8);
        /* DA13 */
        ausReadBuffer4[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x04)>>2)<<ucMask)<<8);
        /* DA14 */
        ausReadBuffer4[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x10)>>4)<<ucMask)<<8);
        /* DA15 */
        ausReadBuffer4[ucBufferIndexB++] |= ((((unsigned char)(aucBufferB[uiBytenumber]&0x40)>>6)<<ucMask)<<8);

        uiBytenumber +=3;
        uiCounter--;
        ucMask--;
    }

	
	/* Reset the index counters for Channel A and channel B */
    indexA = 0;
    indexB = 0;
    readIndexA = 0;
    readIndexB = 0;
	
    return 0;
}

