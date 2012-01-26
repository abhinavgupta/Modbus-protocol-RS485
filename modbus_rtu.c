/* modbus_rtu.c 



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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, US

*/

#include <fcntl.h>		/* File control definitions */
#include <stdio.h>		/* Standard input/output */
#include <string.h>
#include <stdlib.h>
#include <termio.h>		/* POSIX terminal control definitions */
#include <sys/time.h>		/* Time structures for select() */
#include <unistd.h>		/* POSIX Symbolic Constants */
#include <errno.h>		/* Error definitions */
#include "modbus_rtu.h"

#define DEBUG                /* uncomment to see the data sent and received */
// #define DEBUG_CHITO  /* mas comentarios para encontrar el error en recepcion */

int char_interval_timeout;

enum {
FALSE = 0,
TRUE
};

/*************************************************************************

   modbus_query( packet, length)

Function to add a checksum to the end of a packet.
Please note that the packet array must be at least 2 fields longer than
string_length.
**************************************************************************/

void modbus_query(unsigned char *packet, size_t string_length)
{
	int temp_crc;

	/* declaration */
	unsigned int crc(unsigned char buf[], int start, int cnt);



	temp_crc = crc(packet, 0, string_length);

	packet[string_length++] = temp_crc >> 8;
	packet[string_length++] = temp_crc & 0x00FF;
}





/***********************************************************************

   send_query( file_descriptor, query_string, query_length )

Function to send a query out to a modbus slave.
************************************************************************/

int send_query(int ttyfd, unsigned char *query, size_t string_length)
{
	int write_stat;

	int status;
#ifdef DEBUG
	int i;
#endif

	modbus_query(query, string_length);
	string_length += 2;

#ifdef DEBUG
// Print to stderr the hex value of each character that is about to be
// sent to the modbus slave.

	for (i = 0; i < string_length; i++) {
		fprintf(stderr, "[%0.2X]", query[i]);
	}
	fprintf(stderr, " - string length = %d", string_length);
	fprintf(stderr, "\n");
#endif

	tcflush(ttyfd, TCIOFLUSH);	/* flush the input & output streams */

	/* configura la linea RTS para transmision */
	ioctl(ttyfd, TIOCMGET, &status);
	status |= TIOCM_RTS;
	ioctl(ttyfd, TIOCMSET, &status);
	write_stat = write(ttyfd, query, string_length);
	tcflush(ttyfd, TCIFLUSH);	/* maybe not neccesary */

	return (write_stat);
}


/*********************************************************************

	modbus_response( response_data_array, query_array )

   Function to the correct response is returned and that the checksum
   is correct.

   Returns:	string_length if OK
		0 if failed
		Less than 0 for exception errors

	Note: All functions used for sending or receiving data via
	      modbus return these return values.

**********************************************************************/

int modbus_response(unsigned char *data, unsigned char *query, int fd)
{
	int response_length;

	unsigned short crc_calc = 0;
	unsigned short crc_received = 0;
	unsigned char recv_crc_hi;
	unsigned char recv_crc_lo;


	/* local declaration */
	int receive_response(unsigned char *received_string, int ttyfd);
	unsigned int crc(unsigned char buf[], int start, int cnt);


	response_length = receive_response(data, fd);
	if (response_length) {
		crc_calc = crc(data, 0, response_length - 2);

		recv_crc_hi = (unsigned) data[response_length - 2];
		recv_crc_lo = (unsigned) data[response_length - 1];

		crc_received = data[response_length - 2];
		crc_received = (unsigned) crc_received << 8;
		crc_received =
		    crc_received | (unsigned) data[response_length - 1];


		/*********** check CRC of response ************/

		if (crc_calc != crc_received) {

			fprintf(stderr, "crc error received ");
			fprintf(stderr, "%0X - ", crc_received);
			fprintf(stderr, "crc_calc %0X\n", crc_calc);

			response_length = 0;

		}



		/********** check for exception response *****/

		if (response_length && data[1] != query[1]) {
			response_length = 0 - data[2];
		}
	}
	/* FIXME: it does not check for the slave id; jpz */
	return (response_length);
}








/***********************************************************************

	receive_response( array_for_data )

   Function to monitor for the reply from the modbus slave.
   This function blocks for timeout seconds if there is no reply.

   Returns:	Total number of characters received.
***********************************************************************/

int receive_response(unsigned char *received_string, int ttyfd)
{

	int rxchar = PORT_FAILURE;
	int data_avail = FALSE;
	int bytes_received = 0;
	int read_stat;

	int timeout = 1;	/* 1 second */

	fd_set rfds;

	struct timeval tv;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	FD_ZERO(&rfds);
	FD_SET(ttyfd, &rfds);




#ifdef DEBUG
	fprintf(stderr, "Waiting for response.\n");
#endif

	/* wait for a response */
	data_avail = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);

	if (!data_avail) {
		bytes_received = 0;
		fprintf(stderr, "Comms time out\n");
	}


	tv.tv_sec = 0;
	tv.tv_usec = char_interval_timeout;

	FD_ZERO(&rfds);
	FD_SET(ttyfd, &rfds);

	while (data_avail) {
		/* antes de leer un byte, hacer una pausa de "un byte", 
		 * de lo contrario, otro proceso se apodera de la linea RTS */
		//esperaRTS(1);
		/* if no character at the buffer wait char_interval_timeout */
		/* before accepting end of response                         */

		if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv)) {


			read_stat = read(ttyfd, &rxchar, 1);

			if (read_stat < 0) {
				bytes_received = PORT_FAILURE;
				data_avail = FALSE;
			} else {
				rxchar = rxchar & 0xFF;
				received_string[bytes_received++] = rxchar;
			}


			if (bytes_received >= MAX_RESPONSE_LENGTH) {
				bytes_received = PORT_FAILURE;
				data_avail = FALSE;
			}
#ifdef DEBUG
			/* display the hex code of each character received */
			fprintf(stderr, "<%0.2X>", rxchar);
#endif

		} else {
			data_avail = FALSE;
		}

	}

#ifdef DEBUG
	fprintf(stderr, "\n");
#endif
/* Al restar 2 a bytes_received, el crc de retorno es calculado a partir de 
 *	2 bytes menos, lo que resulta en crc_error en lectura. ¡ELIMINADO!
 */
#if 0
	if (bytes_received > 2) {
		bytes_received -= 2;
	}
#endif
#ifdef DEBUG_CHITO
	fprintf(stderr, "receive_response: bytes recibidos: %d\n",
		bytes_received);
#endif

	return (bytes_received);
}







/***********************************************************************

	The following functions construct the required query into
	a modbus query packet.

***********************************************************************/

#define REQUEST_QUERY_SIZE 6	/* the following packets require          */
#define CHECKSUM_SIZE 2		/* 6 unsigned chars for the packet plus   */
				/* 2 for the checksum.                    */

void build_request_packet(int slave, int function, int start_addr,
			  int count, unsigned char *packet)
{
	packet[0] = slave, packet[1] = function;
	start_addr -= 1;
	packet[2] = start_addr >> 8;
	packet[3] = start_addr & 0x00ff;
	packet[4] = count >> 8;
	packet[5] = count & 0x00ff;

}






/************************************************************************

	read_IO_status

	read_coil_stat_query and read_coil_stat_response interigate
	a modbus slave to get coil status. An array of coils shall be
	set to TRUE or FALSE according to the response from the slave.
	
*************************************************************************/

int read_IO_status(int function, int slave, int start_addr, int count,
		   int *dest, int dest_size, int ttyfd)
{
	/* local declaration */
	int read_IO_stat_response(int *dest, int dest_size, int coil_count,
				  unsigned char *query, int fd);

	int status;


	unsigned char packet[REQUEST_QUERY_SIZE + CHECKSUM_SIZE];
	build_request_packet(slave, function, start_addr, count, packet);

	if (send_query(ttyfd, packet, REQUEST_QUERY_SIZE) > -1) {
		status = read_IO_stat_response(dest, dest_size,
					       count, packet, ttyfd);
	} else {
		status = PORT_FAILURE;
	}



	return (status);
}



/************************************************************************

	read_coil_status 

	reads the boolean status of coils and sets the array elements
	in the destination to TRUE or FALSE

*************************************************************************/

int read_coil_status(int slave, int start_addr, int count,
		     int *dest, int dest_size, int ttyfd)
{
	int function = 0x01;
	int status;

	status = read_IO_status(function, slave, start_addr, count,
				dest, dest_size, ttyfd);

	return (status);
}





/************************************************************************

	read_input_status

	same as read_coil_status but reads the slaves input table.

************************************************************************/

int read_input_status(int slave, int start_addr, int count,
		      int *dest, int dest_size, int ttyfd)
{
	int function = 0x02;	/* Function: Read Input Status */
	int status;

	status = read_IO_status(function, slave, start_addr, count,
				dest, dest_size, ttyfd);

	return (status);
}




/**************************************************************************

	read_IO_stat_response

	this function does the work of setting array elements to TRUE
	or FALSE.

**************************************************************************/

int read_IO_stat_response(int *dest, int dest_size, int coil_count,
			  unsigned char *query, int fd)
{

	unsigned char data[MAX_RESPONSE_LENGTH];
	int raw_response_length;
	int temp, i, bit, dest_pos = 0;
	int coils_processed = 0;

	raw_response_length = modbus_response(data, query, fd);


	if (raw_response_length > 0) {
		for (i = 0; i < (data[2]) && i < dest_size; i++) {
			/* shift reg hi_byte to temp */
			temp = data[3 + i];
			for (bit = 0x01; bit & 0xff &&
			     coils_processed < coil_count;) {
				if (temp & bit) {
					dest[dest_pos] = TRUE;
				} else {
					dest[dest_pos] = FALSE;
				}
				coils_processed++;
				dest_pos++;
				bit = bit << 1;
			}
		}
	}

	return (raw_response_length);
}





/************************************************************************

	read_registers

	read the data from a modbus slave and put that data into an array.

************************************************************************/

int read_registers(int function, int slave, int start_addr, int count,
		   int *dest, int dest_size, int ttyfd)
{
	/* local declaration */
	int read_reg_response(int *dest, int dest_size,
			      unsigned char *query, int fd);

	int status;


	unsigned char packet[REQUEST_QUERY_SIZE + CHECKSUM_SIZE];
	build_request_packet(slave, function, start_addr, count, packet);

	if (send_query(ttyfd, packet, REQUEST_QUERY_SIZE) > -1) {
		status = read_reg_response(dest, dest_size, packet, ttyfd);
	} else {
		status = PORT_FAILURE;
	}

	return (status);

}



/************************************************************************

	read_holding_registers

	Read the holding registers in a slave and put the data into
	an array.

*************************************************************************/

int read_holding_registers(int slave, int start_addr, int count,
			   int *dest, int dest_size, int ttyfd)
{
	int function = 0x03;	/* Function: Read Holding Registers */
	int status;

	if (count > MAX_READ_REGS) {
		count = MAX_READ_REGS;
#ifdef DEBUG
		fprintf(stderr, "Too many registers requested.\n");
#endif
	}

	status = read_registers(function, slave, start_addr, count,
				dest, dest_size, ttyfd);

	return (status);
}





/************************************************************************

	read_input_registers

	Read the inputg registers in a slave and put the data into
	an array. 

*************************************************************************/

int read_input_registers(int slave, int start_addr, int count,
			 int *dest, int dest_size, int ttyfd)
{
	int function = 0x04;	/* Function: Read Input Reqisters */
	int status;

	if (count > MAX_INPUT_REGS) {
		count = MAX_INPUT_REGS;
#ifdef DEBUG
		fprintf(stderr, "Too many input registers requested.\n");
#endif
	}

	status = read_registers(function, slave, start_addr, count,
				dest, dest_size, ttyfd);

	return (status);
}





/************************************************************************

	read_reg_response

	reads the response data from a slave and puts the data into an
	array.

************************************************************************/

int read_reg_response(int *dest, int dest_size, unsigned char *query,
		      int fd)
{

	unsigned char data[MAX_RESPONSE_LENGTH];
	int raw_response_length;
	int temp, i;



	raw_response_length = modbus_response(data, query, fd);
	if (raw_response_length > 0)
		raw_response_length -= 2;


	if (raw_response_length > 0) {
		for (i = 0;
		     i < (data[2] * 2) && i < (raw_response_length / 2);
		     i++) {
			/* shift reg hi_byte to temp */
			temp = data[3 + i * 2] << 8;
			/* OR with lo_byte           */
			temp = temp | data[4 + i * 2];

			dest[i] = temp;
		}
	}
	return (raw_response_length);
}





/***********************************************************************

	preset_response

	Gets the raw data from the input stream.

***********************************************************************/

int preset_response(unsigned char *query, int fd)
{
	unsigned char data[MAX_RESPONSE_LENGTH];
	int raw_response_length;

	raw_response_length = modbus_response(data, query, fd);

	return (raw_response_length);
}






/*************************************************************************

	set_single

	sends a value to a register in a slave. 

**************************************************************************/

int set_single(int function, int slave, int addr, int value, int fd)
{

	int status;

	unsigned char packet[REQUEST_QUERY_SIZE];
	packet[0] = slave;
	packet[1] = function;
	addr -= 1;
	packet[2] = addr >> 8;
	packet[3] = addr & 0x00FF;
	packet[4] = value >> 8;
	packet[5] = value & 0x00FF;

	if (send_query(fd, packet, 6) > -1) {
		status = preset_response(packet, fd);
	} else {
		status = PORT_FAILURE;
	}

	return (status);
}






/*************************************************************************

	force_single_coil

	turn on or off a single coil on the slave device

*************************************************************************/

int force_single_coil(int slave, int coil_addr, int state, int fd)
{
	int function = 0x05;
	int status;

	if (state)
		state = 0xFF00;

	status = set_single(function, slave, coil_addr, state, fd);

	return (status);
}





/*************************************************************************

	preset_single_register

	sets a value in one holding register in the slave device

*************************************************************************/

int preset_single_register(int slave, int reg_addr, int value, int fd)
{
	int function = 0x06;
	int status;

	status = set_single(function, slave, reg_addr, value, fd);

	return (status);
}





/************************************************************************

	set_multiple_coils

	Takes an array of ints and sets or resets the coils on a slave
	appropriatly.

*************************************************************************/

#define PRESET_QUERY_SIZE 210

int set_multiple_coils(int slave, int start_addr, int coil_count,
		       int *data, int fd)
{
	int byte_count;
	int i, bit, packet_size = 6;
	int coil_check = 0;
	int data_array_pos = 0;
	int status;

	unsigned char packet[PRESET_QUERY_SIZE];

	if (coil_count > MAX_WRITE_COILS) {
		coil_count = MAX_WRITE_COILS;
#ifdef DEBUG
		fprintf(stderr, "Writing to too many coils.\n");
#endif
	}
	packet[0] = slave;
	packet[1] = 0x0F;
	start_addr -= 1;
	packet[2] = start_addr >> 8;
	packet[3] = start_addr & 0x00FF;
	packet[4] = coil_count >> 8;
	packet[5] = coil_count & 0x00FF;
	byte_count = (coil_count / 8) + 1;
	packet[6] = byte_count;

	bit = 0x01;

	for (i = 0; i < byte_count; i++) {
		packet[++packet_size] = 0;

		while (bit & 0xFF && coil_check++ < coil_count) {
			if (data[data_array_pos++]) {
				packet[packet_size] |= bit;
			} else {
				packet[packet_size] &= ~bit;
			}
			bit = bit << 1;
		}
		bit = 0x01;
	}

	if (send_query(fd, packet, ++packet_size) > -1) {
		status = preset_response(packet, fd);
	} else {
		status = PORT_FAILURE;
	}

	return (status);
}





/*************************************************************************

	preset_multiple_registers

	copy the values in an array to an array on the slave.

***************************************************************************/

int preset_multiple_registers(int slave, int start_addr,
			      int reg_count, int *data, int fd)
{
	int byte_count, i, packet_size = 6;
	int status;

	unsigned char packet[PRESET_QUERY_SIZE];

	if (reg_count > MAX_WRITE_REGS) {
		reg_count = MAX_WRITE_REGS;
#ifdef DEBUG
		fprintf(stderr,
			"Trying to write to too many registers.\n");
#endif
	}

	packet[0] = slave;
	packet[1] = 0x10;
	start_addr -= 1;
	packet[2] = start_addr >> 8;
	packet[3] = start_addr & 0x00FF;
	packet[4] = reg_count >> 8;
	packet[5] = reg_count & 0x00FF;
	byte_count = reg_count * 2;
	packet[6] = byte_count;

	for (i = 0; i < reg_count; i++) {
		packet[++packet_size] = data[i] >> 8;
		packet[++packet_size] = data[i] & 0x00FF;
	}

	if (send_query(fd, packet, ++packet_size) > -1) {
		status = preset_response(packet, fd);
	} else {
		status = PORT_FAILURE;
	}

	return (status);
}











/****************************************************************************
***************************** [  BEGIN:  crc ] ******************************
*****************************************************************************
INPUTS:
	buf   ->  Array containing message to be sent to controller.            	start ->  Start of loop in crc counter, usually 0.
	cnt   ->  Amount of bytes in message being sent to controller/
OUTPUTS:
	temp  ->  Returns crc byte for message.
COMMENTS:
	This routine calculates the crc high and low byte of a message.
	Note that this crc is only used for Modbus, not Modbus+ etc. 
****************************************************************************/

unsigned int crc(unsigned char *buf, int start, int cnt)
{
	int i, j;
	unsigned temp, temp2, flag;

	temp = 0xFFFF;
#ifdef DEBUG_CHITO
	fprintf(stderr, "crc: string length %d\n", cnt);
#endif
	for (i = start; i < cnt; i++) {
		temp = temp ^ buf[i];

		for (j = 1; j <= 8; j++) {
			flag = temp & 0x0001;
			temp = temp >> 1;
			if (flag)
				temp = temp ^ 0xA001;
		}
	}

	/* Reverse byte order. */

	temp2 = temp >> 8;
	temp = (temp << 8) | temp2;
	temp &= 0xFFFF;

#ifdef DEBUG_CHITO
	fprintf(stderr, "crc: temp crc %0.2X\n", temp);
#endif

	return (temp);
}



/************************************************************************

	set_up_comms

	This function sets up a serial port for RTU communications to
	modbus.

**************************************************************************/

int set_up_comms(char *device, int baud_i, char *parity)
{
	int ttyfd;
	struct termios settings;
	int k, n, status;	// jpz

	speed_t baud_rate;

#ifdef DEBUG
	fprintf(stderr, "opening %s\n", device);
#endif

	switch (baud_i) {
	case 110:
		baud_rate = B110;
		char_interval_timeout = TO_B110;
		break;
	case 300:
		baud_rate = B300;
		char_interval_timeout = TO_B300;
		break;
	case 600:
		baud_rate = B600;
		char_interval_timeout = TO_B600;
		break;
	case 1200:
		baud_rate = B1200;
		char_interval_timeout = TO_B1200;
		break;
	case 2400:
		baud_rate = B2400;
		char_interval_timeout = TO_B2400;
		break;
	case 4800:
		baud_rate = B4800;
		char_interval_timeout = TO_B4800;
		break;
	case 9600:
	case 0:
		baud_rate = B9600;
		char_interval_timeout = TO_B9600;
		break;
	case 19200:
		baud_rate = B19200;
		char_interval_timeout = TO_B19200;
		break;
	case 38400:
		baud_rate = B38400;
		char_interval_timeout = TO_B38400;
		break;
	case 57600:
		baud_rate = B57600;
		char_interval_timeout = TO_B57600;
		break;
	case 115200:
		baud_rate = B115200;
		char_interval_timeout = TO_B115200;
		break;
	default:
		baud_rate = B9600;
		char_interval_timeout = TO_B9600;
		fprintf(stderr, "Unknown baud rate %d for %s.", baud_i,
			device);
	}



	if ((ttyfd = open(device, O_RDWR)) < 0) {
		fprintf(stderr, "Error opening device %s.   ", device);

		fprintf(stderr, "Error no. %d \n", errno);
		exit(1);	/* stop the program. This maybe should */
		/* be a bit kinder but will do for now. */
	}
#ifdef DEBUG
	fprintf(stderr, "%s open\n", device);
#endif

	/* read your man page for the meaning of all this. # man termios */
	/* Its a bit to involved to comment here                         */


	cfsetispeed(&settings, baud_rate);	/* Set the baud rate */
	cfsetospeed(&settings, baud_rate);

	settings.c_line = 0;

	settings.c_iflag |= IGNBRK;
	settings.c_iflag |= IGNPAR;
	settings.c_iflag &= ~PARMRK;
	settings.c_iflag &= ~INPCK;
	settings.c_iflag &= ~ISTRIP;
	settings.c_iflag &= ~INLCR;
	settings.c_iflag &= ~IGNCR;
	settings.c_iflag &= ~ICRNL;
	settings.c_iflag &= ~IUCLC;
	settings.c_iflag &= ~IXON;
	settings.c_iflag |= IXANY;
	settings.c_iflag &= ~IXOFF;
	settings.c_iflag &= ~IMAXBEL;

	settings.c_oflag |= OPOST;
	settings.c_oflag &= ~OLCUC;
	settings.c_oflag &= ~ONLCR;
	settings.c_oflag &= ~OCRNL;
	settings.c_oflag |= ONOCR;
	settings.c_oflag &= ~ONLRET;
	settings.c_oflag &= ~OFILL;
	settings.c_oflag &= ~OFDEL;

	settings.c_cflag &= ~CSIZE;
	settings.c_cflag |= CS8;
	settings.c_cflag &= ~CSTOPB;
	settings.c_cflag |= CREAD;

	if (strncmp(parity, "none", 4) == 0) {
		settings.c_cflag &= ~PARENB;
		settings.c_cflag &= ~PARODD;
	} else if (strncmp(parity, "even", 4) == 0) {
		settings.c_cflag |= PARENB;
		settings.c_cflag &= ~PARODD;
	} else {
		settings.c_cflag |= PARENB;
		settings.c_cflag |= PARODD;
	}

	settings.c_cflag &= ~HUPCL;
	settings.c_cflag |= CLOCAL;
	settings.c_cflag &= ~CRTSCTS;

	settings.c_lflag &= ~ISIG;
	settings.c_lflag &= ~ICANON;
	settings.c_lflag &= ~ECHO;
	settings.c_lflag |= IEXTEN;

	settings.c_cc[VMIN] = 0;
	settings.c_cc[VTIME] = 0;

	if (tcsetattr(ttyfd, TCSANOW, &settings) < 0) {
		fprintf(stderr, "tcsetattr failed\n");
		exit(1);
	}

	return (ttyfd);
}
