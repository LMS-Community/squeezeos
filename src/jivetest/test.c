//===================================================================
// 테스트 프로그램
//===================================================================

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h> 
#include <sys/stat.h>

#include <linux/fb.h> 
#include <linux/rtc.h>

#include "test.h"

//===================================================================
int GETRTC_Time(void);
int SETRTC_Time(int year, int mon, int day, int hour, int min, int sec );

//===================================================================
int main(void)
{
	int TestTemp;
	int Key;
	int fd_matrix, fd_led, fd_fb, fd_hold, fd_bat, fd_lcdPwr, fd_wheel,	fd_pmode;
	int fd_hdet,fd_dacp, fd_ampp, fd_mute, fd_wlanp, fd_motion, fd_lcdb, fd_irtx, fd_irrx;
	int fd_rtc;
	int t, tt, offset, ret, temp, i, pmode_flag;
	int rtc1,rtc2,rtc3,rtc4,rtc5;       
	int Tyear, Tmon, Tday, Thour, Tmin,Tsec;
	int	Rx_count;


	unsigned char key, temp_key, key_flag = 0;
	unsigned char buffer[1], bat_ad[1];
	unsigned char hold_buffer[1];
	unsigned int Tx[100];
	unsigned int Rx[1000];
	int DrvCount;
	
	struct fb_var_screeninfo fbvar;
	struct rtc_time *time;
	unsigned short *pfbdata;
	unsigned short pixeldat;

	unsigned char lcdb_data = 0xff;
	
	int count = 1000;	
	
  	fd_matrix = open("/dev/MATRIX", O_RDWR );
	fd_led = open("/dev/LED", O_RDWR);
	fd_fb = open("/dev/fb/0", O_RDWR);
	fd_hold = open("/dev/HOLD", O_RDWR);
	fd_bat = open("/dev/BATTERY", O_RDWR);
	fd_hdet = open("/dev/HDET", O_RDWR );
	fd_lcdPwr = open("/dev/LCDP", O_RDWR );
	fd_dacp = open("/dev/DACP", O_RDWR );
	fd_ampp = open("/dev/AMPP", O_RDWR );
	fd_mute = open("/dev/MUTE", O_RDWR );
	//fd_wlanp = open("/dev/WLANP", O_RDWR );
	fd_motion = open("/dev/MOTION", O_RDWR );
	fd_lcdb = open("/dev/LCDB", O_RDWR );	
	fd_irtx= open("/dev/IRTX", O_RDWR );	
	fd_irrx= open("/dev/IRRX", O_RDWR );	
	//fd_rtc = open("/dev/misc/rtc", O_RDWR );	
	fd_wheel = open("/dev/WHEEL", O_RDWR );	
	fd_pmode = open("/dev/PMODE", O_RDWR );	
	
 	if (fd_matrix <0) {perror("/dev/MATRIX"); exit(-1); }
  	if (fd_led <0) {perror("/dev/LED"); exit(-1); }
    if (fd_fb<0) {perror("/dev/fb/0"); exit(-1); }
	if (fd_hold<0) {perror("/dev/HOLD"); exit(-1); }
	if (fd_bat<0) {perror("/dev/BATTERY"); exit(-1); }
	if (fd_hdet <0) {perror("/dev/HDET"); exit(-1); }
 	if (fd_lcdPwr <0) {perror("/dev/LCDP"); exit(-1); }
 	if (fd_dacp <0) {perror("/dev/DACP"); exit(-1); }
 	if (fd_ampp <0) {perror("/dev/AMPP"); exit(-1); }
 	if (fd_mute <0) {perror("/dev/MUTE"); exit(-1); }
 	//if (fd_wlanp <0) {perror("/dev/WLANP"); exit(-1); }
 	if (fd_motion <0) {perror("/dev/MOTION"); exit(-1); }
 	if (fd_lcdb <0) {perror("/dev/LCDB"); exit(-1); }
 	if (fd_irtx<0) {perror("/dev/IRTX"); exit(-1); }
 	if (fd_irrx<0) {perror("/dev/IRRX"); exit(-1); }
 	//if (fd_rtc <0) {perror("/dev/misc/rtc"); exit(-1); }
	if (fd_wheel <0) {perror("/dev/WHEEL"); exit(-1); }
	if (fd_pmode <0) {perror("/dev/PMODE"); exit(-1); }

	ret = ioctl(fd_fb, FBIOGET_VSCREENINFO, &fbvar);
	if(ret < 0) {perror("fbdev ioctl");exit(1);}
	
	if(fbvar.bits_per_pixel != 16) {fprintf(stderr, "bpp is not 16\n");exit(1);}

	pfbdata = (unsigned short *)mmap(0, fbvar.xres*fbvar.yres*16/8,PROT_READ|PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if((unsigned)pfbdata == (unsigned)-1) {perror("fbdev mmap");exit(1);}

	while((key = _menuTest()) != 0)
	{
		switch (key)
		{
			case KEYTestMenu:
				TestTemp=1;
				while (TestTemp) {
					switch (_KEY_Test_Str_menu()) {	
						case MatrixKeyInput:
							read(fd_matrix, buffer, 1);
							printf("\n pushing key ==> %d", buffer[0]);
							break;
						case ScrollKeyInput:
						{
 							read(fd_wheel, buffer, 1);
							switch(buffer[0])
							{
								case 0:
									printf("No move\n");
									break;
								case 1:
									printf("Left \n");
									break;
								case 2:
									printf("Right \n");
									break;
								default:
									printf("Error\n");
									break;
							}
							break;
						}

						case HoldKeyInput:
							read(fd_hold, buffer, 1);
							//printf("\n hold key status : Low %d", buffer[0]);	
							
							if (buffer[0]) printf("\n hold key status : High");
							else printf("\n hold key status : Low");	
							
							break;
							
						case BackPosition:
							TestTemp=0;
							break;
						default:
							break;
					}
				}	
				break;
			case ADCTestMenu:
				TestTemp=1;
				while (TestTemp)
				{
					switch (_ADC_Test_Str_menu())
					{
						case BatteryADRead:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_ADC_Battery_Str_menu()) 
								{
									case PortDriveRead:
 										read(fd_bat, buffer, 1);
										printf("Battery remain : 0x%x\n",buffer[0]);
										break;
									case BackPosition:
										printf("Back Position\n");
										TestTemp=0;
										break;
									default:
										break;
								}
							}
						case 'B':
							TestTemp=0;
							break;
						default:
							break;
					}
				}	
				break;
			case PWMTestMenu:
				TestTemp=1;
				while (TestTemp)
				{
					switch (_PWM_Test_Str_menu())
					{
						case LCDBacklightControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_PWM_LCDB_Str_menu()) 
								{
									case '1':
										if (lcdb_data != 0xff) lcdb_data+=0x10;
										buffer[0] = lcdb_data;
										write(fd_lcdb, buffer, 1);
										printf("LCDB_data : 0x%x\n",buffer[0]);
										break;
									case '2':
										if (lcdb_data != 0x00) lcdb_data-=0x10;
										buffer[0] = lcdb_data;
										write(fd_lcdb, buffer, 1);
										printf("LCDB_data : 0x%x\n",buffer[0]);
										break;
									case 'B':
										TestTemp=0;
										break;										
									default:
										break;
								}
							}
						case 'B':
							TestTemp=0;
							break;
						default:
							break;
					}
				}	
				break;
			case GPIOTestMenu:
				TestTemp=1;
				while (TestTemp)
				{
					switch (_GPIO_Test_Str_menu())
					{
						case KeyBacklightControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_Led_Str_menu()) 
								{
									case PortDriveOff:
										printf("KeyBacklightControl Power On\n");
										buffer[0] = 0x00;
										write(fd_led, buffer, 1);
										break;
									case PortDriveOn:
										printf("KeyBacklightControl Power Off\n");
										buffer[0] = 0x01;
										write(fd_led, buffer, 1);
										break;
									case BackPosition:
										TestTemp=0;
										printf("Back Position\n");
										break;
									default:
										break;
								}
							}
							break;
						case HeadphoneDetection:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_HeadphoneDetection_Str_menu()) 
								{
									case PortDriveRead:
 										read(fd_hdet, buffer, 1);
										
										if (buffer[0] == 0x01) printf("Headphone unplugged\n");
										else printf("Headphone plugged\n");
										break;
									case BackPosition:
										printf("Back Position\n");
										TestTemp=0;
										break;
									default:
										break;
								}
							}
							break;
						case LCDPowerControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_LcdPower_Str_menu())	
								{
									case PortDriveOff:
										printf("LCD Power Off\n");
										buffer[0] = 0x00;
										write(fd_lcdPwr, buffer, 1);
										break;
									case PortDriveOn:
										printf("LCD Power On\n");
										buffer[0] = 0x01;
										write(fd_lcdPwr, buffer, 1);
										break;
									case BackPosition:
										TestTemp=0;
										printf("Back Position\n");
										break;
									default:
										break;
								}
							}
							break;
						case DACPowerControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_DACPower_Str_menu())	
								{
									case PortDriveOff:
										printf("DAC Power Off\n");
										buffer[0] = 0x00;
										write(fd_dacp, buffer, 1);
										break;
									case PortDriveOn:
										printf("DAC Power On\n");
										buffer[0] = 0x01;
										write(fd_dacp, buffer, 1);
										break;
									case BackPosition:
										TestTemp=0;
										printf("Back Position\n");
										break;
									default:
										break;
								}
							}
							break;
						case AMPPowerControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_AMPPower_Str_menu())	
								{
									case PortDriveOff:
										printf("AMP Power Off\n");
										buffer[0] = 0x00;
										write(fd_ampp, buffer, 1);
										break;
									case PortDriveOn:
										printf("AMP Power On\n");
										buffer[0] = 0x01;
										write(fd_ampp, buffer, 1);
										break;
									case BackPosition:
										TestTemp=0;
										printf("Back Position\n");
										break;
									default:
										break;
								}
							}
							break;
						case MuteTRControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_MuteTR_Str_menu())	
								{
									case PortDriveOff:
										printf("Mute TR Off\n");
										buffer[0] = 0x00;
										write(fd_mute, buffer, 1);
										break;
									case PortDriveOn:
										printf("Mute TR On\n");
										buffer[0] = 0x01;
										write(fd_mute, buffer, 1);
										break;
									case BackPosition:
										TestTemp=0;
										printf("Back Position\n");
										break;
									default:
										break;
								}
							}
							break;
						case WlanPowerControl:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_WlanPower_Str_menu())	
								{
									case PortDriveOff:
										printf("WlanPower Off\n");
										buffer[0] = 0x00;
										write(fd_wlanp, buffer, 1);
										break;
									case PortDriveOn:
										printf("WlanPower On\n");
										buffer[0] = 0x01;
										write(fd_wlanp, buffer, 1);
										break;
									case BackPosition:
										TestTemp=0;
										printf("Back Position\n");
										break;
									default:
										break;
								}
							}
							break;
						case MotionSensor:
							TestTemp=1;
							while(TestTemp)
							{
								switch (_GPIO_MotionSensor_Str_menu())	
								{
									case PortDriveRead:
 										read(fd_motion, buffer, 1);
										if (buffer[0] == 0x01) printf("Port Driver High\n");
										else printf("Port Driver Low\n");
										break;
									case BackPosition:
										printf("Back Position\n");
										TestTemp=0;
										break;
									default:
										break;
								}
							}
							break;
						case 'B':
							TestTemp=0;
							break;
						default:
							break;
						}
					}	
				break;
			case '5':
				break;
			case LCDTestMenu:
				TestTemp=1;
				while (TestTemp)
				{
					switch (_LCD_Test_Str_menu())
					{
						case RandomRectangle:
							srand(1); /* seed for rand */
							while(0 < count--)
							{
								int xpos1, ypos1;
								int xpos2, ypos2;
								int offset;
								int rpixel;
								int t, tt;
			
								/* random number between 0 and xres */
								xpos1 = (int)((fbvar.xres*1.0*rand())/(RAND_MAX+1.0));
								xpos2 = (int)((fbvar.xres*1.0*rand())/(RAND_MAX+1.0));
			
								/* random number between 0 and yres */
								ypos1 = (int)((fbvar.yres*1.0*rand())/(RAND_MAX+1.0));
								ypos2 = (int)((fbvar.yres*1.0*rand())/(RAND_MAX+1.0));

								if(xpos1 > xpos2)
								{
									t = xpos1;
									xpos1 = xpos2;
									xpos2 = t;
								}

								if(ypos1 > ypos2)
								{
									t = ypos1;
									ypos1 = ypos2;
									ypos2 = t;
								}

								rpixel = (int)(65536.0*rand()/(RAND_MAX+1.0));
				
								for(t = ypos1; t <= ypos2; t++)
								{
									offset = t*fbvar.xres;
			
									for(tt = xpos1; tt <= xpos2; tt++)
										*(pfbdata+offset+tt) = rpixel;;
								}					
							}
							count = 1000;							
							break;
						case RGBtest:
							break;
						case 'B':
							TestTemp=0;
							break;							
						default:
							break;
						}
					}	
				break;
			case IRTestMenu:
				TestTemp=1;
				while (TestTemp)
				{
					switch (_IR_TRxTest_Str_menu())
					{
						case IRTxTest_1:	// 0x070702FD Samsung Power Code
							DrvCount=66;
			/* Reder */		Tx[0]=4500;	Tx[1]=4500;
			/* Bin Data		0000 0111 0000 0111 0000 0010 1111 1101 => 1110 0000 1110 0000 0100 0000 1011 1111 */
			/* Cus1 1110 */ 	Tx[2]=560; 	Tx[3]=1690;	Tx[4]=560;	Tx[5]=1690;	Tx[6]=560;	Tx[7]=1690;	Tx[8]=560;	Tx[9]=565;
			/* Cus2 0000 */	Tx[10]=560;	Tx[11]=565;	Tx[12]=560;	Tx[13]=565;	Tx[14]=560;	Tx[15]=565;	Tx[16]=560;	Tx[17]=565;
			/* Cus3 1110 */ 	Tx[18]=560;	Tx[19]=1690;	Tx[20]=560;	Tx[21]=1690;	Tx[22]=560;	Tx[23]=1690;	Tx[24]=560;	Tx[25]=565;
			/* Cus4 0000 */	Tx[26]=560;	Tx[27]=565;	Tx[28]=560;	Tx[29]=565;	Tx[30]=560;	Tx[31]=565;	Tx[32]=560;	Tx[33]=565;

			/* data1 0100 */ 	Tx[34]=560;	Tx[35]=565;	Tx[36]=560;	Tx[37]=1690;	Tx[38]=560;	Tx[39]=565;	Tx[40]=560;	Tx[41]=565;
			/* data2 0000 */ 	Tx[42]=560;	Tx[43]=565;	Tx[44]=560;	Tx[45]=565;	Tx[46]=560;	Tx[47]=565;	Tx[48]=560;	Tx[49]=565;
			/* data3 1011 */ 	Tx[50]=560;	Tx[51]=1690;	Tx[52]=560;	Tx[53]=565;	Tx[54]=560;	Tx[55]=1690;	Tx[56]=560;	Tx[57]=1690;
			/* data4 1111 */ 	Tx[58]=560;	Tx[59]=1690;	Tx[60]=560;	Tx[61]=1690;	Tx[62]=560;	Tx[63]=1690;	Tx[64]=560;	Tx[65]=1690;
							write(fd_irtx, Tx, DrvCount);
							printf("IR Tx OK\n");
							break;

						case IRTxTest_2:	// 0x070710EF Ch Up
							DrvCount=66;
			/* Reder */		Tx[0]=4500;	Tx[1]=4500;
			/* Bin Data		0000 0111 0000 0111 0001 0000 1110 1111 => 1110 0000 1110 0000 0000 1000 1111 0111 */
			/* Cus1 1110 */ 	Tx[2]=560; 	Tx[3]=1690;	Tx[4]=560;	Tx[5]=1690;	Tx[6]=560;	Tx[7]=1690;	Tx[8]=560;	Tx[9]=565;
			/* Cus2 0000 */	Tx[10]=560;	Tx[11]=565;	Tx[12]=560;	Tx[13]=565;	Tx[14]=560;	Tx[15]=565;	Tx[16]=560;	Tx[17]=565;
			/* Cus3 1110 */ 	Tx[18]=560;	Tx[19]=1690;	Tx[20]=560;	Tx[21]=1690;	Tx[22]=560;	Tx[23]=1690;	Tx[24]=560;	Tx[25]=565;
			/* Cus4 0000 */	Tx[26]=560;	Tx[27]=565;	Tx[28]=560;	Tx[29]=565;	Tx[30]=560;	Tx[31]=565;	Tx[32]=560;	Tx[33]=565;

			/* data1 0000 */ 	Tx[34]=560;	Tx[35]=565;	Tx[36]=560;	Tx[37]=565;	Tx[38]=560;	Tx[39]=565;	Tx[40]=560;	Tx[41]=565;
			/* data2 1000 */ 	Tx[42]=560;	Tx[43]=1690;	Tx[44]=560;	Tx[45]=565;	Tx[46]=560;	Tx[47]=565;	Tx[48]=560;	Tx[49]=565;
			/* data3 1111 */ 	Tx[50]=560;	Tx[51]=1690;	Tx[52]=560;	Tx[53]=1690;	Tx[54]=560;	Tx[55]=1690;	Tx[56]=560;	Tx[57]=1690;
			/* data4 0111 */ 	Tx[58]=560;	Tx[59]=565;	Tx[60]=560;	Tx[61]=1690;	Tx[62]=560;	Tx[63]=1690;	Tx[64]=560;	Tx[65]=1690;
							write(fd_irtx, Tx, DrvCount);
							printf("IR Tx OK\n");
							break;

						case IRTxTest_3:	// 0x07071AE5 Ch Down
							DrvCount=66;
			/* Reder */		Tx[0]=4500;	Tx[1]=4500;
			/* Bin Data		0000 0111 0000 0111 0001 1010 1110 0101 => 1110 0000 1110 0000 0101 1000 1010 0111 */
			/* Cus1 1110 */ 	Tx[2]=560; 	Tx[3]=1690;	Tx[4]=560;	Tx[5]=1690;	Tx[6]=560;	Tx[7]=1690;	Tx[8]=560;	Tx[9]=565;
			/* Cus2 0000 */	Tx[10]=560;	Tx[11]=565;	Tx[12]=560;	Tx[13]=565;	Tx[14]=560;	Tx[15]=565;	Tx[16]=560;	Tx[17]=565;
			/* Cus3 1110 */ 	Tx[18]=560;	Tx[19]=1690;	Tx[20]=560;	Tx[21]=1690;	Tx[22]=560;	Tx[23]=1690;	Tx[24]=560;	Tx[25]=565;
			/* Cus4 0000 */	Tx[26]=560;	Tx[27]=565;	Tx[28]=560;	Tx[29]=565;	Tx[30]=560;	Tx[31]=565;	Tx[32]=560;	Tx[33]=565;
			
			/* data1 0101 */ 	Tx[34]=560;	Tx[35]=565;	Tx[36]=560;	Tx[37]=1690;	Tx[38]=560;	Tx[39]=565;	Tx[40]=560;	Tx[41]=1690;
			/* data2 1000 */ 	Tx[42]=560;	Tx[43]=1690;	Tx[44]=560;	Tx[45]=565;	Tx[46]=560;	Tx[47]=565;	Tx[48]=560;	Tx[49]=565;
			/* data3 1010 */ 	Tx[50]=560;	Tx[51]=1690;	Tx[52]=560;	Tx[53]=565;	Tx[54]=1690;	Tx[55]=565;	Tx[56]=560;	Tx[57]=565;
			/* data4 0111 */ 	Tx[58]=560;	Tx[59]=565;	Tx[60]=560;	Tx[61]=1690;	Tx[62]=560;	Tx[63]=1690;	Tx[64]=560;	Tx[65]=1690;
							write(fd_irtx, Tx, DrvCount);
							printf("IR Tx OK\n");
							break;
							
						case IRTxTest_4:	//0x07070BF4 Vol Up
							DrvCount=66;
			/* Reder */		Tx[0]=4500;	Tx[1]=4500;
			/* Bin Data		0000 0111 0000 0111 0000 1011 1111 0100 => 1110 0000 1110 0000 1101 0000 0010 1111 */
			/* Cus1 1110 */ 	Tx[2]=560; 	Tx[3]=1690;	Tx[4]=560;	Tx[5]=1690;	Tx[6]=560;	Tx[7]=1690;	Tx[8]=560;	Tx[9]=565;
			/* Cus2 0000 */	Tx[10]=560;	Tx[11]=565;	Tx[12]=560;	Tx[13]=565;	Tx[14]=560;	Tx[15]=565;	Tx[16]=560;	Tx[17]=565;
			/* Cus3 1110 */ 	Tx[18]=560;	Tx[19]=1690;	Tx[20]=560;	Tx[21]=1690;	Tx[22]=560;	Tx[23]=1690;	Tx[24]=560;	Tx[25]=565;
			/* Cus4 0000 */	Tx[26]=560;	Tx[27]=565;	Tx[28]=560;	Tx[29]=565;	Tx[30]=560;	Tx[31]=565;	Tx[32]=560;	Tx[33]=565;

			/* data1 1101 */ 	Tx[34]=560;	Tx[35]=1690;	Tx[36]=560;	Tx[37]=1690;	Tx[38]=560;	Tx[39]=565;	Tx[40]=560;	Tx[41]=1690;
			/* data2 0000 */ 	Tx[42]=560;	Tx[43]=565;	Tx[44]=560;	Tx[45]=565;	Tx[46]=560;	Tx[47]=565;	Tx[48]=560;	Tx[49]=565;
			/* data3 0010 */ 	Tx[50]=560;	Tx[51]=565;	Tx[52]=560;	Tx[53]=565;	Tx[54]=560;	Tx[55]=1690;	Tx[56]=560;	Tx[57]=565;
			/* data4 1111 */ 	Tx[58]=560;	Tx[59]=1690;	Tx[60]=560;	Tx[61]=1690;	Tx[62]=560;	Tx[63]=1690;	Tx[64]=560;	Tx[65]=1690;
							write(fd_irtx, Tx, DrvCount);
							printf("IR Tx OK\n");
							break;
							
						case IRTxTest_5:	//0x07070FF0 Vol Down
							DrvCount=66;
			/* Reder */		Tx[0]=4500;	Tx[1]=4500;
			/* Bin Data		0000 0111 0000 0111 0000 1111 1111 0000 => 1110 0000 1110 0000 1111 0000 0000 1111 */
			/* Cus1 1110 */ 	Tx[2]=560; 	Tx[3]=1690;	Tx[4]=560;	Tx[5]=1690;	Tx[6]=560;	Tx[7]=1690;	Tx[8]=560;	Tx[9]=565;
			/* Cus2 0000 */	Tx[10]=560;	Tx[11]=565;	Tx[12]=560;	Tx[13]=565;	Tx[14]=560;	Tx[15]=565;	Tx[16]=560;	Tx[17]=565;
			/* Cus3 1110 */ 	Tx[18]=560;	Tx[19]=1690;	Tx[20]=560;	Tx[21]=1690;	Tx[22]=560;	Tx[23]=1690;	Tx[24]=560;	Tx[25]=565;
			/* Cus4 0000 */	Tx[26]=560;	Tx[27]=565;	Tx[28]=560;	Tx[29]=565;	Tx[30]=560;	Tx[31]=565;	Tx[32]=560;	Tx[33]=565;

			/* data1 1111 */ 	Tx[34]=560;	Tx[35]=1690;	Tx[36]=560;	Tx[37]=1690;	Tx[38]=560;	Tx[39]=1690;	Tx[40]=560;	Tx[41]=1690;
			/* data2 0000 */ 	Tx[42]=560;	Tx[43]=565;	Tx[44]=560;	Tx[45]=565;	Tx[46]=560;	Tx[47]=565;	Tx[48]=560;	Tx[49]=565;
			/* data3 0000 */ 	Tx[50]=560;	Tx[51]=565;	Tx[52]=560;	Tx[53]=565;	Tx[54]=560;	Tx[55]=565;	Tx[56]=560;	Tx[57]=565;
			/* data4 1111 */ 	Tx[58]=560;	Tx[59]=1690;	Tx[60]=560;	Tx[61]=1690;	Tx[62]=560;	Tx[63]=1690;	Tx[64]=560;	Tx[65]=1690;
							write(fd_irtx, Tx, DrvCount);
							printf("IR Tx OK\n");
							break;
				
						case IRRxTest_6:
 							read(fd_irrx, Rx, 4000);
							Rx_count = 0;
							while(Rx_count<100){
								printf("Remocon Receiver : RxIndex [ %d ], read time [ %d ]\n",Rx_count,Rx[Rx_count]);
								Rx_count++;
							}								
							break;
							
						case IRRxTest_7:
							break;
						case IRRxTest_8:
							break;
						case IRRxTest_9:
							break;
						case BackPosition:
							TestTemp=0;
							break;
						default:
							break;
					}
				}	
				break;	
			case POWERTestMenu:
				TestTemp=1;
				while (TestTemp) {
					switch (_Power_Test_Str_menu()) {	
						case Power_All_On:
							printf("\n Board status is All power On");
							printf("\n ---------------------------- \n");
							printf("   LCD pwr On  \n");
							printf("   Wlan pwr On  \n");
							printf("   Key Backlit On  \n");
							printf("   DAC pwr On  \n");
							printf("   Mute TR Off  \n");
							printf("   AMP pwr On  \n");
							printf("   LCD Backlit Max  \n");
							ioctl(fd_pmode, 1, pmode_flag);
							break;
						case Power_LCD_Off:
							printf("\n Board status is only LCD power Off");
							printf("\n ---------------------------- \n");
							printf("   LCD pwr Off  \n");
							printf("   Wlan pwr On  \n");
							printf("   Key Backlit Off  \n");
							printf("   DAC pwr On  \n");
							printf("   Mute TR Off  \n");
							printf("   AMP pwr On  \n");
							printf("   LCD Backlit Min  \n");
							ioctl(fd_pmode, 2, pmode_flag);
							break;
						case Power_All_Off:
							printf("\n Board status is All power Off");
							printf("\n ---------------------------- \n");
							printf("   LCD pwr Off  \n");
							printf("   Wlan pwr Off  \n");
							printf("   Key Backlit Off  \n");
							printf("   DAC pwr Off  \n");
							printf("   Mute TR On  \n");
							printf("   AMP pwr Off  \n");
							printf("   LCD Backlit Min  \n");
							ioctl(fd_pmode, 3, pmode_flag);
							break;
							
						case BackPosition:
							TestTemp=0;
							break;
						default:
							break;
					}
				}	
				break;
		
			case RTCTestMenu:
				TestTemp=1;
				while (TestTemp)
				{
					switch (_RTC_Test_Str_menu())
					{
						case SETRTC: 
        						printf("\n");
        						printf("--------------------------------------------------------------------------\n");
        						printf(" Year [XXXX] input = ");		// year
	 						rtc1= getchar();	rtc2= getchar();	rtc3= getchar();	rtc4= getchar();	getchar();			// Enter Code
							rtc1=(rtc1-48)*1000;	rtc2=(rtc2-48)*100;	rtc3=(rtc3-48)*10;		rtc4=(rtc4-48);
							Tyear=rtc1+rtc2+rtc3+rtc4;
							printf(" Set Data [%d/--/--, --:--:--]",Tyear);
							
        						printf(" Month [XX] input = ");		// Month
	 						rtc1= getchar();	rtc2= getchar();	getchar();
							rtc1=(rtc1-48)*10;		rtc2=(rtc2-48);
							Tmon=rtc1+rtc2;
							printf(" Set Data [%d/%d/--, --:--:--]",Tyear,Tmon);
							
        						printf(" Day [XX] input = ");			// day
	 						rtc1= getchar();	rtc2= getchar();	getchar();
							rtc1=(rtc1-48)*10;		rtc2=(rtc2-48);
							Tday=rtc1+rtc2;
							printf(" Set Data [%d/%d/%d, --:--:--]",Tyear,Tmon,Tday);
							
        						printf(" hour [XX] input = ");			// hour
	 						rtc1= getchar();	rtc2= getchar();	getchar();
							rtc1=(rtc1-48)*10;		rtc2=(rtc2-48);
							Thour=rtc1+rtc2;
							printf(" Set Data [%d/%d/%d, %d:--:--]",Tyear,Tmon,Tday,Thour);
							
        						printf(" min [XX] input = ");			// min
	 						rtc1= getchar();	rtc2= getchar();	getchar();
							rtc1=(rtc1-48)*10;		rtc2=(rtc2-48);
							Tmin=rtc1+rtc2;
							printf(" Set Data [%d/%d/%d, %d:%d:--]",Tyear,Tmon,Tday,Thour,Tmin);
							
        						printf(" sec [XX] input = ");			// sec
	 						rtc1= getchar();	rtc2= getchar();	getchar();
							rtc1=(rtc1-48)*10;		rtc2=(rtc2-48);
							Tsec=rtc1+rtc2;
							printf(" Set Data [%d/%d/%d, %d:%d:%d] Complete",Tyear,Tmon,Tday,Thour,Tmin,Tsec);
							
   							SETRTC_Time( Tyear, Tmon, Tday, Thour, Tmin, Tsec );
							
  							//GETRTC_Time( );
							break;
							
						case GETRTC:
  							GETRTC_Time( );
							break;
						case 'B':
							TestTemp=0;
							break;							
						default:
							break;
					}
				}	
				break;			
			default:
				break;
		}
	}
}

//===================================================================
void check_rtc_devicefile( void )
{    
	if( access( "/dev/rtc" , F_OK ) == 0 ) return;    
	mknod( "/dev/rtc" , S_IRWXU|S_IRWXG|S_IFCHR,(10<<8|(135))); 
}

int GETRTC_Time( void )
{    
	int rtc;    

	struct rtc_time rtc_time_data;     
	struct tm tm_src;    
	struct timeval tv = { 0, 0 };    
	check_rtc_devicefile();    

	rtc = open ( "/dev/rtc", O_RDONLY );    
	if ( rtc < 0 ) {perror( "/dev/rtc open error" ); return 0;}    
	if ( ioctl ( rtc , RTC_RD_TIME, &rtc_time_data ) < 0 ) {
		perror( "/dev/rtc rtc read time error" ); 
		close( rtc );
        	return 0;
    	}  
	printf( "RTC TIME %04d-%02d-%02d %02d:%02d:%02d\n", rtc_time_data.tm_year +	1900, rtc_time_data.tm_mon +1,  \
			rtc_time_data.tm_mday, rtc_time_data.tm_hour, rtc_time_data.tm_min,	rtc_time_data.tm_sec );    

	tm_src.tm_year = rtc_time_data.tm_year;    
	tm_src.tm_mon = rtc_time_data.tm_mon;     
	tm_src.tm_mday = rtc_time_data.tm_mday;    
	tm_src.tm_hour = rtc_time_data.tm_hour;    
	tm_src.tm_min  = rtc_time_data.tm_min;    
	tm_src.tm_sec  = rtc_time_data.tm_sec;    
	tv.tv_sec = mktime( &tm_src );    
	settimeofday ( &tv, NULL );     

	close( rtc );
    	return 0;
}

int SETRTC_Time( int year, int mon, int day, int hour, int min, int sec )
{    
	int rtc;    

	struct rtc_time  rtc_time_data;     
	struct tm tm_src;    
	struct timeval tv = { 0, 0 };    

	tm_src.tm_year = rtc_time_data.tm_year = year - 1900;    
	tm_src.tm_mon  = rtc_time_data.tm_mon  = mon  - 1;     
	tm_src.tm_mday = rtc_time_data.tm_mday = day;    
	tm_src.tm_hour = rtc_time_data.tm_hour = hour;    
	tm_src.tm_min  = rtc_time_data.tm_min = min;    
	tm_src.tm_sec  = rtc_time_data.tm_sec  = sec;    
	tv.tv_sec = mktime( &tm_src );    
	settimeofday ( &tv, NULL );        

	//check_rtc_devicefile();    

	rtc = open ( "/dev/rtc", O_WRONLY );    
	if( rtc < 0 ) {perror( "/dev/rtc open error" ); return -1;}    
	if ( ioctl ( rtc , RTC_SET_TIME, &rtc_time_data ) < 0 ) {        
		perror( "/dev/rtc rtc write time error" );        
		close( rtc );        
		return -1;    
	}    

	close( rtc );   
	return 0;
}

//===================================================================
int _menuTest(void)
{
        int c;        
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [1]. [Key]  -MATRIX / Scroll Wheel / HOLD Key \n");
        printf(" [2]. [ADC]  -BATTERY Check\n");
        printf(" [3]. [PWM]  -LCD Backlight \n");
        printf(" [4]. [GPIO]  -Key backlight / Headphone Detection / Charger Detection\n");
        printf("             -LCD / DAC / AMP / WLEN Power /Mute TR / Motion SensorI\n");
        printf(" [None]. [DAC]  - PCM1772\n");
        printf(" [6]. [TFT LCD]  -16bpp\n");
        printf(" [7]. [IR]  -Tx / Rx\n");
        printf(" [8]. [POWER]  -Power mode Management\n");
        printf(" [Testing]. [RTC]  -S3C2412 Management  \n");
        printf(" [None]. [Block Device] Control -MTD Management / SD,MMC Management\n");
        printf(" [None]. [Network Device] Control-CS8900 Management / WLAN Management\n");  
        printf("--------------------------------------------------------------------------\n");
        printf(" => User Test Item Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}

//===================================================================

int _KEY_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [1]. [KEY] Read Test\n");
	 printf("    1. Read Matrix key\n");	
	 printf("    2. Read Scroll key\n");	
	 printf("    3. Read Hold key\n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}

int _KEY_Matrix_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [1]. [KEY]  Test  1. Matrix key\n");
	printf("    Read = 0x00 : no key\n");
	printf("               0x11 ~ 0x44\n");
	printf("    	1. Read\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			

int _KEY_Scroll_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [1]. [KEY]  Test  2. Scroll \n");
	printf("    	1. Read\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			

int _KEY_Hold_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [1]. [KEY]  Test  3. Hold key\n");
	printf("    Read = LOW(Command=0x00/Nothing) /  HIGH(Command=0x01/Check)\n");
	printf("    	1. Read\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			
//===================================================================

int _ADC_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [2]. [ADC]  Test\n");
	 printf("    1. Battery AD read\n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}

int _ADC_Battery_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [2]. [ADC]  Test  1. Battery AD read\n");
	printf("    Read = 0x00 ~ 0xFF\n");
	printf("    	1. Read\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			

//===================================================================
int _PWM_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [3]. [PWM]  Test\n");
	 printf("    1. LCD backlight control\n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}

int _PWM_LCDB_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [3]. [PWM]  Test  1. LCD backlight control\n");
	printf("    Read = 0x00 ~ 0xFF\n");
	printf("    	1. Increase\n");
	printf("    	2. Decrease\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			
//===================================================================
int _GPIO_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [4]. [GPIO]  Test\n");
	 printf("    1. KeyBacklight Control\n");	
	 printf("    2. Headphone Detection\n");
        printf("    3. LCD Power Control\n");
        printf("    4. DAC Power Control\n");
        printf("    5. AMP Power Control\n");
        printf("    6. Mute TR Control\n");
        printf("    7. Wlan Power Control\n");		
        printf("    8. Motion Sensor\n");
        printf(" [B]. Back\n");
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}



int _GPIO_Led_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [4]. [GPIO]  Test  1. KeyBacklight Control\n");
	printf("    KeyBacklight Control Power On = LOW(Command=0x00)  / Power Off = HIGH(Command=0x01)\n");
	printf("    	1. KeyBacklight Control On\n");
	printf("    	2. KeyBacklight Control Off\n");
	printf(" [B]. Back\n");
	printf(" => [1 ~ 4] User Test Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			


int _GPIO_HeadphoneDetection_Str_menu(void)
{
        int c; 
	printf("\n");
       printf("--------------------------------------------------------------------------\n");
       printf(" [4]. [GPIO]  Test  2. Headphone Detection\n");
	printf("    Read = LOW(Command=0x00/Nothing) /  HIGH(Command=0x01/Check)\n");
	printf("    	1. Read\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}			


int _GPIO_LcdPower_Str_menu(void)
{
        int c; 
	printf("\n");
	printf("--------------------------------------------------------------------------\n");
	printf(" [4]. [GPIO]  Test, 3. LCD Power Control\n");
	printf("    LCD Power Off = LOW(Command=0x00)  / Power On = HIGH(Command=0x01)\n");
	printf("    	1. LCD Power Off\n");
	printf("    	2. LCD Power On\n");
	printf(" [B]. Back\n");
	printf(" => [1] or [2] User Test Selection number ? ");
	c= getchar();
	getchar();
	return c;
}			



int _GPIO_DACPower_Str_menu(void)
{
        int c; 
	printf("\n");
	printf("--------------------------------------------------------------------------\n");
	printf(" [4]. [GPIO]  Test  4. DAC Power Control\n");
	printf("    DAC Power Off = LOW(Command=0x00) / Power On = HIGH(Command=0x01)\n");
	printf("    	1. DAC Power Off\n");
	printf("    	2. DAC Powerr On\n");
	printf(" [B]. Back\n");
	printf(" => [1] or [2] User Test Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}	

int _GPIO_AMPPower_Str_menu(void)
{
        int c; 
	printf("\n");
	printf("--------------------------------------------------------------------------\n");
	printf(" [4]. [GPIO]  Test  5. AMP Power Control\n");
	printf("    AMP Power Off = LOW(Command=0x00) / Power On = HIGH(Command=0x01)\n");
	printf("    	1. AMPPower Off\n");
	printf("    	2. AMPPower On\n");
	printf(" [B]. Back\n");
	printf(" => [1] or [2] User Test Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}	



int _GPIO_MuteTR_Str_menu(void)
{
        int c; 
	printf("\n");
	printf("--------------------------------------------------------------------------\n");
	printf(" [4]. [GPIO]  Test  6. Mute TR Control\n");
	printf("    MUTE TR Control LOW(Command=0x00) / HIGH(Command=0x01)\n");
	printf("    	1. MUTE TR Off [not Mute] \n");
	printf("    	2. MUTE TR On [Mute] \n");
	printf(" [B]. Back\n");
	printf(" => [1] or [2] User Test Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}	

int _GPIO_WlanPower_Str_menu(void)
{
        int c; 
	printf("\n");
	printf("--------------------------------------------------------------------------\n");
	printf(" [4]. [GPIO]  Test  7. Wlan Power Control\n");
	printf("   Wlan Power Control LOW(Command=0x00) / HIGH(Command=0x01)\n");
	printf("    	1. Wlan Power Control On \n");
	printf("    	2. Wlan Power Control Off \n");
	printf(" [B]. Back\n");
	printf(" => [1] or [2] User Test Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}	



int _GPIO_MotionSensor_Str_menu(void)
{
        int c; 
	printf("\n");
	printf("--------------------------------------------------------------------------\n");
	printf(" [4]. [GPIO]  Test 8. Motion Sensor\n");
	printf("    Read = LOW(Command=0x00/Nothing) /  HIGH(Command=0x01/Check)\n");
	printf("    	1. Read\n");
	printf(" [B]. Back\n");
	printf(" => [1] Selection number ? ");
	 c= getchar();
	getchar();
	return c;
}	
		
//===================================================================

int _LCD_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [6]. [TFT LCD]  Test\n");
	 printf("    1. Randomize Rectangler\n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}

//===================================================================

int _IR_TRxTest_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [7]. [IR TRx Test]  \n");
	 printf("    1. IR Tx Test [SAMSUNG TV] Power Code 0x070702FD [66Bit] \n");	
	 printf("    2. IR Tx Test [SAMSUNG TV] ChUp Code 0x070710EF  [66Bit] \n");	
	 printf("    3. IR Tx Test [SAMSUNG TV] ChDn Code 0x07071AE5 [66Bit] \n");	
	 printf("    4. IR Tx Test [SAMSUNG TV] VolUp Code 0x07070BF4 [66Bit] \n");	
	 printf("    5. IR Tx Test [SAMSUNG TV] VolDn Code 0x07070FF0 [66Bit] \n");	
        printf("--------------------------------------------------------------------------\n");
	 printf("    6. IR Rx Test [Nothing] \n");	
	 printf("    7. IR Rx Test [Nothing] \n");	
	 printf("    8. IR Rx Test [Nothing] \n");	
	 printf("    9. IR Rx Test [Nothing] \n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}

//===================================================================
int _Power_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [8]. [Power Mode] Test\n");
	 printf("    1. All power On \n");
	 printf("    (Lcd pwr/backlit, DAC pwr, AMP pwr, KEY backlit, Wlan pwr, Mute tr)\n");	
	 printf("    2. LCD Off \n");
	 printf("	 (Lcd pwr/backlit, KEY backlit)\n");	
	 printf("    3. All power Off \n");	
	 printf("    (Lcd pwr/backlit, DAC pwr, AMP pwr, KEY backlit, Wlan pwr, Mute tr)\n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}


//===================================================================

int _RTC_Test_Str_menu(void)
{
        int c; 
        printf("\n");
        printf("--------------------------------------------------------------------------\n");
        printf(" [A]. [RTC LCD]  Test\n");
	 printf("    1. set RTC time\n");	
 	 printf("    2. get RTC time\n");	
	 printf(" [B]. Back\n");	 
        printf(" => User Test Item Selection number ? ");	 
	 c= getchar();
	getchar();
	return c;
}









