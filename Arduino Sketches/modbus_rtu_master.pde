/* 		
 Modbus over serial line - RTU Master Arduino Sketch 
  
 These functions implement functions 3 and 16 (read holding registers
 and preset multiple registers) of the Modbus RTU Protocol, to be used
 over the Arduino serial connection.
 
 This implementation DOES NOT fully comply with the Modbus specifications.
 
 This Arduino adaptation is derived from the work
 By P.Costigan email: phil@pcscada.com.au http://pcscada.com.au
 
 
 This code has its origins with 
 paul@pmcrae.freeserve.co.uk (http://www.pmcrae.freeserve.co.uk)
 who wrote a small program to read 100 registers from a modbus slave.
 
 I have used his code as a catalist to produce this more functional set
 of functions. Thanks paul.
 
 */
/* FIXME: not yet being used */
unsigned long interframe_delay = 2;  /* Modbus t3.5 = 2 ms */

/* 
 * preset_multiple_registers: Modbus function 16. Write the data from an
 * array into the holding registers of a slave.
 * INPUTS
 * slave: modbus slave id number 
 * start_addr: address of the slave's first register (+1)
 * reg_count: number of consecutive registers to preset
 * data: array of words (ints) with the data to write into the slave
 * RETURNS: the number of bytes received as response on success, or
 *         0 if no bytes received (i.e. response timeout)
 *        -1 to -4 (modbus exception code)
 *        -5 for other errors (port error, etc.).
 */

int preset_multiple_registers(int slave, int start_addr,
int reg_count, int *data);

/* 
 * read_holding_registers: Modbus function 3. Read the holding registers 
 * in a slave and put the data into an array
 * INPUTS
 * slave: modbus slave id number 
 * start_addr: address of the slave's first register (+1)
 * count: number of consecutive registers to read
 * dest: array of words (ints) on which the read data is to be stored
 * dest_size: size of the array, which should be at least 'count' 
 * RETURNS: the number of bytes received as response on success, or 
 *         0 if no valid response received (i.e. response timeout, bad crc)
 *        -1 to -4 (modbus exception code)
 *        -5 for other errors (port error, etc.).
 */

int read_holding_registers(int slave, int start_addr, int count,
int *dest, int dest_size);

void setup()
{
        const int baudrate = 9600;
        if (baudrate <= 19200)
                interframe_delay = (unsigned long)(3.5 * 11 / baudrate);  /* Modbus t3.5 */
        Serial.begin(baudrate); 	/* format 8N1, DOES NOT comply with Modbus spec. */
}

/* example data */
int retval;
int data[10];

void loop()
{
        /* example, this will write some data in the first 10 registers of slave 1  */
                retval = preset_multiple_registers(1,1,10, data);
                data[0] = retval; 
                data[1]++;
                data[8]=0xdead;
                data[9] = 0xbeaf;
                delay(500);                
}

/****************************************************************************
 * BEGIN MODBUS RTU MASTER FUNCTIONS
 ****************************************************************************/

#define TIMEOUT 1000		/* 1 second */
#define MAX_READ_REGS 125
#define MAX_WRITE_REGS 125
#define MAX_RESPONSE_LENGTH 256
#define PRESET_QUERY_SIZE 256
/* errors */
#define PORT_ERROR -5

/*
CRC
 
 INPUTS:
 	buf   ->  Array containing message to be sent to controller.            
 	start ->  Start of loop in crc counter, usually 0.
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

        return (temp);
}


/***********************************************************************
 * 
 * 	The following functions construct the required query into
 * 	a modbus query packet.
 * 
 ***********************************************************************/

#define REQUEST_QUERY_SIZE 6	/* the following packets require          */
#define CHECKSUM_SIZE 2		/* 6 unsigned chars for the packet plus   */
/* 2 for the checksum.                    */

void build_request_packet(int slave, int function, int start_addr,
int count, unsigned char *packet)
{
        packet[0] = slave;
        packet[1] = function;
        start_addr -= 1;
        packet[2] = start_addr >> 8;
        packet[3] = start_addr & 0x00ff;
        packet[4] = count >> 8;
        packet[5] = count & 0x00ff;
}

/*************************************************************************
 * 
 * modbus_query( packet, length)
 * 
 * Function to add a checksum to the end of a packet.
 * Please note that the packet array must be at least 2 fields longer than
 * string_length.
 **************************************************************************/

void modbus_query(unsigned char *packet, size_t string_length)
{
        int temp_crc;

        temp_crc = crc(packet, 0, string_length);

        packet[string_length++] = temp_crc >> 8;
        packet[string_length++] = temp_crc & 0x00FF;
        packet[string_length] = 0;
}



/***********************************************************************
 * 
 * send_query(query_string, query_length )
 * 
 * Function to send a query out to a modbus slave.
 ************************************************************************/

int send_query(unsigned char *query, size_t string_length)
{

        int i;

        modbus_query(query, string_length);
        string_length += 2;

        for (i = 0; i < string_length; i++) {
                Serial.print(query[i], BYTE);
        }
        /* without the following delay, the reading of the response might be wrong 
        * apparently, */
        delay(200);            /* FIXME: value to use? */

        return i; 		/* it does not mean that the write was succesful, though */
}


/***********************************************************************
 * 
 * 	receive_response( array_for_data )
 * 
 * Function to monitor for the reply from the modbus slave.
 * This function blocks for timeout seconds if there is no reply.
 * 
 * Returns:	Total number of characters received.
 ***********************************************************************/

int receive_response(unsigned char *received_string)
{

        int bytes_received = 0;
        int i = 0;
        /* wait for a response; this will block! */

        while(Serial.available() == 0) {
                delay(1);
                if (i++ > TIMEOUT)
                        return bytes_received;
        }

        /* FIXME: does Serial.available wait 1.5T or 3.5T before exiting the loop? */
        while(Serial.available()) {
                received_string[bytes_received] = Serial.read();
                bytes_received++;
                if (bytes_received >= MAX_RESPONSE_LENGTH)
                        return PORT_ERROR;
        }    

        return (bytes_received);
}


/*********************************************************************
 * 
 * 	modbus_response( response_data_array, query_array )
 * 
 * Function to the correct response is returned and that the checksum
 * is correct.
 * 
 * Returns:	string_length if OK
 * 		0 if failed
 * 		Less than 0 for exception errors
 * 
 * 	Note: All functions used for sending or receiving data via
 * 	      modbus return these return values.
 * 
 **********************************************************************/

int modbus_response(unsigned char *data, unsigned char *query)
{
        int response_length;

        unsigned int crc_calc = 0;
        unsigned int crc_received = 0;
        unsigned char recv_crc_hi;
        unsigned char recv_crc_lo;

      do {        // repeat if unexpected slave replied
                response_length = receive_response(data);
      } while ((response_length > 0) && (data[0] != query[0]));

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
                        response_length = 0;
                }



                /********** check for exception response *****/

                if (response_length && data[1] != query[1]) {
                        response_length = 0 - data[2];
                }
        }
        return (response_length);
}


/************************************************************************
 * 
 * 	read_reg_response
 * 
 * 	reads the response data from a slave and puts the data into an
 * 	array.
 * 
 ************************************************************************/

int read_reg_response(int *dest, int dest_size, unsigned char *query)
{

        unsigned char data[MAX_RESPONSE_LENGTH];
        int raw_response_length;
        int temp, i;

        raw_response_length = modbus_response(data, query);
        if (raw_response_length > 0)
                raw_response_length -= 2;

        if (raw_response_length > 0) {
                /* FIXME: data[2] * 2 ???!!! data[2] isn't already the byte count (number of registers * 2)?! */
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
 * 
 * 	preset_response
 * 
 * 	Gets the raw data from the input stream.
 * 
 ***********************************************************************/

int preset_response(unsigned char *query)
{
        unsigned char data[MAX_RESPONSE_LENGTH];
        int raw_response_length;

        raw_response_length = modbus_response(data, query);

        return (raw_response_length);
}


/************************************************************************
 * 
 * 	read_holding_registers
 * 
 * 	Read the holding registers in a slave and put the data into
 * 	an array.
 * 
 *************************************************************************/

int read_holding_registers(int slave, int start_addr, int count,
int *dest, int dest_size)
{
        int function = 0x03; 	/* Function: Read Holding Registers */
        int ret;

        unsigned char packet[REQUEST_QUERY_SIZE + CHECKSUM_SIZE];

        if (count > MAX_READ_REGS) {
                count = MAX_READ_REGS;
        }

        build_request_packet(slave, function, start_addr, count, packet);

        if (send_query(packet, REQUEST_QUERY_SIZE) > -1) {
                ret = read_reg_response(dest, dest_size, packet);
        } 
        else {

                ret = -1;
        }

        return (ret);
}


/************************************************************************
 * 
 * 	preset_multiple_registers
 * 
 * 	Write the data from an array into the holding registers of a
 * 	slave.
 * 
 *************************************************************************/

int preset_multiple_registers(int slave, int start_addr,
int reg_count, int *data)
{
        int function = 0x10; 	/* Function 16: Write Multiple Registers */
        int byte_count, i, packet_size = 6;
        int ret;

        unsigned char packet[PRESET_QUERY_SIZE];

        if (reg_count > MAX_WRITE_REGS) {
                reg_count = MAX_WRITE_REGS;
        }

        build_request_packet(slave, function, start_addr, reg_count, packet);
        byte_count = reg_count * 2;
        packet[6] = (unsigned char)byte_count;

        for (i = 0; i < reg_count; i++) {
                packet_size++;
                packet[packet_size] = data[i] >> 8;
                packet_size++;
                packet[packet_size] = data[i] & 0x00FF;
        }

        packet_size++;
        if (send_query(packet, packet_size) > -1) {
                ret = preset_response(packet);
        } 
        else {
                ret = -1;
        }

        return (ret);
}



