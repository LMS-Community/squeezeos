
#define rGPBCON   S3C2413_GPBCON    	//Configure the pins of port B
#define rGPBDAT    S3C2413_GPBDAT		//The data for port B



#define L3C (1<<4)	//GPB4 = L3CLOCK
#define L3D (1<<3)	//GPB3 = L3DATA 
#define L3M (1<<2)	//GPB2 = L3MODE


void _WrL3Addr(unsigned char data)
{	 
 	int i,j;

    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | L3C) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H

    	for(j=0;j<4;j++);	 //tsu(L3) > 190ns

	//GPB[4:2]=L3C:L3D:L3M
    	for(i=0;i<8;i++)	//LSB first
    	{
	  	if(data & 0x1)	//If data's LSB is 'H'
	  	{
			writel(readl(rGPBDAT)&~L3C,rGPBDAT);
			writel(readl(rGPBDAT)|L3D,rGPBDAT);
			for(j=0;j<4;j++);	        //tcy(L3) > 500ns
			writel(readl(rGPBDAT)|L3C,rGPBDAT);
			writel(readl(rGPBDAT)|L3D,rGPBDAT);
			for(j=0;j<4;j++);	        //tcy(L3) > 500ns
	  	}
	  	else		//If data's LSB is 'L'
	  	{
			writel(readl(rGPBDAT)&~L3C,rGPBDAT);
			writel(readl(rGPBDAT)&~L3D,rGPBDAT);
			for(j=0;j<4;j++);	       //tcy(L3) > 500ns
			writel(readl(rGPBDAT)|L3C,rGPBDAT);
			writel(readl(rGPBDAT)&~L3D,rGPBDAT);
			for(j=0;j<4;j++);	       //tcy(L3) > 500ns		
	  	}
	  	data >>= 1;
    	}

    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
}


void _WrL3Data(unsigned char data,int halt)
{
 	int i,j;

    	if(halt)
    	{
    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | L3C ) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
	  	for(j=0;j<4;j++);		//tstp(L3) > 190ns
    	}

    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
    	for(j=0;j<4;j++);		//tsu(L3)D > 190ns

	//GPB[4:2]=L3C:L3D:L3M
    	for(i=0;i<8;i++)
    	{
	  	if(data & 0x1)	//if data's LSB is 'H'
	  	{
			writel(readl(rGPBDAT)&~L3C,rGPBDAT);
			writel(readl(rGPBDAT)|L3D,rGPBDAT);

	     		for(j=0;j<4;j++);			//tcy(L3) > 500ns
			writel(readl(rGPBDAT)|(L3D|L3C),rGPBDAT);
	     		for(j=0;j<4;j++);		 	//tcy(L3) > 500ns
	  	}
	  	else		//If data's LSB is 'L'
	  	{
			writel(readl(rGPBDAT)&~L3C,rGPBDAT);
			writel(readl(rGPBDAT)&~L3D,rGPBDAT);
	     		for(j=0;j<4;j++);		//tcy(L3) > 500ns
			writel(readl(rGPBDAT)|L3C,rGPBDAT);
			writel(readl(rGPBDAT)&~L3D,rGPBDAT);
	     		for(j=0;j<4;j++);		//tcy(L3) > 500ns
	  	}
		data >>= 1;		//For check next bit
    	}

    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
}














void Init1341(int mode)
{
 	
	//----------------------------------------------------------
	//PORT B GROUP
	//Ports  :   GPB3  		GPB4  		GPB2
	//Signal :   L3DATA      L3CLOCK 	L3MODE 
	//Setting:  OUTPUT      OUTPUT 		OUTPUT
	//	          [7:6]		[9:8] 		[5:4] 
	//Binary :     01 	      01	      01
	//---------------------------------------------------------- 
    	writel((readl( rGPBDAT) & ~(L3D | L3M | L3C) | (L3C | L3M)) ,rGPBDAT);	//L3D=L, L3M=L(in address mode), L3C=H
		
		writel((readl(rGPBCON)& ~(0x3<<4) & ~(0x3<<6) & ~(0x3<<8)),rGPBCON);
		writel((readl(rGPBCON)| (0x1<<4) |(0x1<<6) |(0x1<<8) ),rGPBCON);
 
	//L3 Interface
    _WrL3Addr(0x14 + 2);     //STATUS (000101xx+10)
 	_WrL3Data(0x50,0);	 //0,1,01, 000,0 : Status 0,Reset, 384fs,IIS-bus,no DC-filtering
	
	_WrL3Addr(0x14 + 2);     //STATUS (000101xx+10)
    _WrL3Data(0x81,0);	 //bit[7:0] => 1,0,0,0, 0,0,01 

	//Record Sound via MIC-In
    if(mode == 1)
    {
	  	_WrL3Addr(0x14 + 2);    //STATUS (000101xx+10)
		_WrL3Data(0xa2,0);	//bit[7:0] => 1,0,1,0,0,0,10
		
		_WrL3Addr(0x14 + 0);    //DATA0 (000101xx+00)
       	_WrL3Data(0xc2, 0);	//1100 0,010  : Extended addr(3bits), 010 
        _WrL3Data(0xf2, 0);	//111,100,10 : DATA0, MIC Amplifier Gain 21dB, input channel 2 select (input channel 1 off)              
    }
}
