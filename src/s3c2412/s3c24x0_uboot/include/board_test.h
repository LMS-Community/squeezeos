//=============================================================================
// File Name : boardtest.h
// Function  : S3C2412 board Define Address Register
// History
//   0.0 : Programming start 
//=============================================================================

#ifndef __BOARDTEST_H__
#define __BOARDTEST_H__


extern int usb_upgrade_dnw( char mode); 
extern int nandw_upgrade(ulong startblk,ulong size, ulong memadr);
extern int go_upgrade(void);

extern unsigned long upgrade_size;
extern int upgrade_status;







#endif








