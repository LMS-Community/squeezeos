/*
        The purpose of this file is to monitor the MSP430 and to make sure that the communication between iMX25 and the MSP430 is fine.
        If the communication is not functional for a period of time then the MSP430 is marked as failure and this process exits.
        It is the responsibility of the watchdog to monitor this process to make sure that it is running.
        As soon as this process exits the watchdog knows that this process is not running anymore and would make proper decision
        ( which could be to reset the system ).
*/


/*
        Include files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/*
        Macros
*/


#define MSP430_FW_I2C_DEV             "/sys/bus/i2c/drivers/msp430/1-0010/fw"
#define MAX_CONSECUTIVE_FAIL_COUNT    30
#define MODULE_NAME                   "MONITOR_MSP430"


//#define DEBUG

/*!
        Monitors the MSP430 by reading its firmware revision. This causes i2c issues to the MSP430 to read the information.
        If the request fails for a specified number of times consecutively then the MSP430 is considered failed.
        This function returns only if it declraes the MSP430 as failed.
*/

void monitor_msp430 ( void )
{
        int  fd;
        char buf[10];
        int  ret;
        int  consecutive_fail_count;


        consecutive_fail_count = 0;

        while ( 1 )
        {
                /*
                        To make sure that the MSP430 is functional open and read from the MSP430_FW_I2C_DEV device.
                        If we open the device only once and issue read commands there are times that we get stale data.
                        That is the reason we open the device every time. Since this action is done only once every
                        second there should not be any overhead. 
                */

                sleep ( 1 );
                fd = open ( MSP430_FW_I2C_DEV, O_RDONLY );
                if ( fd < 0 )
                {
                        fprintf ( stderr, "%s: Failed to open the device %s, error ( %s )\n", MODULE_NAME, MSP430_FW_I2C_DEV, strerror ( errno ));
                        ret = -1;
                }
                else
                {
                        ret = read ( fd, buf, sizeof ( buf )); 
#ifdef DEBUG
                        if ( ret > 0 )
                        {
                                /*
                                        Firmware version has a newline at the end. Remove that as well.
                                */

                                buf[ret-1] = 0;
                                fprintf ( stdout, "%s: %d bytes read from the device. The data read from the fw is %s\n", MODULE_NAME, ret, buf );
                        }
                        else
                                fprintf ( stderr, "%s: Failed to read from the device %s, error ( %s )\n", MODULE_NAME, MSP430_FW_I2C_DEV, strerror ( errno ));
#endif
                        close ( fd );
                }

                /*
                        Check if the request to communicate to MSP430 is failed for any reason.
                */

                if ( ret < 0 )
                {
                        /*
                                The communication to MSP430 is failed.
                                Increase the failed count and check if we passed the threshold.
                        */

                        consecutive_fail_count++;

                        if ( consecutive_fail_count > MAX_CONSECUTIVE_FAIL_COUNT )
                        {
                                fprintf ( stdout, "%s: Too many consecutive communication failure to MSP430. Consider it failed\n", MODULE_NAME );
                                break;
                        }
                }
                else
                {
                        /*
                                The communication to MSP430 is working fine. Clear out any history of failed communication counts. 
                        */

                        consecutive_fail_count = 0;
                }
        }
        return;
}


int main (int argc, char **argv)
{
        /*
                Monitor the MSP430.
        */

        monitor_msp430 ();

        return 0;
}
