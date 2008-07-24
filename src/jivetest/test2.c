/*------------------------------------------
 @  
 -------------------------------------------
 @ brief	: Test program for s3c2412 driver
 @ author	: works

 @ history
  060907 first make by works
------------------------------------------*/

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <linux/fb.h> 

#include "test.h"

int main(void)
{
	int fd_matrix, fd_led, fd_fb, fd_hold, fd_bat, fd_lcdPwr;
	int fd_hdet,fd_dacp, fd_ampp, fd_mute, fd_wlanp, fd_motion, fd_lcdb;
	int t, tt, offset, ret, temp;
	int dimming = 0xff;
	
	unsigned char key, temp_key, key_flag = 0;
	unsigned char buffer[1], bat_ad[1];
	unsigned char hold_buffer[1];

	struct fb_var_screeninfo fbvar;
	unsigned short *pfbdata;
	unsigned short pixeldat;
	
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
	fd_wlanp = open("/dev/WLANP", O_RDWR );
	fd_motion = open("/dev/MOTION", O_RDWR );
	fd_lcdb = open("/dev/LCDB", O_RDWR );	
	
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
 	if (fd_wlanp <0) {perror("/dev/WLANP"); exit(-1); }
 	if (fd_motion <0) {perror("/dev/MOTION"); exit(-1); }
 	if (fd_lcdb <0) {perror("/dev/LCDB"); exit(-1); }

	ret = ioctl(fd_fb, FBIOGET_VSCREENINFO, &fbvar);
	if(ret < 0) {perror("fbdev ioctl");exit(1);}
	
	if(fbvar.bits_per_pixel != 16) {fprintf(stderr, "bpp is not 16\n");exit(1);}

	pfbdata = (unsigned short *)mmap(0, fbvar.xres*fbvar.yres*16/8,PROT_READ|PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if((unsigned)pfbdata == (unsigned)-1) {perror("fbdev mmap");exit(1);}

	buffer[0] = 0x01;
	write(fd_lcdPwr, buffer, 1);

	buffer[0] = 0x01;
	write(fd_dacp, buffer, 1);
	
  	while(1) {
 		read(fd_matrix, buffer, 1);
		read(fd_hold, hold_buffer, 1);
		temp_key = buffer[0];

		if (key == temp_key) key_flag = 0;
		else {
			key = temp_key;
			key_flag = 1;
		}
		
		if (key_flag != 0 && hold_buffer[0] != 0) {
			switch(key){
				case 0x11:
					if (dimming < 0xef) dimming += 0x10;
					else dimming = 0xff;
					buffer[0] = dimming;
					printf("LCDB_dimming value is 0x%x\n", buffer[0]);
					write(fd_lcdb, buffer, 1);
	       			break;
				case 0x12:
					if (dimming > 0x10) dimming -= 0x10;
					else dimming = 0x00;
					buffer[0] = dimming;
					printf("LCDB_dimming value is 0x%x\n", buffer[0]);
					write(fd_lcdb, buffer, 1);
					break;
				case 0x13:
	       			break;
				case 0x14:
					break;
				case 0x21:
					printf("BATTERY check\n");
					read(fd_bat, bat_ad, 1);
					printf("Remain battery is %x\n", bat_ad[0]);
	       			break;
				case 0x22:
					printf("RGB test(565)\n");
		
	       			break;				
				case 0x31:
					printf("Box test\n");
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
				case 0x41:
					break;
				case 0x42:
					break;
				case 0x24:
					printf("RED test(565)\n");
					temp = 0x0000;
					
					for (t=0; t<256; t++) {
						offset = t*fbvar.xres;
						temp++;
								
						for (tt=0; tt<fbvar.xres; tt++) {
							pixeldat = 0x0000;
							pixeldat |= ((temp >>3)<<11);
							*(pfbdata+offset+tt) = pixeldat;
						}
					}
					break;				
				case 0x34:
					printf("GREEN test(565)\n");
					temp = 0x0000;
					
					for (t=0; t<256; t++) {
						offset = t*fbvar.xres;
						temp++;
						
						for (tt=0; tt<fbvar.xres; tt++) {
							pixeldat = 0x0000;
							pixeldat |= ((temp >>2)<<5); 
							*(pfbdata+offset+tt) = pixeldat;
						}
					}
					break;				
				case 0x44:
					printf("BLUE test(565)\n");
					temp = 0x0000;
					
					for (t=0; t<256; t++) {
						offset = t*fbvar.xres;
						temp++;
						
						for (tt=0; tt<fbvar.xres; tt++) {
							pixeldat = 0x0000;
							pixeldat |= ((temp >>3));
							*(pfbdata+offset+tt) = pixeldat;
						}
					}
					break;
			}
  		}
	}

    	return key;
}



