#include <ModbusSlave.h>

/* First step MBS: create an instance */
ModbusSlave mbs;

/* slave registers */
enum {        
        MB_CTRL,        /* Led control on, off or blink */
        MB_TIME,        /* blink time in milliseconds */
        MB_CNT,        /* count the number of blinks */
        MB_REGS	 	/* total number of registers on slave */
};

int regs[MB_REGS];
int ledPin = 13;
unsigned long wdog = 0;         /* watchdog */
unsigned long tprev = 0;         /* previous time*/

void setup() 
{
        
/* the Modbus slave configuration parameters */
const unsigned char SLAVE = 1;
const long BAUD = 115200;
const char PARITY = 'e';
const char TXENPIN = 0;

       /* Second step MBS: configure */
      mbs.configure(SLAVE,BAUD,PARITY,TXENPIN);

        pinMode(ledPin, OUTPUT);
        
}

void loop()
{
        /* Third and las step MBS: update in loop*/
        if(mbs.update(regs, MB_REGS))
                wdog = millis();

        if ((millis() - wdog) > 5000)  {      /* no comms in 5 sec */
                regs[MB_CTRL] = 0;	/* turn off led */
        }        

        /* the values in regs are set by the modbus master */
        switch(regs[MB_CTRL]) {
        case 0:
                digitalWrite(ledPin, LOW);
                break;
        case 1:
                digitalWrite(ledPin, HIGH);
                break;        
        default: /* blink */
                if (millis() - tprev > regs[MB_TIME]) {
                        if (LOW == digitalRead(ledPin)) {
                                digitalWrite(ledPin, HIGH);
                                /* this is how you change your holding registers
                                  so the master can read values from the slave */
                                regs[MB_CNT]++;
                        } else {
                                digitalWrite(ledPin, LOW);
                        }
                        tprev = millis();
                }
        }

}


