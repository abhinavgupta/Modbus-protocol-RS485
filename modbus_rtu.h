/* 		modbus_rtu.h

   By P.Costigan email: phil@pcscada.com.au http://pcscada.com.au
 
   These library of functions are designed to enable a program send and
   receive data from a device that communicates using the Modbus protocol.
 
   Copyright (C) 2000 Philip Costigan  P.C. SCADA LINK PTY. LTD.
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                    
 */


#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#define MAX_DATA_LENGTH 246
#define MAX_QUERY_LENGTH 256
#define MAX_RESPONSE_LENGTH 256


/***********************************************************************

	 Note: All functions used for sending or receiving data via
	       modbus return these return values.


	Returns:	string_length if OK
			0 if failed
			Less than 0 for exception errors

***********************************************************************/

#define COMMS_FAILURE 0
#define ILLEGAL_FUNCTION -1
#define ILLEGAL_DATA_ADDRESS -2
#define ILLEGAL_DATA_VALUE -3
#define SLAVE_DEVICE_FAILURE -4
#define ACKNOWLEDGE -5
#define SLAVE_DEVICE_BUSY -6
#define NEGATIVE_ACKNOWLEDGE -7
#define MEMORY_PARITY_ERROR -8

#define PORT_FAILURE -11



/************************************************************************

	read_coil_status()

	reads the boolean status of coils and sets the array elements
	in the destination to TRUE or FALSE.

*************************************************************************/

int read_coil_status( int slave, int start_addr, int count,
			  int *dest, int dest_size, int fd );





/************************************************************************

	read_input_status()

	same as read_coil_status but reads the slaves input table.

************************************************************************/

int read_input_status( int slave, int start_addr, int count,
			   int *dest, int dest_size, int fd );





/***********************************************************************

	read_holding_registers()

	Read the holding registers in a slave and put the data into
	an array.

************************************************************************/

#define MAX_READ_REGS 100

int read_holding_registers( int slave, int start_addr, int count, 
			  	int *dest, int dest_size, int fd );




/***********************************************************************

	read_input_registers()

	Read the inputg registers in a slave and put the data into
	an array.

***********************************************************************/

#define MAX_INPUT_REGS 100

int read_input_registers( int slave, int start_addr, int count,
				int *dest, int dest_size, int fd );







/************************************************************************

	force_single_coil()

	turn on or off a single coil on the slave device.

************************************************************************/

int force_single_coil( int slave, int addr, int state, int fd );







/*************************************************************************

	preset_single_register()

	sets a value in one holding register in the slave device.

*************************************************************************/

int preset_single_register( int slave, int reg_addr, int value, int fd );






/*************************************************************************

	set_multiple_coils()

	Takes an array of ints and sets or resets the coils on a slave
	appropriatly.

**************************************************************************/

#define MAX_WRITE_COILS 800

int set_multiple_coils( int slave, int start_addr, 
			    int coil_count, int *data, int fd );






/*************************************************************************

	preset_multiple_registers()

	copy the values in an array to an array on the slave.

*************************************************************************/

#define MAX_WRITE_REGS 100

int preset_multiple_registers( int slave, int start_addr,
				   int reg_count, int *data, int fd );








/***************************************************************************

	set_up_comms

	This function sets up a serial port for RTU communications to
	modbus.


***************************************************************************/

int set_up_comms( char *device, int baud, char *parity );
/* baud should be the plain baud rate, eg 2400; zero for the default 9600.
 * If an unsupported baud rate is specified, prints a message to stderr and
 * uses 9600. */


#define TO_B110	3200000	/* These values are the timeout delays */
#define TO_B300 1600000	/* at the end of packets of data.      */
#define TO_B600  800000 /* At this stage a true calculation    */
#define TO_B1200 400000	/* has not been worked out. So these   */
#define TO_B2400 200000	/* values are just a guess.            */
#define TO_B4800 100000	/*                                     */
#define TO_B9600  50000	/* The spec says that a message frame  */
#define TO_B19200 25000	/* starts after a silent interval of   */
#define TO_B38400 12500 /* at least 3.5 character times.       */
#define TO_B57600  8333 /* These are uS times.                */
#define TO_B115200 4167 




#endif  /* MODBUS_RTU_H */
