/*
 *  linux/include/linux/l3/WM8750.h
 *
 * wolfson WM8750 mixer device driver
 *
 * Copyright (c) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

#define WM8750_NAME "wm8750"

struct wm8750_cfg {
	unsigned int fs:16;
	unsigned int format:3;
};

#define FMT_I2S		0
#define FMT_LSB16	1
#define FMT_LSB18	2
#define FMT_LSB20	3
#define FMT_MSB		4
#define FMT_LSB16MSB	5
#define FMT_LSB18MSB	6
#define FMT_LSB20MSB	7

#define L3_WM8750_CONFIGURE	0x13410001

struct l3_gain {
	unsigned int	left:8;
	unsigned int	right:8;
	unsigned int	unused:8;
	unsigned int	channel:8;
};

#define L3_SET_VOLUME		0x13410002
#define L3_SET_TREBLE		0x13410003
#define L3_SET_BASS		0x13410004
#define L3_SET_GAIN		0x13410005

struct l3_agc {
	unsigned int	level:8;
	unsigned int	enable:1;
	unsigned int	attack:7;
	unsigned int	decay:8;
	unsigned int	channel:8;
};

#define L3_INPUT_AGC		0x13410006

#define ALL_ON				0			// DAC/ADC all on
#define DAC_ON				1			// DAC on,  ADC off
#define ADC_ON				2			// DAC off, ADC on
#define ALL_OFF				3			// DAC/ADC all off
#define LOUT_ON				4

//Regiser Define

#define 		WM8750_R0_LeftInVol_ADDR			0x00
	#define		WM8750_R0_LIVU_BIT				0x01
	#define		WM8750_R0_LINMUTE_BIT				0x80				
	#define		WM8750_R0_LIZC_BIT				0x40
	#define		WM8750_R0_LINVOL_MASK				0x3f		//max input 
	
#define 		WM8750_R1_RightInVol_ADDR			0x02
	#define		WM8750_R1_RIVU_BIT				0x01
	#define		WM8750_R1_RINMUTE_BIT				0x80				
	#define		WM8750_R1_RIZC_BIT				0x40
	#define		WM8750_R1_RINVOL_MASK				0x3f

#define		WM8750_R2_LOUT1Vol_ADDR					0x04
	#define		WM8750_R2_LO1VU_BIT				0x01
	#define		WM8750_R2_LO1ZC_BIT				0x80
	#define		WM8750_R2_LOUT1VOL_MASK				0x7f		//max output 6db

#define		WM8750_R3_ROUT1Vol_ADDR					0x06
	#define		WM8750_R3_RO1VU_BIT				0x01
	#define		WM8750_R3_RO1ZC_BIT				0x80
	#define		WM8750_R3_ROUT1VOL_MASK				0x7f

#define		WM8750_R4_Reserved_ADDR					0x08

#define		WM8750_R5_ADCDACCON_ADDR				0x0a
	#define		WM8750_R5_ADCDIV2_BIT				0x01
	#define		WM8750_R5_DACDIV2_BIT				0x80
	#define		WM8750_R5_ADCPOL_MASK				0x60
	#define		WM8750_R5_HPOR_BIT				0x10
	#define		WM8750_R5_DACMU_BIT				0x08
	#define		WM8750_R5_DEEMPH_MASK				0x06
	#define		WM8750_R5_ADCHPD_BIT				0x01

#define		WM8750_R6_Reserved_ADDR					0x0c

#define		WM8750_R7_AudioIF_ADDR					0x0e
	#define		WM8750_R7_BCLKINV_BIT				0x80
	#define		WM8750_R7_MS_BIT				0x40
	#define		WM8750_R7_LRSWAP_BIT				0x20
	#define		WM8750_R7_LRP_BIT				0x10
	#define		WM8750_R7_WL_MASK				0x00		//16bit
	#define		WM8750_R7_FORMAT_MASK				0x02		//i^s format

#define		WM8750_R8_SampleRate_ADDR				0x10
	#define		WM8750_R8_CLKDIV2_BIT				0x40
	#define		WM8750_R8_SR_MASK				0x20		//11.2896Mhz(MCLK),adc sample rate 44.1khz,dac sample rate 44.1khz
	#define		WM8750_R8_USB_BIT				0x01

#define		WM8750_R9_Reserved_ADDR					0x12

#define		WM8750_R10_LeftDACVol_ADDR				0x14
	#define		WM8750_R10_LDVU_BIT				0x01
	#define		WM8750_R10_LDACVOL_MASK				0xff

#define		WM8750_R11_RightDACVOl_ADDR				0x16
	#define		WM8750_R11_RDVU_BIT				0x01
	#define		WM8750_R11_RDACVOL_MASK				0xff	

#define		WM8750_R12_BASSCon_ADDR					0x18
	#define		WM8750_R12_BB_BIT				0x80
	#define		WM8750_R12_BC_BIT				0x40
	#define		WM8750_R12_BASS_MASK				0x0f

#define		WM8750_R13_TrebleCon_ADDR				0x1a
	#define		WM8750_R13_TC_BIT				0x40
	#define		WM8750_R13_TRBL_MASK				0x0f

#define		WM8750_R15_Reset_ADDR					0x1e

#define		WM8750_R16_3DCon_ADDR					0x20
	#define		WM8750_R16_MODE3D_BIT				0x80
	#define		WM8750_R16_3DUC_BIT				0x40
	#define		WM8750_R16_3DLC_BIT				0x20
	#define		WM8750_R16_3DDEPTH_MASK				0x1e
	#define		WM8750_R16_3DEN_BIT				0x01

#define		WM8750_R17_ALC1_ADDR					0x22
	#define		WM8750_R17_ALCSEL_MASK_1			0x01	//stereo //Note: LINVOL,RINVOL setting are the same Before entering this mode
	#define		WM8750_R17_ALCSEL_MASK_0			0x80
	#define		WM8750_R17_MAXGAIN_MASK				0x70	//max gain 30db 
	#define		WM8750_R17_ALCL_MASK				0x08    // -8.5db

#define		WM8750_R18_ALC2_ADDR					0x24
	#define		WM8750_R18_ALCZC_BIT				0x80
	#define		WM8750_R18_HLD_MASK				0x0f

#define		WM8750_R19_ALC3_ADDR			0x26
	#define		WM8750_R19_DCY_MASK				0xf0
	#define		WM8750_R19_ATK_MASK				0x0f

#define		WM8750_R20_NosieGate_ADDR		0x28
	#define		WM8750_R20_NGTH_MASK				0xf8
	#define		WM8750_R20_NGG_MASK				0x06
	#define		WM8750_R20_NGAT_BIT				0x01

#define		WM8750_R21_LeftADCVol_ADDR		0x2a
	#define		WM8750_R21_LAVU_BIT				0x01
#define		WM8750_R22_RightADCVol_ADDR		0x2c
	#define		WM8750_R21_RAVU_BIT				0x01

#define		WM8750_R23_AdditionalCont1_ADDR		0x2e
	#define		WM8750_R23_TSDEN_BIT				0x01
	#define		WM8750_R23_VSEL_MASK				0x00	//AVDD1.8V
	//#define	WM8750_R23_VSEL_MASK				0x60	//AVDD2.5V
	#define		WM8750_R23_DMONOMIX_MASK			0x00	//STEREO
	//#define		WM8750_R23_DATSEL_MASK				0x00	//left data=left ADC;right data=right ADC
	#define		WM8750_R23_DATSEL_MASK				0x0c	//left data=right ADC;right data=left ADC
	#define		WM8750_R23_DACINV_BIT				0x02
	#define		WM8750_R23_TOEN_BIT				0x01
#define		WM8750_R24_AdditionalCont2_ADDR		0x30
	#define		WM8750_R24_OUT3SW_MASK_1			0x01	//VREF
	#define		WM8750_R24_OUT3SW_MASK_0			0x80
	#define		WM8750_R24_HPSWEN_BIT				0x40
	#define		WM8750_R24_HPSWPOL_BIT				0x20	
	#define		WM8750_R24_ROUT2INV_BIT				0x10		
	#define		WM8750_R24_TRI_BIT				0x08		
	#define		WM8750_R24_LRCM_BIT				0x04
	#define		WM8750_R24_ADCOSR_BIT				0x02		
	#define		WM8750_R24_DACOSR_BIT				0x01	
#define		WM8750_R25_PwrMgmt1_ADDR		0x32
	#define		WM8750_R25_VMIDSEL_MASK_1			0x01	//
	#define		WM8750_R25_VMIDSEL_MASK_0			0x80
	#define		WM8750_R25_VREF_BIT				0x40
	#define		WM8750_R25_AINL_BIT				0x20
	#define		WM8750_R25_AINR_BIT				0x10
	#define		WM8750_R25_ADCL_BIT				0x08
	#define		WM8750_R25_ADCR_BIT				0x04
	#define		WM8750_R25_MICB_BIT				0x02	
	#define		WM8750_R25_DIGENB_BIT				0x01	//NOTE; 1MSEC		
#define		WM8750_R26_PwrMgmt2_ADDR		0x34
	#define		WM8750_R26_DACL_BIT				0x01	//
	#define		WM8750_R26_DACR_BIT				0x80
	#define		WM8750_R26_LOUT1_BIT				0x40
	#define		WM8750_R26_ROUT1_BIT				0x20
	#define		WM8750_R26_LOUT2_BIT				0x10
	#define		WM8750_R26_ROUT2_BIT				0x08
	#define		WM8750_R26_MONO_BIT				0x04
	#define		WM8750_R26_OUT3_BIT				0x02	
#define		WM8750_R27_AdditionalCont3_ADDR		0x36
	#define		WM8750_R27_ADCLRM_MASK_1			0x01	//
	#define		WM8750_R27_ADCLRM_MASK_0			0x80
	#define		WM8750_R27_VROL_BIT				0x40
#define		WM8750_R31_ADCInputMode_ADDR		0x3e
	#define		WM8750_R31_DS_BIT				0x01	//
	#define		WM8750_R31_MONOMIX_MASK				0xC0	//STEREO
	#define		WM8750_R31_RDCM_BIT				0x20
	#define		WM8750_R31_LDCM_BIT				0x10
#define		WM8750_R32_ADCL_ADDR			0x40
	#define		WM8750_R32_LINSEL_MASK_1			0x80	//
	#define		WM8750_R32_LINSEL_MASK_0			0x60
	#define		WM8750_R32_LMICBOOST_MASK			0x00	//BOOST OFF	
#define		WM8750_R33_ADCR_ADDR			0x42
	#define		WM8750_R33_RINSEL_MASK_1			0x80	//
	#define		WM8750_R33_RINSEL_MASK_0			0x60
	#define		WM8750_R33_RMICBOOST_MASK			0x00	//BOOST OFF

#define		WM8750_R34_LeftOutMix1_ADDR		0x44
	#define		WM8750_R34_LD2LO_BIT				0x01
	#define		WM8750_R34_LI2LO_BIT				0x80
	#define		WM8750_R34_LI2LOVOL_MASK			0x00	//MAX 6db 
	#define		WM8750_R34_LMIXSEL_MASK_2			0x04
	#define		WM8750_R34_LMIXSEL_MASK_1			0x02
	#define		WM8750_R34_LMIXSEL_MASK_0			0x01
#define		WM8750_R35_LeftOutMix2_ADDR		0x46
	#define		WM8750_R35_RD2LO_BIT				0x01
	#define		WM8750_R35_RI2LO_BIT				0x80
	#define		WM8750_R35_RI2LOVOL_MASK			0x00	//MAX 6db 

#define		WM8750_R36_RightOutMix1_ADDR		0x48
	#define		WM8750_R36_LD2RO_BIT				0x01
	#define		WM8750_R36_LI2RO_BIT				0x80
	#define		WM8750_R36_LI2ROVOL_MASK			0x00	//MAX 6db 
	#define		WM8750_R36_RMIXSEL_MASK_2			0x04
	#define		WM8750_R36_RMIXSEL_MASK_1			0x02
	#define		WM8750_R36_RMIXSEL_MASK_0			0x01
#define		WM8750_R37_RightOutMix2_ADDR		0x4a
	#define		WM8750_R36_RD2RO_BIT				0x01
	#define		WM8750_R36_RI2RO_BIT				0x80
	#define		WM8750_R36_RI2ROVOL_MASK			0x00	//MAX 6db 

#define		WM8750_R38_MonoOutMix1_ADDR		0x4c
#define		WM8750_R39_MonoOutMix2_ADDR		0x4e
#define		WM8750_R40_LOUT2Vol_ADDR		0x50
	#define		WM8750_R40_LO2VU_BIT				0x01
	#define		WM8750_R40_LO2ZC_BIT				0x80
	#define		WM8750_R40_LOUT2VOL_MASK			0x7f

#define		WM8750_R41_ROUT2Vol_ADDR		0x52
	#define		WM8750_R41_RO2VU_BIT				0x01
	#define		WM8750_R41_RO2ZC_BIT				0x80
	#define		WM8750_R41_ROUT2VOL_MASK			0x7f

#define		WM8750_R42_MONOOUT_ADDR			0x54


//#define		VOLUME_MAX		40

/*
unsigned char VolumeTable[VOLUME_MAX+1] = {

   0x00,
   0x3b, 0x42, 0x48, 0x4c, 0x53, 0x58, 0x5b, 0x5c, 0x5d, 0x5e,
   0x5f,  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
   0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,  0x70, 0x71, 0x72,
   0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c

};
*/
struct wm8750;

int wm8750_configure(struct wm8750 *uda, struct wm8750_cfg *conf);
int wm8750_mixer_ctl(struct wm8750 *uda, int cmd, void *arg);
int wm8750_open(struct wm8750 *uda);
void wm8750_close(struct wm8750 *uda);

struct wm8750 *wm8750_attach(const char *adapter);
void wm8750_detach(struct wm8750 *uda);

extern void WM8750_SEND_CMD(unsigned char hb, unsigned char lb);
extern void SPI_SCSB(int data);
extern void SPI_SDA(int data);
extern void SPI_SCLK(int data);


