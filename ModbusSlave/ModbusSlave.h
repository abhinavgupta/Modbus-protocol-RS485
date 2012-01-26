#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H
#include "WProgram.h"

class ModbusSlave {
private:
  unsigned char slave;
  char txenpin;

  unsigned int crc(unsigned char *buf, unsigned char start, unsigned char cnt);
  void build_read_packet(unsigned char function, unsigned char count, unsigned char *packet);
  void build_write_packet(unsigned char function, unsigned int start_addr, unsigned char count, unsigned char *packet);
  void build_write_single_packet(unsigned char function, unsigned int write_addr, unsigned int reg_val, unsigned char* packet);
  void build_error_packet(unsigned char function,unsigned char exception, unsigned char *packet);
  void modbus_reply(unsigned char *packet, unsigned char string_length);
  int send_reply(unsigned char *query, unsigned char string_length);
  int receive_request(unsigned char *received_string);
  int modbus_request(unsigned char *data);
  int validate_request(unsigned char *data, unsigned char length, unsigned int regs_size);
  int write_regs(unsigned int start_addr, unsigned char *query, int *regs);
  int preset_multiple_registers(unsigned int start_addr,unsigned char count,unsigned char *query,int *regs);
  int read_holding_registers(unsigned int start_addr, unsigned char reg_count, int *regs);
  int write_single_register(unsigned int write_addr, unsigned char *query, int *regs);  
  void configure(long baud, char parity, char txenpin);
  
public:
/* 
 * configure(slave, baud, parity, txenpin)
 *
 * sets the communication parameters for of the serial line.
 *
 * slave: identification number of the slave in the Modbus network (1 to 127)
 * baud: baudrate in bps (typical values 9600, 19200... 115200)
 * parity: a single character sets the parity mode (character frame format): 
 *         'n' no parity (8N1); 'e' even parity (8E1), 'o' for odd parity (8O1).
 * txenpin: arduino pin number that controls transmision/reception
 *        of an external half-duplex device (e.g. a RS485 interface chip).
 *        0 or 1 disables this function (for a two-device network)
 *        >2 for point-to-multipoint topology (e.g. several arduinos)
 */
  void configure(unsigned char slave, long baud, char parity, char txenpin);

/*
 * update(regs, regs_size)
 * 
 * checks if there is any valid request from the modbus master. If there is,
 * performs the requested action
 * 
 * regs: an array with the holding registers. They start at address 1 (master point of view)
 * regs_size: total number of holding registers, i.e. the size of the array regs.
 * returns: 0 if no request from master,
 * 	NO_REPLY (-1) if no reply is sent to the master
 * 	an exception code (1 to 4) in case of a modbus exceptions
 * 	the number of bytes sent as reply ( > 4) if OK.
 */
  int update(int *regs, unsigned int regs_size); 

  // empty constructor
  ModbusSlave()
  {
  
  }

};

#endif
