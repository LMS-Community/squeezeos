/*
 * s3c2440_nand.h
 * 
 * Copyright (C) 2004 Samsung Electronics Inc.
 *
 * s3c2440 NAND specific definiton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _S3C2440_NAND_H_
#define _S3C2440_NAND_H_


#define NFCONF		bNAND_CTL(0x00)
#define NFCONT		bNAND_CTL(0x04)
#define NFCMMD		bNAND_CTL(0x08)
#define NFADDR		bNAND_CTL(0x0c)
#define NFDATA8     __REGb(0x4e000000 + 0x10)
#define NFDATA32	bNAND_CTL(0x10)
#define NFMECCD0	bNAND_CTL(0x14)
#define NFMECCD1	bNAND_CTL(0x18)
#define NFSECCD		bNAND_CTL(0x1c)
#define NFSTAT		bNAND_CTL(0x20)
#define NFESTAT0	bNAND_CTL(0x24)
#define NFESTAT1	bNAND_CTL(0x28)
#define NFMECC0		bNAND_CTL(0x2c)
#define NFMECC1		bNAND_CTL(0x30)
#define NFSECC		bNAND_CTL(0x34)
#define NFSBLK		bNAND_CTL(0x38)
#define NFEBLK		bNAND_CTL(0x3c)


/*
 * NFCONF
 */
#define fNFCONF_TACLS		Fld(3,12)
#define fNFCONF_TWRPH0		Fld(3,8)
#define fNFCONF_TWRPH1		Fld(3,4)
#define fNFCONF_AdvFlash	Fld(1,3)
#define fNFCONF_PageSize	Fld(1,2)
#define fNFCONF_AddrCycle	Fld(1,1)
#define fNFCONF_BusWidth	Fld(1,0)

#define m1NFCONF_TACLS		FMsk(fNFCONF_TACLS)
#define m1NFCONF_TWRPH0		FMsk(fNFCONF_TWRPH0)
#define m1NFCONF_TWRPH1		FMsk(fNFCONF_TWRPH1)
#define m1NFCONF_AdvFlash	FMsk(fNFCONF_AdvFlash)
#define m1NFCONF_PageSize	FMsk(fNFCONF_PageSize)
#define m1NFCONF_AddrCycle	FMsk(fNFCONF_AddrCycle)
#define m1NFCONF_BusWidth	FMsk(fNFCONF_BusWidth)

#define m0NFCONF_TACLS		(~m1NFCONF_TACLS)
#define m0NFCONF_TWRPH0		(~m1NFCONF_TWRPH0)
#define m0NFCONF_TWRPH1		(~m1NFCONF_TWRPH1)
#define m0NFCONF_AdvFlash	(~m1NFCONF_AdvFlash)
#define m0NFCONF_PageSize	(~m1NFCONF_PageSize)
#define m0NFCONF_AddrCycle	(~m1NFCONF_AddrCycle)
#define m0NFCONF_BusWidth	(~m1NFCONF_BusWidth)

#define sNFCONF_TACLS(f_)	(FInsrt(f_,fNFCONF_TACLS)	& m1NFCONF_TACLS)
#define sNFCONF_TWRPH0(f_)	(FInsrt(f_,fNFCONF_TWRPH0)	& m1NFCONF_TWRPH0)
#define sNFCONF_TWRPH1(f_)	(FInsrt(f_,fNFCONF_TWRPH1)	& m1NFCONF_TWRPH1)


/*
 * NFCONT
 */
#define fNFCONT_LockTight		Fld(1,13)
#define fNFCONT_SoftLock		Fld(1,12)
#define fNFCONT_EnbIllegalAccINT	Fld(1,10)
#define fNFCONT_EnbRnBINT		Fld(1,9)
#define fNFCONT_RnB_TransMode		Fld(1,8)
#define fNFCONT_SpareECCLock		Fld(1,6)
#define fNFCONT_MainECCLock		Fld(1,5)
#define fNFCONT_InitECC			Fld(1,4)
#define fNFCONT_Reg_nCE			Fld(1,1)
#define fNFCONT_MODE			Fld(1,0)

#define m1NFCONT_LockTight		FMsk(fNFCONT_LockTight)
#define m1NFCONT_SoftLock		FMsk(fNFCONT_SoftLock)
#define m1NFCONT_EnbIllegalAccINT	FMsk(fNFCONT_EnbIllegalAccINT)
#define m1NFCONT_EnbRnBINT		FMsk(fNFCONT_EnbRnBINT)
#define m1NFCONT_RnB_TransMode		FMsk(fNFCONT_RnB_TransMode)
#define m1NFCONT_SpareECCLock		FMsk(fNFCONT_SpareECCLock)
#define m1NFCONT_MainECCLock		FMsk(fNFCONT_MainECCLock)
#define m1NFCONT_InitECC		FMsk(fNFCONT_InitECC)
#define m1NFCONT_Reg_nCE		FMsk(fNFCONT_Reg_nCE)
#define m1NFCONT_MODE			FMsk(fNFCONT_MODE)

#define m0NFCONT_LockTight		(~m1NFCONT_LockTight)
#define m0NFCONT_SoftLock		(~m1NFCONT_SoftLock)
#define m0NFCONT_EnbIllegalAccINT	(~m1NFCONT_EnbIllegalAccINT)
#define m0NFCONT_EnbRnBINT		(~m1NFCONT_EnbRnBINT)
#define m0NFCONT_RnB_TransMode		(~m1NFCONT_RnB_TransMode)
#define m0NFCONT_SpareECCLock		(~m1NFCONT_SpareECCLock)
#define m0NFCONT_MainECCLock		(~m1NFCONT_MainECCLock)
#define m0NFCONT_InitECC		(~m1NFCONT_InitECC)
#define m0NFCONT_Reg_nCE		(~m1NFCONT_Reg_nCE)
#define m0NFCONT_MODE			(~m1NFCONT_MODE)

#define sNFCONT_LockTight(f_)	(FInsrt(f_,fNFCONT_LockTight) & m1NFCONT_LockTight)
#define sNFCONT_SoftLock(f_)	(FInsrt(f_,fNFCONT_SoftLock) & m1NFCONT_SoftLock)
#define sNFCONT_EnbIllegalAccINT(f_)	(FInsrt(f_,fNFCONT_EnbIllegalAccINT)	& m1NFCONT_EnbIllegalAccINT)
#define sNFCONT_EnbRnBINT(f_)	(FInsrt(f_,fNFCONT_EnbRnBINT) & m1NFCONT_EnbRnBINT)
#define sNFCONT_RnB_TransMode(f_) (FInsrt(f_,fNFCONT_RnB_TransMode) & m1NFCONT_RnB_TransMode)
#define sNFCONT_SpareECCLock(f_) (FInsrt(f_,fNFCONT_SpareECCLock)& m1NFCONT_SpareECCLock)
#define sNFCONT_MainECCLock(f_)	(FInsrt(f_,fNFCONT_MainECCLock)	& m1NFCONT_MainECCLock)
#define sNFCONT_InitECC(f_)	(FInsrt(f_,fNFCONT_InitECC)& m1NFCONT_InitECC)
#define sNFCONT_Reg_nCE(f_)	(FInsrt(f_,fNFCONT_Reg_nCE)& m1NFCONT_Reg_nCE)
#define sNFCONT_MODE(f_)	(FInsrt(f_,fNFCONT_MODE)& m1NFCONT_MODE)


/*
 * NFCMMD
 */
#define fNFCMMD_B0		Fld(8,0)
#define m1NFCMMD_B0		FMsk(fNFCMMD_B0)
#define m0NFCMMD_B0		(~m1NFCMMD_B0)
#define sNFCMMD_B0(f_)	(FInsrt(f_,fNFCMMD_B0)	& m1NFCMMD_B0)


/*
 * NFADDR
 */
#define fNFADDR_B0		Fld(8,0)
#define m1NFADDR_B0		FMsk(fNFADDR_B0)
#define m0NFADDR_B0		(~m1NFADDR_B0)
#define sNFADDR_B0(f_)	(FInsrt(f_,fNFADDR_B0)	& m1NFADDR_B0)


/*
 * NFDATA
 */
#define fNFDATA_B3		Fld(8,24)
#define fNFDATA_B2		Fld(8,16)
#define fNFDATA_B1		Fld(8,8)
#define fNFDATA_B0		Fld(8,0)
#define fNFDATA_HW1		Fld(16,16)
#define fNFDATA_HW0		Fld(16,0)

#define m1NFDATA_B3		FMsk(fNFDATA_B3)
#define m1NFDATA_B2		FMsk(fNFDATA_B2)
#define m1NFDATA_B1		FMsk(fNFDATA_B1)
#define m1NFDATA_B0		FMsk(fNFDATA_B0)
#define m1NFDATA_HW1	FMsk(fNFDATA_HW1)
#define m1NFDATA_HW0	FMsk(fNFDATA_HW0)

#define m0NFDATA_B3		(~m1NFDATA_B3)
#define m0NFDATA_B2		(~m1NFDATA_B2)
#define m0NFDATA_B1		(~m1NFDATA_B1)
#define m0NFDATA_B0		(~m1NFDATA_B0)
#define m0NFDATA_HW1	(~m1NFDATA_HW1)
#define m0NFDATA_HW0	(~m1NFDATA_HW0)

#define sNFDATA_B3(f_)	(FInsrt(f_,fNFDATA_B3)	& m1NFDATA_B3)
#define sNFDATA_B2(f_)	(FInsrt(f_,fNFDATA_B2)	& m1NFDATA_B2)
#define sNFDATA_B1(f_)	(FInsrt(f_,fNFDATA_B1)	& m1NFDATA_B1)
#define sNFDATA_B0(f_)	(FInsrt(f_,fNFDATA_B0)	& m1NFDATA_B0)
#define sNFDATA_HW1(f_)	(FInsrt(f_,fNFDATA_HW1)	& m1NFDATA_HW1)
#define sNFDATA_HW0(f_)	(FInsrt(f_,fNFDATA_HW0)	& m1NFDATA_HW0)


/*
 * NFMECCD0
 */
#define fNFMECCD0_ECCData1_1	Fld(8,24)
#define fNFMECCD0_ECCData1_0	Fld(8,16)
#define fNFMECCD0_ECCData0_1	Fld(8,8)
#define fNFMECCD0_ECCData0_0	Fld(8,0)

#define m1NFMECCD0_ECCData1_1	FMsk(fNFMECCD0_ECCData1_1)
#define m1NFMECCD0_ECCData1_0	FMsk(fNFMECCD0_ECCData1_0)
#define m1NFMECCD0_ECCData0_1	FMsk(fNFMECCD0_ECCData0_1)
#define m1NFMECCD0_ECCData0_0	FMsk(fNFMECCD0_ECCData0_0)

#define m0NFMECCD0_ECCData1_1	(~m1NFMECCD0_ECCData1_1)
#define m0NFMECCD0_ECCData1_0	(~m1NFMECCD0_ECCData1_0)
#define m0NFMECCD0_ECCData0_1	(~m1NFMECCD0_ECCData0_1)
#define m0NFMECCD0_ECCData0_0	(~m1NFMECCD0_ECCData0_0)

#define sNFMECCD0_ECCData1_1(f_)	(FInsrt(f_,fNFMECCD0_ECCData1_1)	& m1NFMECCD0_ECCData1_1)
#define sNFMECCD0_ECCData1_0(f_)	(FInsrt(f_,fNFMECCD0_ECCData1_0)	& m1NFMECCD0_ECCData1_0)
#define sNFMECCD0_ECCData0_1(f_)	(FInsrt(f_,fNFMECCD0_ECCData0_1)	& m1NFMECCD0_ECCData0_1)
#define sNFMECCD0_ECCData0_0(f_)	(FInsrt(f_,fNFMECCD0_ECCData0_0)	& m1NFMECCD0_ECCData0_0)

/*
 * NFMECCD1
 */
#define fNFMECCD1_ECCData3_1	Fld(8,24)
#define fNFMECCD1_ECCData3_0	Fld(8,16)
#define fNFMECCD1_ECCData2_1	Fld(8,8)
#define fNFMECCD1_ECCData2_0	Fld(8,0)

#define m1NFMECCD1_ECCData3_1	FMsk(fNFMECCD1_ECCData3_1)
#define m1NFMECCD1_ECCData3_0	FMsk(fNFMECCD1_ECCData3_0)
#define m1NFMECCD1_ECCData2_1	FMsk(fNFMECCD1_ECCData2_1)
#define m1NFMECCD1_ECCData2_0	FMsk(fNFMECCD1_ECCData2_0)

#define m0NFMECCD1_ECCData3_1	(~m1NFMECCD1_ECCData3_1)
#define m0NFMECCD1_ECCData3_0	(~m1NFMECCD1_ECCData3_0)
#define m0NFMECCD1_ECCData2_1	(~m1NFMECCD1_ECCData2_1)
#define m0NFMECCD1_ECCData2_0	(~m1NFMECCD1_ECCData2_0)

#define sNFMECCD1_ECCData3_1(f_)	(FInsrt(f_,fNFMECCD1_ECCData3_1)	& m1NFMECCD1_ECCData3_1)
#define sNFMECCD1_ECCData3_0(f_)	(FInsrt(f_,fNFMECCD1_ECCData3_0)	& m1NFMECCD1_ECCData3_0)
#define sNFMECCD1_ECCData2_1(f_)	(FInsrt(f_,fNFMECCD1_ECCData2_1)	& m1NFMECCD1_ECCData2_1)
#define sNFMECCD1_ECCData2_0(f_)	(FInsrt(f_,fNFMECCD1_ECCData2_0)	& m1NFMECCD1_ECCData2_0)

/*
 * NFSECCD
 */
#define fNFSECCD_ECCData1_1	Fld(8,24)
#define fNFSECCD_ECCData1_0	Fld(8,16)
#define fNFSECCD_ECCData0_1	Fld(8,8)
#define fNFSECCD_ECCData0_0	Fld(8,0)

#define m1NFSECCD_ECCData1_1	FMsk(fNFSECCD_ECCData1_1)
#define m1NFSECCD_ECCData1_0	FMsk(fNFSECCD_ECCData1_0)
#define m1NFSECCD_ECCData0_1	FMsk(fNFSECCD_ECCData0_1)
#define m1NFSECCD_ECCData0_0	FMsk(fNFSECCD_ECCData0_0)

#define m0NFSECCD_ECCData1_1	(~m1NFSECCD_ECCData1_1)
#define m0NFSECCD_ECCData1_0	(~m1NFSECCD_ECCData1_0)
#define m0NFSECCD_ECCData0_1	(~m1NFSECCD_ECCData0_1)
#define m0NFSECCD_ECCData0_0	(~m1NFSECCD_ECCData0_0)

#define sNFSECCD_ECCData1_1(f_)	(FInsrt(f_,fNFSECCD_ECCData1_1)	& m1NFSECCD_ECCData1_1)
#define sNFSECCD_ECCData1_0(f_)	(FInsrt(f_,fNFSECCD_ECCData1_0)	& m1NFSECCD_ECCData1_0)
#define sNFSECCD_ECCData0_1(f_)	(FInsrt(f_,fNFSECCD_ECCData0_1)	& m1NFSECCD_ECCData0_1)
#define sNFSECCD_ECCData0_0(f_)	(FInsrt(f_,fNFSECCD_ECCData0_0)	& m1NFSECCD_ECCData0_0)


/*
 * NFSTAT
 */
#define fNFSTAT_IllegalAccess	Fld(1,3)
#define fNFSTAT_RnB_TransDetect	Fld(1,2)
#define fNFSTAT_nCE				Fld(1,1)
#define fNFSTAT_RnB				Fld(1,0)

#define m1NFSTAT_IllegalAccess		FMsk(fNFSTAT_IllegalAccess)
#define m1NFSTAT_RnB_TransDetect	FMsk(fNFSTAT_RnB_TransDetect)
#define m1NFSTAT_nCE				FMsk(fNFSTAT_nCE)
#define m1NFSTAT_RnB				FMsk(fNFSTAT_RnB)

#define m0NFSTAT_IllegalAccess		(~m1NFSTAT_IllegalAccess)
#define m0NFSTAT_RnB_TransDetect	(~m1NFSTAT_RnB_TransDetect)
#define m0NFSTAT_nCE				(~m1NFSTAT_nCE)
#define m0NFSTAT_RnB				(~m1NFSTAT_RnB)

#define sNFSTAT_IllegalAccess(f_)	(FInsrt(f_,fNFSTAT_IllegalAccess)	& m1NFSTAT_IllegalAccess)
#define sNFSTAT_RnB_TransDetect(f_)	(FInsrt(f_,fNFSTAT_RnB_TransDetect)	& m1NFSTAT_RnB_TransDetect)


/*
 * NFESTAT0
 */
#define fNFESTAT0_SErrorDataNo	Fld(4,21)
#define fNFESTAT0_SErrorBitNo	Fld(3,18)
#define fNFESTAT0_MErrorDataNo	Fld(11,7)
#define fNFESTAT0_MErrorBitNo	Fld(3,4)
#define fNFESTAT0_SpareError	Fld(2,2)
#define fNFESTAT0_MainError		Fld(2,0)

#define m1NFESTAT0_SErrorDataNo		FMsk(fNFESTAT0_SErrorDataNo)
#define m1NFESTAT0_SErrorBitNo		FMsk(fNFESTAT0_SErrorBitNo)
#define m1NFESTAT0_MErrorDataNo		FMsk(fNFESTAT0_MErrorDataNo)
#define m1NFESTAT0_MErrorBitNo		FMsk(fNFESTAT0_MErrorBitNo)
#define m1NFESTAT0_SpareError		FMsk(fNFESTAT0_SpareError)
#define m1NFESTAT0_MainError		FMsk(fNFESTAT0_MainError)

#define m0NFESTAT0_SErrorDataNo		(~m1NFESTAT0_SErrorDataNo)
#define m0NFESTAT0_SErrorBitNo		(~m1NFESTAT0_SErrorBitNo)
#define m0NFESTAT0_MErrorDataNo		(~m1NFESTAT0_MErrorDataNo)
#define m0NFESTAT0_MErrorBitNo		(~m1NFESTAT0_MErrorBitNo)
#define m0NFESTAT0_SpareError		(~m1NFESTAT0_SpareError)
#define m0NFESTAT0_MainError		(~m1NFESTAT0_MainError)

#define sNFESTAT0_SErrorDataNo(f_)	(FInsrt(f_,fNFESTAT0_SErrorDataNo)	& m1NFESTAT0_SErrorDataNo)
#define sNFESTAT0_SErrorBitNo(f_)	(FInsrt(f_,fNFESTAT0_SErrorBitNo)	& m1NFESTAT0_SErrorBitNo)
#define sNFESTAT0_MErrorDataNo(f_)	(FInsrt(f_,fNFESTAT0_MErrorDataNo)	& m1NFESTAT0_MErrorDataNo)
#define sNFESTAT0_MErrorBitNo(f_)	(FInsrt(f_,fNFESTAT0_MErrorBitNo)	& m1NFESTAT0_MErrorBitNo)
#define sNFESTAT0_SpareError(f_)	(FInsrt(f_,fNFESTAT0_SpareError)	& m1NFESTAT0_SpareError)
#define sNFESTAT0_MainError(f_)		(FInsrt(f_,fNFESTAT0_MainError)		& m1NFESTAT0_MainError)

/*
 * NFESTAT1
 */
#define fNFESTAT1_SErrorDataNo	Fld(4,21)
#define fNFESTAT1_SErrorBitNo	Fld(3,18)
#define fNFESTAT1_MErrorDataNo	Fld(11,7)
#define fNFESTAT1_MErrorBitNo	Fld(3,4)
#define fNFESTAT1_SpareError	Fld(2,2)
#define fNFESTAT1_MainError		Fld(2,0)

#define m1NFESTAT1_SErrorDataNo		FMsk(fNFESTAT1_SErrorDataNo)
#define m1NFESTAT1_SErrorBitNo		FMsk(fNFESTAT1_SErrorBitNo)
#define m1NFESTAT1_MErrorDataNo		FMsk(fNFESTAT1_MErrorDataNo)
#define m1NFESTAT1_MErrorBitNo		FMsk(fNFESTAT1_MErrorBitNo)
#define m1NFESTAT1_SpareError		FMsk(fNFESTAT1_SpareError)
#define m1NFESTAT1_MainError		FMsk(fNFESTAT1_MainError)

#define m0NFESTAT1_SErrorDataNo		(~m1NFESTAT1_SErrorDataNo)
#define m0NFESTAT1_SErrorBitNo		(~m1NFESTAT1_SErrorBitNo)
#define m0NFESTAT1_MErrorDataNo		(~m1NFESTAT1_MErrorDataNo)
#define m0NFESTAT1_MErrorBitNo		(~m1NFESTAT1_MErrorBitNo)
#define m0NFESTAT1_SpareError		(~m1NFESTAT1_SpareError)
#define m0NFESTAT1_MainError		(~m1NFESTAT1_MainError)

#define sNFESTAT1_SErrorDataNo(f_)	(FInsrt(f_,fNFESTAT1_SErrorDataNo)	& m1NFESTAT1_SErrorDataNo)
#define sNFESTAT1_SErrorBitNo(f_)	(FInsrt(f_,fNFESTAT1_SErrorBitNo)	& m1NFESTAT1_SErrorBitNo)
#define sNFESTAT1_MErrorDataNo(f_)	(FInsrt(f_,fNFESTAT1_MErrorDataNo)	& m1NFESTAT1_MErrorDataNo)
#define sNFESTAT1_MErrorBitNo(f_)	(FInsrt(f_,fNFESTAT1_MErrorBitNo)	& m1NFESTAT1_MErrorBitNo)
#define sNFESTAT1_SpareError(f_)	(FInsrt(f_,fNFESTAT1_SpareError)	& m1NFESTAT1_SpareError)
#define sNFESTAT1_MainError(f_)		(FInsrt(f_,fNFESTAT1_MainError)		& m1NFESTAT1_MainError)


/*
 * NFMECC0
 */
#define fNFMECC0_MECC0_3	Fld(8,24)
#define fNFMECC0_MECC0_2	Fld(8,16)
#define fNFMECC0_MECC0_1	Fld(8,8)
#define fNFMECC0_MECC0_0	Fld(8,0)

#define m1NFMECC0_MECC0_3	FMsk(fNFMECC0_MECC0_3)
#define m1NFMECC0_MECC0_2	FMsk(fNFMECC0_MECC0_2)
#define m1NFMECC0_MECC0_1	FMsk(fNFMECC0_MECC0_1)
#define m1NFMECC0_MECC0_0	FMsk(fNFMECC0_MECC0_0)

#define m0NFMECC0_MECC0_3	(~m1NFMECC0_MECC0_3)
#define m0NFMECC0_MECC0_2	(~m1NFMECC0_MECC0_2)
#define m0NFMECC0_MECC0_1	(~m1NFMECC0_MECC0_1)
#define m0NFMECC0_MECC0_0	(~m1NFMECC0_MECC0_0)

#define sNFMECC0_MECC0_3(f_)	(FInsrt(f_,fNFMECC0_MECC0_3)	& m1NFMECC0_MECC0_3)
#define sNFMECC0_MECC0_2(f_)	(FInsrt(f_,fNFMECC0_MECC0_2)	& m1NFMECC0_MECC0_2)
#define sNFMECC0_MECC0_1(f_)	(FInsrt(f_,fNFMECC0_MECC0_1)	& m1NFMECC0_MECC0_1)
#define sNFMECC0_MECC0_0(f_)	(FInsrt(f_,fNFMECC0_MECC0_0)	& m1NFMECC0_MECC0_0)

/*
 * NFMECC1
 */
#define fNFMECC1_MECC1_3	Fld(8,24)
#define fNFMECC1_MECC1_2	Fld(8,16)
#define fNFMECC1_MECC1_1	Fld(8,8)
#define fNFMECC1_MECC1_0	Fld(8,0)

#define m1NFMECC1_MECC1_3	FMsk(fNFMECC1_MECC1_3)
#define m1NFMECC1_MECC1_2	FMsk(fNFMECC1_MECC1_2)
#define m1NFMECC1_MECC1_1	FMsk(fNFMECC1_MECC1_1)
#define m1NFMECC1_MECC1_0	FMsk(fNFMECC1_MECC1_0)

#define m0NFMECC1_MECC1_3	(~m1NFMECC1_MECC1_3)
#define m0NFMECC1_MECC1_2	(~m1NFMECC1_MECC1_2)
#define m0NFMECC1_MECC1_1	(~m1NFMECC1_MECC1_1)
#define m0NFMECC1_MECC1_0	(~m1NFMECC1_MECC1_0)

#define sNFMECC1_MECC1_3(f_)	(FInsrt(f_,fNFMECC1_MECC1_3)	& m1NFMECC1_MECC1_3)
#define sNFMECC1_MECC1_2(f_)	(FInsrt(f_,fNFMECC1_MECC1_2)	& m1NFMECC1_MECC1_2)
#define sNFMECC1_MECC1_1(f_)	(FInsrt(f_,fNFMECC1_MECC1_1)	& m1NFMECC1_MECC1_1)
#define sNFMECC1_MECC1_0(f_)	(FInsrt(f_,fNFMECC1_MECC1_0)	& m1NFMECC1_MECC1_0)

/*
 * NFSECC
 */
#define fNFSECC_SECC1_1	Fld(8,24)
#define fNFSECC_SECC1_0	Fld(8,16)
#define fNFSECC_SECC0_1	Fld(8,8)
#define fNFSECC_SECC0_0	Fld(8,0)

#define m1NFSECC_SECC1_1	FMsk(fNFSECC_SECC1_1)
#define m1NFSECC_SECC1_0	FMsk(fNFSECC_SECC1_0)
#define m1NFSECC_SECC0_1	FMsk(fNFSECC_SECC0_1)
#define m1NFSECC_SECC0_0	FMsk(fNFSECC_SECC0_0)

#define m0NFSECC_SECC1_1	(~m1NFSECC_SECC1_1)
#define m0NFSECC_SECC1_0	(~m1NFSECC_SECC1_0)
#define m0NFSECC_SECC0_1	(~m1NFSECC_SECC0_1)
#define m0NFSECC_SECC0_0	(~m1NFSECC_SECC0_0)

#define sNFSECC_SECC1_1(f_)	(FInsrt(f_,fNFSECC_SECC1_1)	& m1NFSECC_SECC1_1)
#define sNFSECC_SECC1_0(f_)	(FInsrt(f_,fNFSECC_SECC1_0)	& m1NFSECC_SECC1_0)
#define sNFSECC_SECC0_1(f_)	(FInsrt(f_,fNFSECC_SECC0_1)	& m1NFSECC_SECC0_1)
#define sNFSECC_SECC0_0(f_)	(FInsrt(f_,fNFSECC_SECC0_0)	& m1NFSECC_SECC0_0)


/*
 * NFSBLK
 */
#define fNFSBLK_SBLK_ADDR2	Fld(8,16)
#define fNFSBLK_SBLK_ADDR1	Fld(8,8)
#define fNFSBLK_SBLK_ADDR0	Fld(8,0)

#define m1NFSBLK_SBLK_ADDR2	FMsk(fNFSBLK_SBLK_ADDR2)
#define m1NFSBLK_SBLK_ADDR1	FMsk(fNFSBLK_SBLK_ADDR1)
#define m1NFSBLK_SBLK_ADDR0	FMsk(fNFSBLK_SBLK_ADDR0)

#define m0NFSBLK_SBLK_ADDR2	(~m1NFSBLK_SBLK_ADDR2)
#define m0NFSBLK_SBLK_ADDR1	(~m1NFSBLK_SBLK_ADDR1)
#define m0NFSBLK_SBLK_ADDR0	(~m1NFSBLK_SBLK_ADDR0)

#define sNFSBLK_SBLK_ADDR2(f_)	(FInsrt(f_,fNFSBLK_SBLK_ADDR2)	& m1NFSBLK_SBLK_ADDR2)
#define sNFSBLK_SBLK_ADDR1(f_)	(FInsrt(f_,fNFSBLK_SBLK_ADDR1)	& m1NFSBLK_SBLK_ADDR1)
#define sNFSBLK_SBLK_ADDR0(f_)	(FInsrt(f_,fNFSBLK_SBLK_ADDR0)	& m1NFSBLK_SBLK_ADDR0)

/*
 * NFEBLK
 */
#define fNFEBLK_EBLK_ADDR2	Fld(8,16)
#define fNFEBLK_EBLK_ADDR1	Fld(8,8)
#define fNFEBLK_EBLK_ADDR0	Fld(8,0)

#define m1NFEBLK_EBLK_ADDR2	FMsk(fNFEBLK_EBLK_ADDR2)
#define m1NFEBLK_EBLK_ADDR1	FMsk(fNFEBLK_EBLK_ADDR1)
#define m1NFEBLK_EBLK_ADDR0	FMsk(fNFEBLK_EBLK_ADDR0)

#define m0NFEBLK_EBLK_ADDR2	(~m1NFEBLK_EBLK_ADDR2)
#define m0NFEBLK_EBLK_ADDR1	(~m1NFEBLK_EBLK_ADDR1)
#define m0NFEBLK_EBLK_ADDR0	(~m1NFEBLK_EBLK_ADDR0)

#define sNFEBLK_EBLK_ADDR2(f_)	(FInsrt(f_,fNFEBLK_EBLK_ADDR2)	& m1NFEBLK_EBLK_ADDR2)
#define sNFEBLK_EBLK_ADDR1(f_)	(FInsrt(f_,fNFEBLK_EBLK_ADDR1)	& m1NFEBLK_EBLK_ADDR1)
#define sNFEBLK_EBLK_ADDR0(f_)	(FInsrt(f_,fNFEBLK_EBLK_ADDR0)	& m1NFEBLK_EBLK_ADDR0)


#endif

