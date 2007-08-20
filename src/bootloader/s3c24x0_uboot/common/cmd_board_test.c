/*********************************************************************************
*
*
*
*********************************************************************************/
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/s3c2413.h>
#include <s3c2413.h>
#include	<board_test.h>
#include "../drivers/cs8900.h"
#include <usb_2413.h>
#include <usb_2413_setup.h>
#include <s3c24x0_usb.h>
#include <spi.h>
#include <mmc_2413.h>
#if (CONFIG_COMMANDS & CFG_CMD_BOARD_TEST)


////////////////////////////////////////////////
static int toggle_value;
int upgrade_status;
unsigned long upgrade_size;
static int	Battery_Ir_mode_flag;


static int	time_delay_test;

//USB
static unsigned int ep0State;


////////////////////////////////
////////////////////////////////////////////
void boardtest_main(void);
int boardtest_menu(void);

///////
/////////////////////////////////////////////////////
void	spi_wlan_test_main(void);
int spi_wlan_test_menu(void);

//////////////////////////////////////////////
int	do_boardupgrade(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	boardtest_main();
	return 0;
}

void boardtest_main(void)
{
	int key;
	while((key = boardtest_menu()) != 0)
	{
		if(key != 'q')
		{
			switch(key)
		        {
		                case '0':		//uboot upgrade
		                	printf("uboot upgrade \n");
					upgrade_status = 0;	
					upgrade_size = 0;
					if(usb_upgrade_dnw(0) == 0)
					{
						while(upgrade_status == 0);
						if(upgrade_size != 0 && upgrade_size < 0x25000)
						{							
							if(upgrade_status == 1)
							{
								(void)nandw_upgrade(0,upgrade_size, USBD_DOWN_ADDR);
							}
							else
							{
								printf("upgrade_status fail \n");
							}
						}
						else
							printf("upgrade_size fail size = 0x%x \n", upgrade_size);
					}
					else
					{
						printf("usb_upgrade_dnw fail \n");
					}
		                	break;
		                case '1':    	//kernel upgrade  
		                	printf("kernel upgrade \n");
					upgrade_status = 0;	
					upgrade_size = 0;
					if(usb_upgrade_dnw(1) == 0)
					{
						while(upgrade_status == 0);
						if(upgrade_size != 0 && upgrade_size < 0x150000)
						{						
							if(upgrade_status == 1)
							{
								(void)nandw_upgrade(0x0c,upgrade_size, USBD_DOWN_ADDR);
							}
							else
							{
								printf("upgrade_status fail \n");
							}
						}
						else
							printf("upgrade_size fail size = 0x%x \n", upgrade_size);						
					}
					else
					{
						printf("usb_upgrade_dnw fail \n");
					}
		                        break;
		                case '2':	//root file system upgrade
		                	printf("root file system upgrade \n");
					upgrade_status = 0;	
					upgrade_size = 0;
					if(usb_upgrade_dnw(2) == 0)
					{
						while(upgrade_status == 0);
						if(upgrade_size != 0) //  && upgrade_size > 0x150000)
						{							
							if(upgrade_status == 1)
							{
								(void)nandw_upgrade(0x80,upgrade_size, USBD_DOWN_ADDR);
							}
							else
							{
								printf("upgrade_status fail \n");
							}
						}
						else
							printf("upgrade_size fail size = 0x%x \n", upgrade_size);						
					}
					else
					{
						printf("usb_upgrade_dnw fail \n");
					}
					break;
		                case '3':
					printf("not used \n");
					spi_wlan_test_main();
		                     break;
		                case '4':
					printf("not used \n");
					break;
	                       case '5':
					printf("not used \n");
		                        break;
	                       case '6':
					printf("not used \n");
		                        break;
	                       case '7':
					printf("not used \n");
		                        break;
		               case '8':
					printf("not used \n");
					break;
		               case '9':
		                   printf("restart \n");
				     (void)go_upgrade();
		                    break;
				
			}
		}
		else
		{
			printf("quit\n");

			return ;
		}
	}
}

int boardtest_menu(void)
{
        int c;        
        printf("\n");
        printf("idcode = %x\n", GSTATUS1);
		
        printf("********** main menu **********\n");
        printf("* 0.  U-BOOT Upgrade                     *\n");
        printf("* 1.  Kernel Upgrade                     *\n");
        printf("* 2.  root file system upgrade   *\n");
        printf("* 3.  spi_wlan_test *\n");
        printf("* 4.  not used   *\n");
        printf("* 5. not used        *\n");
        printf("* 6.  not used                     *\n");
        printf("* 7.  not used                     *\n");
        printf("* 8.  not used                  *\n");
        printf("* 9.  not   used            *\n");
	 printf("* q.  QUIT                      *\n");
        printf("*******************************\n");
        printf("\n\n");
        

        printf("select the command number : "); 
        
	 c= serial_getc();
        printf("\n\n");

        return c;
}
//============================================================//
//=============== spi_wlan_test_main test ======================================//
//============================================================//
void	spi_wlan_test_main(void)
{
	int spi_char;

	printf("spi_wlan menu main\n");
	while((spi_char = spi_wlan_test_menu()) != 0)
	{
		if(spi_char != 'q')
		{	
			switch(spi_char)
		        {
		                case '0':   
					{
						SPI_Port_Init(0);
						SPI_Baud_Rate_Set(0, 8000000);			// SPI Channel 0, 10MHz

						rSPCON0=(0<<5)|(1<<4)|(1<<3)|(1<<2)|(0<<1)|(0<<0);//Polling,en-SCK,master,low,A,normal
						rSPPIN0=(0<<2)|(1<<1)|(0<<0);//dis-ENMUL,SBO,release
				  
		                	}
		                	break;
		                case '1': 
					printf("tx rx test \n");
					spi0TxStr[0] = 0xff;
					endSpi0Tx = 1;
					while(endSpi0Tx==1)
					{
						printf("Check Tx ready state \n");
						if(REDY0)   //Check Tx ready state
						{	
							printf("Check Tx ready state \n");
							if(spi0TxStr[0] == 0xff)
							{
								rSPTDAT0= spi0TxStr[0];
								printf("tx data 0x%x rSPTDAT0 0x%x \n",spi0TxStr[0],rSPTDAT0);
							}
							else 
							{
								endSpi0Tx=0;
								printf("endSpi0Tx=0 \n");
								break;
							}
							while(!(REDY0_org));   //Check Rx ready state 
							spi0RxStr[0]=rSPRDATB0;
							printf("rx data 0x%x \n",spi0RxStr[0]);
						}
					}						
		                     break;
		                case '2':
						printf("not used 2 \n");
					break; 
		                case '3':
					printf("mmc init  \n");		
					MMC_FLASH_Init();
		                        break;
		                case '4':
					Master_nSS_Con0(0);
					break;
	                       case '5':
		                        printf("not used 5 \n");
		                        break;
	                       case '6':
		                        printf("not used 6\n");
		                        break;
	                       case '7':
		                        printf("not used 7\n");
		                        break;
		               case '8':
		                    printf("not used 8\n");
		                    break;
		               case '9':
		                    printf("not used 9\n");
		                    break;
			}
		}
		else
		{
			printf("quit\n");

			return ;
		}			
	}
}

int spi_wlan_test_menu(void)
{
        int c;        
        printf("\n");
        printf("********** timer menu *****\n");
        printf("* 0.  spi_ start                      *\n");
        printf("* 1.  tx rx test                      *\n");
        printf("* 2.  not used                      *\n");
        printf("* 3.  wlan_test_Init           *\n");
        printf("* 4.  nSS low                     *\n");
        printf("* 5.  not used                      *\n");
        printf("* 6.  not used                     *\n");
        printf("* 7.  not used                      *\n");
        printf("* 8.  not used                     *\n");
        printf("* 9.  not used                  *\n");
        printf("* q.  QUIT                      *\n");
        printf("*******************************\n");
        printf("select the timer command number : ");
        
	 c= serial_getc();
        printf("\n\n");

        return c;
}

U_BOOT_CMD(
        upgrade, 1,       1,      do_boardupgrade,
        "upgrade   - Upgrade default, i.e., run 'upgrade'\n",
        NULL
);


#endif




