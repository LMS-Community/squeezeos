#ifndef __PIN2413_H__
#define __PIN2413_H__

#include "def.h"
/*****************************************************************************/
/* Boundary Scan Cell number of S3C2413                                      */
/*****************************************************************************/

#define S2413_MAX_CELL_INDEX	484	

#define  DATA0_OUT   	(414)
#define  DATA0_IN   	(415) 
#define  DATA1_OUT      (416)
#define  DATA1_IN		(417) 
#define  DATA2_OUT		(418) 
#define  DATA2_IN       (419) 
#define  DATA3_OUT      (420)
#define  DATA3_IN       (421) 
#define  DATA4_OUT      (422)
#define  DATA4_IN       (423) 
#define  DATA5_OUT      (424) 
#define  DATA5_IN       (425)
#define  DATA6_OUT      (426)
#define  DATA6_IN       (427) 
#define  DATA7_OUT      (428)
#define  DATA7_IN       (430)
#define  DATA0_7_CON    (429)

#define  DATA8_OUT      (431)
#define  DATA8_IN       (432) 
#define  DATA9_OUT      (433)
#define  DATA9_IN       (434) 
#define  DATA10_OUT     (435)
#define  DATA10_IN      (436)
#define  DATA11_OUT     (437)
#define  DATA11_IN      (438)
#define  DATA12_OUT     (439)
#define  DATA12_IN      (440)
#define  DATA13_OUT     (441)
#define  DATA13_IN      (442)
#define  DATA14_OUT     (443)
#define  DATA14_IN      (444)
#define  DATA15_OUT     (445)
#define  DATA15_IN      (447)
#define  DATA8_15_CON   (446)

  
#define  ADDR0		  (374)
#define  ADDR0_CON    (375) //Because of tri-state type cell. Should be 'LOW'

#define  ADDR1		  (376)
#define  ADDR2	      (377)
#define  ADDR3	      (378)
#define  ADDR4	      (379)
#define  ADDR5	      (380)
#define  ADDR6	      (381)
#define  ADDR7	      (382)
#define  ADDR8	      (383)
#define  ADDR9	      (384)  
#define  ADDR10	      (385)
#define  ADDR11	      (386)
#define  ADDR12	      (387)
#define  ADDR13	      (388)
#define  ADDR14	      (389)
#define  ADDR15       (390)
#define  ADDR1_15_CON (391)

#define  ADDR16	      (392)
#define  ADDR16_CON   (393)

#define  ADDR17	      (394)
#define  ADDR17_CON   (395)

#define  ADDR18	      (396)
#define  ADDR18_CON   (397)
#define  ADDR19       (398)
#define  ADDR19_CON   (399)
#define  ADDR20       (400)  
#define  ADDR20_CON   (401)
#define  ADDR21	      (402)
#define  ADDR21_CON   (403)
#define  ADDR22       (404)
#define  ADDR22_CON   (405)
#define  ADDR23       (406)
#define  ADDR23_CON   (407)
#define  ADDR24       (408)
#define  ADDR24_CON   (409)
#define  ADDR25       (410)  
#define  ADDR25_CON   (411)
#define  ADDR26       (412)  
#define  ADDR26_CON   (413)

#define  CLE		 	(332)
#define  CLE_CON	 	(333)
#define  ALE			(330)
#define  ALE_CON		(331)
#define  FWE			(328)
#define  FWE_CON		(329)
#define  FRE		    (326)
#define  FRE_CON  	    (327)
#define  RNB			(323)


#define  GCSn0    		(348)
#define  GCSn0_CON		(349)
#define  GCSn1		    (346)
#define  GCSn1_CON	    (347) 
#define  GCSn2	   		(344)
#define  GCSn2_CON		(345) 
#define  GCSn3	   		(342)
#define  GCSn3_CON		(343) 
#define  GCSn4    		(340)
#define  GCSn4_CON		(341)
#define  GCSn5		    (338)
#define  GCSn6	   		(337)
#define  GCSn7	   		(336)
#define  GCSn5_7_CON	(339) 


#define  NBE0   	    (366)
#define  NBE1           (367)
#define  NBE2   	    (368)
#define  NBE3           (369)
#define  NBE0_3_CON     (370)

#define	 NWE	  	    (362)
#define	 NWE_CON 	    (363)
#define	 NOE 		    (364)
#define	 NOE_CON	    (365)
#define	 WAIT	  		(335)


/*****************************************************************************/
/* Exported Functions                                                        */
/*****************************************************************************/
void S2413_InitCell(void);
void S2413_SetPin(int index, char value);
char S2413_GetPin(int index);

void S2413_SetAddr(U32 addr);

void S2413_SetDataByte(U8);
void S2413_SetDataHW(U16);
void S2413_SetDataWord(U32);

U8 S2413_GetDataByte(void);
U16 S2413_GetDataHW(void);
U32 S2413_GetDataWord(void);

extern char outCellValue[S2413_MAX_CELL_INDEX+2];
extern char inCellValue[S2413_MAX_CELL_INDEX+2];
extern int  dataOutCellIndex[16];
extern int  dataInCellIndex[16];
extern int  addrCellIndex[27];

// MACRO for speed up
//#define S2410_SetPin(index,value)   outCellValue[index] = value
//#define S2410_GetPin(index)	    inCellValue[index]


#endif  //__PIN24a0_H__
