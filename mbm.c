#include "modbus_rtu.h"
#include <unistd.h>
#include <stdio.h>

#define COMM_PORT		"/dev/ttyUSB0"
#define COMM_PARITY		"even"

/* Modbus RTU common parameters, the Slave MUST use the same parameters */
enum {
        COMM_BPS = 115200,
        MB_SLAVE = 1,	/* modbus slave id */
};
/* slave 1 registers */
enum {        
        MB_CTRL,        /* Led control on, off or blink */
        MB_TIME,        /* blink time in milliseconds */
        MB_CNT,        /* count the number of blinks */        
        MB_REGS	 	/* total number of registers on slave */
};

int main(int argc, char *argv[])
{
	int err = 0;
	int val = 0;
	int blink[MB_REGS];
	int fd = set_up_comms(COMM_PORT, COMM_BPS, COMM_PARITY);

	/* turn off Arduino led */
    	preset_single_register(MB_SLAVE,(MB_CTRL+1), val, fd);

	sleep(1);
	
	/* turn it on for 2 secs */
	val = 1;
	preset_single_register(MB_SLAVE,(MB_CTRL+1), val, fd);
	sleep(2);
	
	/* blink for 2 secs */
	blink[MB_CTRL] = 2;
	blink[MB_TIME] = 200;
	preset_multiple_registers(MB_SLAVE,(MB_CTRL+1), 2, blink, fd);
	sleep(2);

	/* read the slave's registers */	
	err = read_holding_registers( MB_SLAVE, (MB_CTRL+1), MB_REGS, blink, MB_REGS,fd);
	printf("\n The blink count at this time is %d \n\n", blink[MB_CNT]);

	/* blink one more second */		  	
	sleep(1);

	/* blink a little bit slower until the watchdog turns off the LED */
	blink[MB_CTRL] = 2;
	blink[MB_TIME] = 500;
	preset_multiple_registers(MB_SLAVE,(MB_CTRL+1), 2, blink, fd);
	sleep(1);
	
	/* read the slave's registers */
	err = read_holding_registers( MB_SLAVE, (MB_CTRL+1), MB_REGS, blink, MB_REGS,fd);
	printf("\n The blink count at this time is %d \n\n", blink[MB_CNT]);

	/* On ending this program, the slave's watchdog will start.
	 * 10 secs later the led will turn off
	 */
	
	return 0;
}
