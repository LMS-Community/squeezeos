
/**************************************************
 Code to initialize the LCD for 2413 SMDK Eval board
 through SPI controlled by GPIO's.
*************************************************/

#define LCD_DEN		(1<<2)
#define LCD_DSERI	(1<<12)
#define LCD_DCLK	(1<<13)
#define LCD_RESET     (0)


#define LCD_DEN_Lo	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					   gpdat &= ~LCD_DEN; \
					   __raw_writel(gpdat, S3C2413_GPGDAT); \
					 }

#define LCD_DEN_Hi	{ gpdat = __raw_readl(S3C2413_GPGDAT); \
					   gpdat |= LCD_DEN; \
					   __raw_writel(gpdat, S3C2413_GPGDAT); \
					 }

#define LCD_DCLK_Lo	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat &= ~LCD_DCLK; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					 }

#define LCD_DCLK_Hi	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat |= LCD_DCLK; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					 }

#define LCD_DSERI_Lo  { gpdat = __raw_readl(S3C2413_GPEDAT); \
					   gpdat &= ~LCD_DSERI; \
					   __raw_writel(gpdat, S3C2413_GPEDAT); \
					  }

#define LCD_DSERI_Hi	{ gpdat = __raw_readl(S3C2413_GPEDAT); \
							gpdat |= LCD_DSERI; \
							__raw_writel(gpdat, S3C2413_GPEDAT); \
						}

#define LCD_RESET_Lo	(0)
#define LCD_RESET_Hi	(1)

void Write_LDI(unsigned short address, unsigned short data)
{
	int j;
	unsigned long DELAY = 1;
	unsigned long gpdat;

	LCD_DEN_Hi;		//      EN = High                                       CS high
//      LCD_DCLK_Lo;                                    
	LCD_DCLK_Hi;		//      SCL High
	LCD_DSERI_Lo;		//      Data Low

	udelay(DELAY);

	LCD_DEN_Lo;		//      EN = Low                                CS Low
	udelay(DELAY);

	LCD_DCLK_Lo;		//      SCL Low

	udelay(DELAY);

	for (j = 7; j >= 0; j--) {
		if ((address >> j) & 0x0001) {	// DATA HIGH or LOW
			LCD_DSERI_Hi;
		}
		else {
			LCD_DSERI_Lo;
		}

		udelay(DELAY);

		LCD_DCLK_Hi;	// CLOCK = High
		udelay(DELAY);

		if (j != 0) {
			LCD_DCLK_Lo;
			udelay(DELAY);	// CLOCK = Low
		}
	}

	LCD_DSERI_Lo;		// Data Low
	udelay(DELAY);

	LCD_DEN_Hi;		//      EN = High
	udelay(DELAY);

	LCD_DEN_Lo;		//      EN = Low
	udelay(DELAY);

	LCD_DCLK_Lo;
	udelay(DELAY);		// CLOCK = Low

	for (j = 7; j >= 0; j--) {
		if ((data >> j) & 0x0001){	// DATA HIGH or LOW
			LCD_DSERI_Hi;
		}
		else {
			LCD_DSERI_Lo;
		}

		udelay(DELAY);

		LCD_DCLK_Hi;	// CLOCK = High
		udelay(DELAY);

		if (j != 0) {
			LCD_DCLK_Lo;
			udelay(DELAY);	// CLOCK = Low
		}
	}

	LCD_DEN_Hi;		// EN = High
	udelay(DELAY);
}

void SetGPIOforLDI(void)
{
	unsigned long gpdn;
	unsigned long gpdat;
	unsigned long gpcon;

	gpdn = __raw_readl(S3C2413_GPEDN);
	gpdn  |= (0x3<<12);
	__raw_writel(gpdn, S3C2413_GPEDN);

	gpdn = __raw_readl(S3C2413_GPGDN);
	gpdn  |= (0x1<<2);
	__raw_writel(gpdn, S3C2413_GPGDN);

	gpdat = __raw_readl(S3C2413_GPEDAT);
	gpdat  |= (0x3<<12);
	__raw_writel(gpdat, S3C2413_GPEDAT);

	gpdat = __raw_readl(S3C2413_GPGDAT);
	gpdat  |= (0x1<<2);
	__raw_writel(gpdat, S3C2413_GPGDAT);

	gpcon = __raw_readl(S3C2413_GPECON);
	gpcon  &= ~(0xf<<24);
	gpcon |= (0x5<<24);
	__raw_writel(gpcon, S3C2413_GPECON);

	gpcon = __raw_readl(S3C2413_GPGCON);
	gpcon  &= ~(0x3<<4);
	gpcon |= (0x1<<4);
	__raw_writel(gpcon, S3C2413_GPGCON);
}

static void InitLDI(void)
{
	unsigned long gpdat;
	
	SetGPIOforLDI();
	
	LCD_DEN_Hi;
	LCD_DCLK_Hi;
	LCD_DSERI_Hi;
    LCD_RESET_Hi;

	// LCD Reset high
      	LCD_RESET_Hi;     

	///////////////////////////////////////////////////////////////////
	// Power Setting Function 1
	//////////////////////////////////////////////////////////////////
	Write_LDI(0x22, 0x01);	// PARTIAL 2 DISPLAY AREA RASTER-ROW NUMBER REGISTER 1
	Write_LDI(0x03, 0x01);	// RESET REGISTER

	///////////////////////////////////////////////////////////////////
	// Initializing Function 1
	///////////////////////////////////////////////////////////////////
	Write_LDI(0x00, 0x0a);	// CONTROL REGISTER 1
	udelay(1);		// delay about 300ns
	Write_LDI(0x01, 0x10);	// CONTROL REGISTER 2
	udelay(1);		// delay about 300ns
	Write_LDI(0x02, 0x06);	// RGB INTERFACE REGISTER
	udelay(1);		// delay about 300ns
	Write_LDI(0x05, 0x00);	// DATA ACCESS CONTROL REGISTER
	udelay(1);		// delay about 300ns
	Write_LDI(0x0D, 0x00);	// 

	// delay about 40ms
	mdelay(40);
	///////////////////////////////////////////////////////////////////
	// Initializing Function 2
	///////////////////////////////////////////////////////////////////
	Write_LDI(0x0E, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x0F, 0x00);	// 
	udelay(1);		// delay about 300ns
	Write_LDI(0x10, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x11, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x12, 0x00);	//
	udelay(1);		// delay about 300ns 
	Write_LDI(0x13, 0x00);	// DISPLAY SIZE CONTROL REGISTER
	udelay(1);		// delay about 300ns
	Write_LDI(0x14, 0x00);	// PARTIAL-OFF AREA COLOR REGISTER 1
	udelay(1);		// delay about 300ns
	Write_LDI(0x15, 0x00);	// PARTIAL-OFF AREA COLOR REGISTER 2
	udelay(1);		// delay about 300ns
	Write_LDI(0x16, 0x00);	// PARTIAL 1 DISPLAY AREA STARTING REGISTER 1
	udelay(1);		// delay about 300ns
	Write_LDI(0x17, 0x00);	// PARTIAL 1 DISPLAY AREA STARTING REGISTER 2
	udelay(1);		// delay about 300ns
	Write_LDI(0x34, 0x01);	// POWER SUPPLY SYSTEM CONTROL REGISTER 14
	udelay(1);		// delay about 300ns
	Write_LDI(0x35, 0x00);	// POWER SUPPLY SYSTEM CONTROL REGISTER 7

	// delay about 30ms
	mdelay(30);

	////////////////////////////////////////////////////////////////////
	// Initializing Function 3
	////////////////////////////////////////////////////////////////////
	Write_LDI(0x8D, 0x01);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x8B, 0x28);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x4B, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x4C, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x4D, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x4E, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x4F, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x50, 0x00);	//  ID CODE REGISTER 2        
	// delay about 50 us
	udelay(50);

	Write_LDI(0x86, 0x00);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x87, 0x26);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x88, 0x02);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x89, 0x05);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x33, 0x01);	//  POWER SUPPLY SYSTEM CONTROL REGISTER 13
	udelay(1);		// delay about 300ns
	Write_LDI(0x37, 0x06);	//  POWER SUPPLY SYSTEM CONTROL REGISTER 12   

	// delay about 50 us
	udelay(50);

	Write_LDI(0x76, 0x00);	//  SCROLL AREA START REGISTER 2

	// delay about 30ms
	mdelay(30);
	/////////////////////////////////////////////////////////////////////
	// Initializing Function 4
	/////////////////////////////////////////////////////////////////////
	Write_LDI(0x42, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x43, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x44, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x45, 0x00);	//  CALIBRATION REGISTER
	udelay(1);		// delay about 300ns
	Write_LDI(0x46, 0xef);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x47, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x48, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x49, 0x01);	//  ID CODE REGISTER 1               check it out

	// delay about 50 us
	udelay(50);

	Write_LDI(0x4A, 0x3f);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x3C, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x3D, 0x00);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x3E, 0x01);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x3F, 0x3f);	//
	udelay(1);		// delay about 300ns
//      Write_LDI(0x40,0x03);  //       horizontal back porch   
	Write_LDI(0x40, 0x01);	//       horizontal back porch    //050105 Boaz.Kim
	udelay(1);		// delay about 300ns
//      Write_LDI(0x41,0x04);  //       vertical back porch
	Write_LDI(0x41, 0x0a);	//       horizontal back porch   //050105 Boaz.Kim
	udelay(1);		// delay about 300ns
	Write_LDI(0x8F, 0x05);	//

	// delay about 30ms
	mdelay(30);

	/////////////////////////////////////////////////////////////////////
	// Initializing Function 5
	/////////////////////////////////////////////////////////////////////
	Write_LDI(0x90, 0x05);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x91, 0x44);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x92, 0x44);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x93, 0x44);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x94, 0x33);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x95, 0x05);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x96, 0x05);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x97, 0x44);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x98, 0x44);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x99, 0x44);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x9A, 0x33);	//  
	udelay(1);		// delay about 300ns
	Write_LDI(0x9B, 0x33);	//
	udelay(1);		// delay about 300ns
	Write_LDI(0x9C, 0x33);	//
	udelay(1);		// delay about 300ns

#if 1
	Write_LDI(0x9D, 0x80);	//       16 or 18bit RGB
#else
	Write_LDI(0x9D, 0x81);	//       6bit RGB
#endif

	// delay about 30ms
	mdelay(30);
	/////////////////////////////////////////////////////////////////////
	// Power Setting 2
	/////////////////////////////////////////////////////////////////////
	Write_LDI(0x1D, 0x08);	//

	// delay about 30ms
	mdelay(30);

	Write_LDI(0x23, 0x00);	//  PARTIAL 2 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
	// delay about 50 us
	udelay(50);
	Write_LDI(0x24, 0x94);	//  POWER SUPPLY SYSTEM CONTROL REGISTER 1
	// delay about 50 us
	udelay(50);
	Write_LDI(0x25, 0x6f);	//  POWER SUPPLY SYSTEM CONTROL REGISTER 2

	// delay about 40ms
	mdelay(40);


	/////////////////////////////////////////////////////////////////////
	// Power Setting 3
	/////////////////////////////////////////////////////////////////////
	Write_LDI(0x28, 0x1e);	// 
	Write_LDI(0x1A, 0x00);	// 
	Write_LDI(0x21, 0x10);	//  PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2 
	Write_LDI(0x18, 0x25);	//  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

	// delay about 40ms
	mdelay(40);

	Write_LDI(0x19, 0x48);	//  PARTIAL 2 DISPLAY AREA STARTING REGISTER 2
	Write_LDI(0x18, 0xe5);	//  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

	// delay about 10ms
	mdelay(10);

	Write_LDI(0x18, 0xF7);	//  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1 

	// delay about 40ms
	mdelay(40);

	Write_LDI(0x1B, 0x07);	// org
//      Write_LDI(0x1B,0x01);  // 90 rotate
//      Write_LDI(0x1B,0x02);  // 90 rotate
//      Write_LDI(0x1B,0x03);  // 90 rotate


	// delay about 80ms
	mdelay(80);

	Write_LDI(0x1F, 0x6b);	// org
//      Write_LDI(0x1F,0x5E);  // 90 rotate

	Write_LDI(0x20, 0x51);	//  org, PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 1
//      Write_LDI(0x20,0x5F);  //  90 rotate, PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 1

	Write_LDI(0x1E, 0xc1);	// 

	// delay about 10ms
	mdelay(10);

	Write_LDI(0x21, 0x00);	//  PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2 
	Write_LDI(0x3B, 0x01);	// 

	// delay about 20ms
	mdelay(20);

	Write_LDI(0x00, 0x20);	//  CONTROL REGISTER 1
	Write_LDI(0x02, 0x01);	//  RGB INTERFACE REGISTER

	// delay about 10ms
	mdelay(10);
//      Reg16_OPCLK_DIV = 0x0201;                               // 6.4

}
