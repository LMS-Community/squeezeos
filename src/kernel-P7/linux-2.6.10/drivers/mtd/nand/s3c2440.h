/*
 * Macros for low-level register control
 */
#define NFCONF_KeepMask			\
(					\
	m1NFCONF_AdvFlash	|	\
	m1NFCONF_PageSize	|	\
	m1NFCONF_AddrCycle	|	\
	m1NFCONF_BusWidth		\
)
#define NFCONF_InitSet						\
(								\
	sNFCONF_TACLS(0x7)	|	\
	sNFCONF_TWRPH0(0x7)	|	\
	sNFCONF_TWRPH1(0x7)		\
)

#define NFCONT_InitSet				\
(						\
	sNFCONT_LockTight(0)		|	\
	sNFCONT_SoftLock(0)		|	\
	sNFCONT_EnbIllegalAccINT(0)	|	\
	sNFCONT_EnbRnBINT(0)	 	|	\
	sNFCONT_RnB_TransMode(0)	|	\
	sNFCONT_SpareECCLock(0)	 	|	\
	sNFCONT_MainECCLock(0)	 	|	\
	sNFCONT_InitECC(0)	  	|	\
	sNFCONT_Reg_nCE(1)	  	|	\
	sNFCONT_MODE(1)				\
)
#endif				/* __S3C24X0_NAND_DEF_H__ */
