/*
 * Automatically generated C config: don't edit
 * Linux kernel version: 2.6.10
 * Mon Nov 27 10:10:56 2006
 */
#define AUTOCONF_INCLUDED
#define CONFIG_ARM 1
#define CONFIG_MMU 1
#define CONFIG_UID16 1
#define CONFIG_RWSEM_GENERIC_SPINLOCK 1
#define CONFIG_GENERIC_CALIBRATE_DELAY 1
#define CONFIG_GENERIC_IOMAP 1

/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1
#define CONFIG_CLEAN_COMPILE 1
#define CONFIG_BROKEN_ON_SMP 1
#define CONFIG_LOCK_KERNEL 1

/*
 * General setup
 */
#define CONFIG_LOCALVERSION ""
#undef CONFIG_SWAP
#define CONFIG_SYSVIPC 1
#undef CONFIG_POSIX_MQUEUE
#undef CONFIG_BSD_PROCESS_ACCT
#define CONFIG_SYSCTL 1
#undef CONFIG_AUDIT
#define CONFIG_LOG_BUF_SHIFT 14
#undef CONFIG_HOTPLUG
#define CONFIG_KOBJECT_UEVENT 1
#undef CONFIG_IKCONFIG
#define CONFIG_EMBEDDED 1
#define CONFIG_KALLSYMS 1
#undef CONFIG_KALLSYMS_ALL
#undef CONFIG_KALLSYMS_EXTRA_PASS
#define CONFIG_FUTEX 1
#define CONFIG_EPOLL 1
#define CONFIG_CC_OPTIMIZE_FOR_SIZE 1
#define CONFIG_SHMEM 1
#define CONFIG_CC_ALIGN_FUNCTIONS 0
#define CONFIG_CC_ALIGN_LABELS 0
#define CONFIG_CC_ALIGN_LOOPS 0
#define CONFIG_CC_ALIGN_JUMPS 0
#undef CONFIG_TINY_SHMEM

/*
 * Loadable module support
 */
#define CONFIG_MODULES 1
#define CONFIG_MODULE_UNLOAD 1
#define CONFIG_MODULE_FORCE_UNLOAD 1
#define CONFIG_OBSOLETE_MODPARM 1
#undef CONFIG_MODVERSIONS
#undef CONFIG_MODULE_SRCVERSION_ALL
#undef CONFIG_KMOD

/*
 * System Type
 */
#undef CONFIG_ARCH_CLPS7500
#undef CONFIG_ARCH_CLPS711X
#undef CONFIG_ARCH_CO285
#undef CONFIG_ARCH_EBSA110
#undef CONFIG_ARCH_CAMELOT
#undef CONFIG_ARCH_FOOTBRIDGE
#undef CONFIG_ARCH_INTEGRATOR
#undef CONFIG_ARCH_IOP3XX
#undef CONFIG_ARCH_IXP4XX
#undef CONFIG_ARCH_IXP2000
#undef CONFIG_ARCH_L7200
#undef CONFIG_ARCH_PXA
#undef CONFIG_ARCH_RPC
#undef CONFIG_ARCH_SA1100
#undef CONFIG_ARCH_S3C2440
#undef CONFIG_ARCH_S3C2410
#define CONFIG_ARCH_S3C2413 1
#undef CONFIG_ARCH_SHARK
#undef CONFIG_ARCH_LH7A40X
#undef CONFIG_ARCH_OMAP
#undef CONFIG_ARCH_VERSATILE
#undef CONFIG_ARCH_IMX
#undef CONFIG_ARCH_H720X

/*
 * S3C24XX Implementations
 */
#define CONFIG_ARCH_SMDK2413 1
#undef CONFIG_ARCH_SMDK2411
#define CONFIG_CPU_S3C2413 1

/*
 * S3C2413 Setup
 */
#define CONFIG_S3C2413_DMA 1
#undef CONFIG_S3C2413_DMA_DEBUG
#define CONFIG_S3C2413_LOWLEVEL_UART_PORT 0

/*
 * Processor Type
 */
#define CONFIG_CPU_32 1
#define CONFIG_CPU_ARM926T 1
#define CONFIG_CPU_32v5 1
#define CONFIG_CPU_ABRT_EV5TJ 1
#define CONFIG_CPU_CACHE_VIVT 1
#define CONFIG_CPU_COPY_V4WB 1
#define CONFIG_CPU_TLB_V4WBI 1

/*
 * Processor Features
 */
#define CONFIG_ARM_THUMB 1
#undef CONFIG_CPU_ICACHE_DISABLE
#undef CONFIG_CPU_DCACHE_DISABLE
#undef CONFIG_CPU_DCACHE_WRITETHROUGH
#undef CONFIG_CPU_CACHE_ROUND_ROBIN

/*
 * General setup
 */
#define CONFIG_ISA 1
#define CONFIG_ZBOOT_ROM_TEXT 0x0
#define CONFIG_ZBOOT_ROM_BSS 0x0
#undef CONFIG_XIP_KERNEL

/*
 * At least one math emulation must be selected
 */
#define CONFIG_FPE_NWFPE 1
#define CONFIG_FPE_NWFPE_XP 1
#define CONFIG_FPE_FASTFPE 1
#undef CONFIG_VFP
#define CONFIG_BINFMT_ELF 1
#define CONFIG_BINFMT_AOUT 1
#undef CONFIG_BINFMT_MISC

/*
 * Generic Driver Options
 */
#define CONFIG_STANDALONE 1
#define CONFIG_PREVENT_FIRMWARE_BUILD 1
#undef CONFIG_DEBUG_DRIVER
#undef CONFIG_PM
#define CONFIG_PREEMPT 1
#undef CONFIG_ARTHUR
#define CONFIG_CMDLINE "root=/dev/mtdblock2 console=ttySAC0,115200 mem=64M init=/linuxrc"
#define CONFIG_ALIGNMENT_TRAP 1

/*
 * Parallel port support
 */
#undef CONFIG_PARPORT

/*
 * Memory Technology Devices (MTD)
 */
#define CONFIG_MTD 1
#undef CONFIG_MTD_DEBUG
#define CONFIG_MTD_PARTITIONS 1
#define CONFIG_MTD_CONCAT 1
#undef CONFIG_MTD_REDBOOT_PARTS
#undef CONFIG_MTD_CMDLINE_PARTS
#undef CONFIG_MTD_AFS_PARTS

/*
 * User Modules And Translation Layers
 */
#define CONFIG_MTD_CHAR 1
#define CONFIG_MTD_BLOCK 1
#undef CONFIG_FTL
#undef CONFIG_NFTL
#undef CONFIG_INFTL

/*
 * RAM/ROM/Flash chip drivers
 */
#define CONFIG_MTD_CFI 1
#undef CONFIG_MTD_JEDECPROBE
#define CONFIG_MTD_GEN_PROBE 1
#undef CONFIG_MTD_CFI_ADV_OPTIONS
#define CONFIG_MTD_MAP_BANK_WIDTH_1 1
#define CONFIG_MTD_MAP_BANK_WIDTH_2 1
#define CONFIG_MTD_MAP_BANK_WIDTH_4 1
#undef CONFIG_MTD_MAP_BANK_WIDTH_8
#undef CONFIG_MTD_MAP_BANK_WIDTH_16
#undef CONFIG_MTD_MAP_BANK_WIDTH_32
#define CONFIG_MTD_CFI_I1 1
#define CONFIG_MTD_CFI_I2 1
#undef CONFIG_MTD_CFI_I4
#undef CONFIG_MTD_CFI_I8
#define CONFIG_MTD_CFI_INTELEXT 1
#undef CONFIG_MTD_CFI_AMDSTD
#undef CONFIG_MTD_CFI_STAA
#define CONFIG_MTD_CFI_UTIL 1
#undef CONFIG_MTD_RAM
#undef CONFIG_MTD_ROM
#undef CONFIG_MTD_ABSENT

/*
 * Mapping drivers for chip access
 */
#undef CONFIG_MTD_COMPLEX_MAPPINGS
#undef CONFIG_MTD_PHYSMAP
#undef CONFIG_MTD_ARM_INTEGRATOR
#undef CONFIG_MTD_EDB7312

/*
 * Self-contained MTD device drivers
 */
#undef CONFIG_MTD_SLRAM
#undef CONFIG_MTD_PHRAM
#undef CONFIG_MTD_MTDRAM
#undef CONFIG_MTD_BLKMTD

/*
 * Disk-On-Chip Device Drivers
 */
#undef CONFIG_MTD_DOC2000
#undef CONFIG_MTD_DOC2001
#undef CONFIG_MTD_DOC2001PLUS

/*
 * NAND Flash Device Drivers
 */
#define CONFIG_MTD_NAND 1
#undef CONFIG_MTD_NAND_VERIFY_WRITE
#define CONFIG_MTD_NAND_IDS 1
#define CONFIG_MTD_NAND_S3C2413 1
#undef CONFIG_MTD_NAND_S3C2413_DEBUG
#undef CONFIG_S3C24XX_LARGEPAGE_NAND
#define CONFIG_MTD_NAND_S3C24XX_DISABLE_ECC 1
#undef CONFIG_MTD_NAND_DISKONCHIP

/*
 * Plug and Play support
 */
#undef CONFIG_PNP

/*
 * Block devices
 */
#undef CONFIG_BLK_DEV_FD
#undef CONFIG_BLK_DEV_XD
#define CONFIG_BLK_DEV_LOOP 1
#undef CONFIG_BLK_DEV_CRYPTOLOOP
#undef CONFIG_BLK_DEV_NBD
#undef CONFIG_BLK_DEV_UB
#define CONFIG_BLK_DEV_RAM 1
#define CONFIG_BLK_DEV_RAM_COUNT 16
#define CONFIG_BLK_DEV_RAM_SIZE 8192
#define CONFIG_BLK_DEV_INITRD 1
#define CONFIG_INITRAMFS_SOURCE ""
#undef CONFIG_CDROM_PKTCDVD

/*
 * IO Schedulers
 */
#define CONFIG_IOSCHED_NOOP 1
#define CONFIG_IOSCHED_AS 1
#define CONFIG_IOSCHED_DEADLINE 1
#define CONFIG_IOSCHED_CFQ 1

/*
 * Multi-device support (RAID and LVM)
 */
#undef CONFIG_MD

/*
 * Networking support
 */
#define CONFIG_NET 1

/*
 * Networking options
 */
#define CONFIG_PACKET 1
#undef CONFIG_PACKET_MMAP
#undef CONFIG_NETLINK_DEV
#define CONFIG_UNIX 1
#undef CONFIG_NET_KEY
#define CONFIG_INET 1
#undef CONFIG_IP_MULTICAST
#undef CONFIG_IP_ADVANCED_ROUTER
#define CONFIG_IP_PNP 1
#undef CONFIG_IP_PNP_DHCP
#define CONFIG_IP_PNP_BOOTP 1
#undef CONFIG_IP_PNP_RARP
#undef CONFIG_NET_IPIP
#undef CONFIG_NET_IPGRE
#undef CONFIG_ARPD
#undef CONFIG_SYN_COOKIES
#undef CONFIG_INET_AH
#undef CONFIG_INET_ESP
#undef CONFIG_INET_IPCOMP
#undef CONFIG_INET_TUNNEL
#define CONFIG_IP_TCPDIAG 1
#undef CONFIG_IP_TCPDIAG_IPV6
#undef CONFIG_IPV6
#undef CONFIG_NETFILTER

/*
 * SCTP Configuration (EXPERIMENTAL)
 */
#undef CONFIG_IP_SCTP
#undef CONFIG_ATM
#undef CONFIG_BRIDGE
#undef CONFIG_VLAN_8021Q
#undef CONFIG_DECNET
#undef CONFIG_LLC2
#undef CONFIG_IPX
#undef CONFIG_ATALK
#undef CONFIG_X25
#undef CONFIG_LAPB
#undef CONFIG_NET_DIVERT
#undef CONFIG_ECONET
#undef CONFIG_WAN_ROUTER

/*
 * QoS and/or fair queueing
 */
#undef CONFIG_NET_SCHED
#undef CONFIG_NET_CLS_ROUTE

/*
 * Network testing
 */
#undef CONFIG_NET_PKTGEN
#undef CONFIG_NETPOLL
#undef CONFIG_NET_POLL_CONTROLLER
#undef CONFIG_HAMRADIO
#undef CONFIG_IRDA
#undef CONFIG_BT
#define CONFIG_NETDEVICES 1
#undef CONFIG_DUMMY
#undef CONFIG_BONDING
#undef CONFIG_EQUALIZER
#undef CONFIG_TUN

/*
 * ARCnet devices
 */
#undef CONFIG_ARCNET

/*
 * Ethernet (10 or 100Mbit)
 */
#define CONFIG_NET_ETHERNET 1
#undef CONFIG_MII
#undef CONFIG_NET_VENDOR_3COM
#undef CONFIG_LANCE
#undef CONFIG_NET_VENDOR_SMC
#undef CONFIG_SMC91X
#undef CONFIG_NET_VENDOR_RACAL
#undef CONFIG_AT1700
#undef CONFIG_DEPCA
#undef CONFIG_HP100
#undef CONFIG_NET_ISA
#define CONFIG_NET_PCI 1
#undef CONFIG_AC3200
#undef CONFIG_APRICOT
#define CONFIG_CS89x0 1
#undef CONFIG_NET_POCKET

/*
 * Ethernet (1000 Mbit)
 */

/*
 * Ethernet (10000 Mbit)
 */

/*
 * Token Ring devices
 */
#undef CONFIG_TR

/*
 * Wireless LAN (non-hamradio)
 */
#define CONFIG_NET_RADIO 1

/*
 * Obsolete Wireless cards support (pre-802.11)
 */
#undef CONFIG_STRIP
#undef CONFIG_ARLAN
#undef CONFIG_WAVELAN

/*
 * Wireless 802.11b ISA/PCI cards support
 */
#undef CONFIG_ATMEL
#define CONFIG_NET_WIRELESS 1

/*
 * Wan interfaces
 */
#undef CONFIG_WAN
#undef CONFIG_PPP
#undef CONFIG_SLIP
#undef CONFIG_SHAPER
#undef CONFIG_NETCONSOLE

/*
 * SCSI device support
 */
#define CONFIG_SCSI 1
#define CONFIG_SCSI_PROC_FS 1

/*
 * SCSI support type (disk, tape, CD-ROM)
 */
#define CONFIG_BLK_DEV_SD 1
#undef CONFIG_CHR_DEV_ST
#undef CONFIG_CHR_DEV_OSST
#undef CONFIG_BLK_DEV_SR
#define CONFIG_CHR_DEV_SG 1

/*
 * Some SCSI devices (e.g. CD jukebox) support multiple LUNs
 */
#undef CONFIG_SCSI_MULTI_LUN
#undef CONFIG_SCSI_CONSTANTS
#undef CONFIG_SCSI_LOGGING

/*
 * SCSI Transport Attributes
 */
#undef CONFIG_SCSI_SPI_ATTRS
#undef CONFIG_SCSI_FC_ATTRS

/*
 * SCSI low-level drivers
 */
#undef CONFIG_SCSI_7000FASST
#undef CONFIG_SCSI_AHA152X
#undef CONFIG_SCSI_AHA1542
#undef CONFIG_SCSI_AIC7XXX_OLD
#undef CONFIG_SCSI_IN2000
#define CONFIG_SCSI_SATA 1
#undef CONFIG_SCSI_BUSLOGIC
#undef CONFIG_SCSI_DTC3280
#undef CONFIG_SCSI_EATA
#undef CONFIG_SCSI_EATA_PIO
#undef CONFIG_SCSI_FUTURE_DOMAIN
#undef CONFIG_SCSI_GDTH
#undef CONFIG_SCSI_GENERIC_NCR5380
#undef CONFIG_SCSI_GENERIC_NCR5380_MMIO
#undef CONFIG_SCSI_NCR53C406A
#undef CONFIG_SCSI_PAS16
#undef CONFIG_SCSI_PSI240I
#undef CONFIG_SCSI_QLOGIC_FAS
#undef CONFIG_SCSI_SYM53C416
#undef CONFIG_SCSI_T128
#undef CONFIG_SCSI_U14_34F
#undef CONFIG_SCSI_DEBUG

/*
 * Fusion MPT device support
 */

/*
 * IEEE 1394 (FireWire) support
 */

/*
 * I2O device support
 */

/*
 * ISDN subsystem
 */
#undef CONFIG_ISDN

/*
 * Input device support
 */
#define CONFIG_INPUT 1

/*
 * Userland interfaces
 */
#undef CONFIG_INPUT_MOUSEDEV
#undef CONFIG_INPUT_JOYDEV
#undef CONFIG_INPUT_TSDEV
#define CONFIG_INPUT_EVDEV 1
#define CONFIG_INPUT_EVBUG 1

/*
 * Input I/O drivers
 */
#undef CONFIG_GAMEPORT
#define CONFIG_SOUND_GAMEPORT 1
#define CONFIG_SERIO 1
#define CONFIG_SERIO_SERPORT 1
#undef CONFIG_SERIO_CT82C710
#undef CONFIG_SERIO_RAW

/*
 * Input Device Drivers
 */
#undef CONFIG_INPUT_KEYBOARD
#undef CONFIG_INPUT_MOUSE
#undef CONFIG_INPUT_JOYSTICK
#define CONFIG_INPUT_TOUCHSCREEN 1
#undef CONFIG_TOUCHSCREEN_GUNZE
#undef CONFIG_TOUCHSCREEN_S3C2440
#define CONFIG_TOUCHSCREEN_S3C2413 1
#define CONFIG_S3C2413_TSHOOK 1
#define CONFIG_S3C2413_TS_OLD_API 1
#undef CONFIG_S3C2413_ADC_POLLING
#undef CONFIG_INPUT_MISC

/*
 * Character devices
 */
#define CONFIG_VT 1
#define CONFIG_VT_CONSOLE 1
#define CONFIG_HW_CONSOLE 1
#define CONFIG_SERIAL_NONSTANDARD 1
#undef CONFIG_COMPUTONE
#undef CONFIG_ROCKETPORT
#undef CONFIG_CYCLADES
#undef CONFIG_DIGIEPCA
#undef CONFIG_DIGI
#undef CONFIG_ESPSERIAL
#undef CONFIG_MOXA_INTELLIO
#undef CONFIG_MOXA_SMARTIO
#undef CONFIG_SYNCLINKMP
#undef CONFIG_N_HDLC
#undef CONFIG_RISCOM8
#undef CONFIG_SPECIALIX
#undef CONFIG_SX
#undef CONFIG_RIO
#undef CONFIG_STALDRV

/*
 * Serial drivers
 */
#undef CONFIG_SERIAL_8250

/*
 * Non-8250 serial port support
 */
#define CONFIG_SERIAL_S3C2413 1
#define CONFIG_SERIAL_S3C2413_CONSOLE 1
#define CONFIG_SERIAL_CORE 1
#define CONFIG_SERIAL_CORE_CONSOLE 1
#define CONFIG_UNIX98_PTYS 1
#define CONFIG_LEGACY_PTYS 1
#define CONFIG_LEGACY_PTY_COUNT 16

/*
 * IPMI
 */
#undef CONFIG_IPMI_HANDLER

/*
 * Watchdog Cards
 */
#undef CONFIG_WATCHDOG
#undef CONFIG_NVRAM
#undef CONFIG_RTC
#define CONFIG_S3C2413_RTC 1
#undef CONFIG_DTLK
#undef CONFIG_R3964

/*
 * Ftape, the floppy tape device driver
 */
#undef CONFIG_DRM
#undef CONFIG_RAW_DRIVER

/*
 * Yamu Solutions Driver
 */
#define CONFIG_YAMU_DRIVER 1
#define CONFIG_KeyBackLightDIMM 1
#define CONFIG_MATRIX 1
//#define CONFIG_HOLD 1
#define CONFIG_BATTERY 1
//#define CONFIG_LCDP 1
#define CONFIG_HDET 1
//#define CONFIG_DACP 1
//#define CONFIG_AMPP 1
//#define CONFIG_MuteTR 1
#define CONFIG_WLANP 1
#define CONFIG_MOTION 1
#define CONFIG_LCDDIMM 1
#define CONFIG_IRTX 1
//#define CONFIG_IRRX 1
#define CONFIG_WHEEL 1
#define CONFIG_PMODE 1
#define CONFIG_LcdReset 1
//#define CONFIG_LCDBacklightEn 1
//#define CONFIG_MuteFET 1
#define CONFIG_NANDWP 1
//#define CONFIG_AG_OUT 1
//#define CONFIG_BT_PRIORITY 1
#define CONFIG_CHG_ACPR 1
#define CONFIG_CHG_HPWR 1
#define CONFIG_CHG_PPR 1
#define CONFIG_CHG_SUSP 1
#define CONFIG_DOCKDET 1
#define CONFIG_WLAN_nRESET 1
#define CONFIG_WLAN_SPI_nINT 1
#define CONFIG_WM8750_REG 1
#define CONFIG_DOCKDET_AD  1
#define CONFIG_WLAN_DC_EN 1
#define CONFIG_WLAN_SLEEP_CLK  1
#define CONFIG_SW_PW_OFF  1
#define CONFIG_ACCEL  1

/*
 * I2C support
 */
#define CONFIG_I2C 1
#define CONFIG_I2C_CHARDEV 1

/*
 * I2C Algorithms
 */
#undef CONFIG_I2C_ALGOBIT
#undef CONFIG_I2C_ALGOPCF
#undef CONFIG_I2C_ALGOPCA

/*
 * I2C Hardware Bus support
 */
#undef CONFIG_I2C_ELEKTOR
#undef CONFIG_I2C_ISA
#undef CONFIG_I2C_PARPORT_LIGHT
#undef CONFIG_I2C_S3C2413
#undef CONFIG_I2C_STUB
#undef CONFIG_I2C_PCA_ISA

/*
 * Hardware Sensors Chip support
 */
#undef CONFIG_I2C_SENSOR
#undef CONFIG_SENSORS_ADM1021
#undef CONFIG_SENSORS_ADM1025
#undef CONFIG_SENSORS_ADM1026
#undef CONFIG_SENSORS_ADM1031
#undef CONFIG_SENSORS_ASB100
#undef CONFIG_SENSORS_DS1621
#undef CONFIG_SENSORS_FSCHER
#undef CONFIG_SENSORS_GL518SM
#undef CONFIG_SENSORS_IT87
#undef CONFIG_SENSORS_LM63
#undef CONFIG_SENSORS_LM75
#undef CONFIG_SENSORS_LM77
#undef CONFIG_SENSORS_LM78
#undef CONFIG_SENSORS_LM80
#undef CONFIG_SENSORS_LM83
#undef CONFIG_SENSORS_LM85
#undef CONFIG_SENSORS_LM87
#undef CONFIG_SENSORS_LM90
#undef CONFIG_SENSORS_MAX1619
#undef CONFIG_SENSORS_PC87360
#undef CONFIG_SENSORS_SMSC47M1
#undef CONFIG_SENSORS_W83781D
#undef CONFIG_SENSORS_W83L785TS
#undef CONFIG_SENSORS_W83627HF

/*
 * Other I2C Chip support
 */
#undef CONFIG_SENSORS_EEPROM
#undef CONFIG_SENSORS_PCF8574
#undef CONFIG_SENSORS_PCF8591
#undef CONFIG_SENSORS_RTC8564
#define CONFIG_I2C_DEBUG_CORE 1
#define CONFIG_I2C_DEBUG_ALGO 1
#define CONFIG_I2C_DEBUG_BUS 1
#define CONFIG_I2C_DEBUG_CHIP 1

/*
 * SPI support
 */
#undef CONFIG_SPI

/*
 * L3 serial bus support
 */
#define CONFIG_L3 1
#define CONFIG_L3_ALGOBIT 1
#define CONFIG_L3_BIT_S3C2413_GPIO 1

/*
 * Multimedia devices
 */
#undef CONFIG_VIDEO_DEV

/*
 * Digital Video Broadcasting Devices
 */
#undef CONFIG_DVB

/*
 * File systems
 */
#define CONFIG_EXT2_FS 1
#undef CONFIG_EXT2_FS_XATTR
#undef CONFIG_EXT3_FS
#undef CONFIG_JBD
#undef CONFIG_REISERFS_FS
#undef CONFIG_JFS_FS
#undef CONFIG_XFS_FS
#define CONFIG_MINIX_FS 1
#define CONFIG_ROMFS_FS 1
#undef CONFIG_QUOTA
#define CONFIG_DNOTIFY 1
#undef CONFIG_AUTOFS_FS
#undef CONFIG_AUTOFS4_FS

/*
 * CD-ROM/DVD Filesystems
 */
#undef CONFIG_ISO9660_FS
#undef CONFIG_UDF_FS

/*
 * DOS/FAT/NT Filesystems
 */
#define CONFIG_FAT_FS 1
#define CONFIG_MSDOS_FS 1
#define CONFIG_VFAT_FS 1
#define CONFIG_FAT_DEFAULT_CODEPAGE 437
#define CONFIG_FAT_DEFAULT_IOCHARSET "iso8859-1"
#undef CONFIG_NTFS_FS

/*
 * Pseudo filesystems
 */
#define CONFIG_PROC_FS 1
#define CONFIG_SYSFS 1
#define CONFIG_DEVFS_FS 1
#define CONFIG_DEVFS_MOUNT 1
#define CONFIG_DEVFS_DEBUG 1
#undef CONFIG_DEVPTS_FS_XATTR
#define CONFIG_TMPFS 1
#define CONFIG_TMPFS_XATTR 1
#undef CONFIG_TMPFS_SECURITY
#undef CONFIG_HUGETLB_PAGE
#define CONFIG_RAMFS 1

/*
 * Miscellaneous filesystems
 */
#undef CONFIG_ADFS_FS
#undef CONFIG_AFFS_FS
#undef CONFIG_HFS_FS
#undef CONFIG_HFSPLUS_FS
#undef CONFIG_BEFS_FS
#undef CONFIG_BFS_FS
#undef CONFIG_EFS_FS
#undef CONFIG_JFFS_FS
#undef CONFIG_JFFS2_FS
#define CONFIG_YAFFS_FS 1
#define CONFIG_YAFFS_YAFFS1 1
#define CONFIG_YAFFS_DOES_ECC 1
#undef CONFIG_YAFFS_ECC_WRONG_ORDER
#define CONFIG_YAFFS_YAFFS2 1
#define CONFIG_YAFFS_AUTO_YAFFS2 1
#undef CONFIG_YAFFS_DISABLE_WIDE_TNODES
#define CONFIG_YAFFS_DISABLE_CHUNK_ERASED_CHECK 1
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM 1
#define CONFIG_CRAMFS 1
#undef CONFIG_VXFS_FS
#undef CONFIG_HPFS_FS
#undef CONFIG_QNX4FS_FS
#undef CONFIG_SYSV_FS
#undef CONFIG_UFS_FS

/*
 * Network File Systems
 */
#define CONFIG_NFS_FS 1
#define CONFIG_NFS_V3 1
#undef CONFIG_NFS_V4
#undef CONFIG_NFS_DIRECTIO
#undef CONFIG_NFSD
#define CONFIG_ROOT_NFS 1
#define CONFIG_LOCKD 1
#define CONFIG_LOCKD_V4 1
#undef CONFIG_EXPORTFS
#define CONFIG_SUNRPC 1
#undef CONFIG_RPCSEC_GSS_KRB5
#undef CONFIG_RPCSEC_GSS_SPKM3
#undef CONFIG_SMB_FS
#undef CONFIG_CIFS
#undef CONFIG_NCP_FS
#undef CONFIG_CODA_FS
#undef CONFIG_AFS_FS

/*
 * Partition Types
 */
#define CONFIG_PARTITION_ADVANCED 1
#undef CONFIG_ACORN_PARTITION
#undef CONFIG_OSF_PARTITION
#undef CONFIG_AMIGA_PARTITION
#undef CONFIG_ATARI_PARTITION
#undef CONFIG_MAC_PARTITION
#define CONFIG_MSDOS_PARTITION 1
#undef CONFIG_BSD_DISKLABEL
#undef CONFIG_MINIX_SUBPARTITION
#undef CONFIG_SOLARIS_X86_PARTITION
#undef CONFIG_UNIXWARE_DISKLABEL
#undef CONFIG_LDM_PARTITION
#undef CONFIG_SGI_PARTITION
#undef CONFIG_ULTRIX_PARTITION
#undef CONFIG_SUN_PARTITION
#undef CONFIG_EFI_PARTITION

/*
 * Native Language Support
 */
#define CONFIG_NLS 1
#define CONFIG_NLS_DEFAULT "iso8859-1"
#define CONFIG_NLS_CODEPAGE_437 1
#undef CONFIG_NLS_CODEPAGE_737
#undef CONFIG_NLS_CODEPAGE_775
#undef CONFIG_NLS_CODEPAGE_850
#undef CONFIG_NLS_CODEPAGE_852
#undef CONFIG_NLS_CODEPAGE_855
#undef CONFIG_NLS_CODEPAGE_857
#undef CONFIG_NLS_CODEPAGE_860
#undef CONFIG_NLS_CODEPAGE_861
#undef CONFIG_NLS_CODEPAGE_862
#undef CONFIG_NLS_CODEPAGE_863
#undef CONFIG_NLS_CODEPAGE_864
#undef CONFIG_NLS_CODEPAGE_865
#undef CONFIG_NLS_CODEPAGE_866
#undef CONFIG_NLS_CODEPAGE_869
#undef CONFIG_NLS_CODEPAGE_936
#undef CONFIG_NLS_CODEPAGE_950
#undef CONFIG_NLS_CODEPAGE_932
#undef CONFIG_NLS_CODEPAGE_949
#undef CONFIG_NLS_CODEPAGE_874
#undef CONFIG_NLS_ISO8859_8
#undef CONFIG_NLS_CODEPAGE_1250
#undef CONFIG_NLS_CODEPAGE_1251
#define CONFIG_NLS_ASCII 1
#define CONFIG_NLS_ISO8859_1 1
#undef CONFIG_NLS_ISO8859_2
#undef CONFIG_NLS_ISO8859_3
#undef CONFIG_NLS_ISO8859_4
#undef CONFIG_NLS_ISO8859_5
#undef CONFIG_NLS_ISO8859_6
#undef CONFIG_NLS_ISO8859_7
#undef CONFIG_NLS_ISO8859_9
#undef CONFIG_NLS_ISO8859_13
#undef CONFIG_NLS_ISO8859_14
#undef CONFIG_NLS_ISO8859_15
#undef CONFIG_NLS_KOI8_R
#undef CONFIG_NLS_KOI8_U
#undef CONFIG_NLS_UTF8

/*
 * Profiling support
 */
#undef CONFIG_PROFILING

/*
 * Graphics support
 */
#define CONFIG_FB 1
#undef CONFIG_FB_MODE_HELPERS
#undef CONFIG_FB_TILEBLITTING
#undef CONFIG_FB_SMDK2413
#define CONFIG_FB_SMDK2413_PMG 1
#define CONFIG_FB_SMDK2413_PMG24 1
#undef CONFIG_FB_SMDK2413_DOUBLE
#undef CONFIG_FB_SMDK2413_24BIT
#undef CONFIG_FB_SMDK2411
#undef CONFIG_FB_VIRTUAL

/*
 * Console display driver support
 */
#undef CONFIG_VGA_CONSOLE
#undef CONFIG_MDA_CONSOLE
#define CONFIG_DUMMY_CONSOLE 1
#undef CONFIG_FRAMEBUFFER_CONSOLE

/*
 * Logo configuration
 */
#undef CONFIG_LOGO

/*
 * Sound
 */
#define CONFIG_SOUND 1

/*
 * Advanced Linux Sound Architecture
 */
#undef CONFIG_SND

/*
 * Open Sound System
 */
#define CONFIG_SOUND_PRIME 1
#undef CONFIG_SOUND_BT878
#undef CONFIG_SOUND_FUSION
#undef CONFIG_SOUND_CS4281
#undef CONFIG_SOUND_SONICVIBES
#undef CONFIG_SOUND_TRIDENT
#undef CONFIG_SOUND_MSNDCLAS
#undef CONFIG_SOUND_MSNDPIN
#undef CONFIG_SOUND_OSS
#undef CONFIG_SOUND_TVMIXER
#define CONFIG_SOUND_S3C2413 1
#undef CONFIG_SOUND_UDA1341
#define CONFIG_SOUND_PCM1770 1
#define CONFIG_SOUND_2413_PCM1770 1
#define CONFIG_SOUND_WM8750 1
#define CONFIG_SOUND_2413_WM8750 1
#undef CONFIG_SOUND_AD1980

/*
 * Misc devices
 */

/*
 * USB support
 */
#define CONFIG_USB 1
#undef CONFIG_USB_DEBUG

/*
 * Miscellaneous USB options
 */
#undef CONFIG_USB_DEVICEFS
#undef CONFIG_USB_BANDWIDTH
#undef CONFIG_USB_DYNAMIC_MINORS
#undef CONFIG_USB_OTG
#define CONFIG_USB_ARCH_HAS_HCD 1
#define CONFIG_USB_ARCH_HAS_OHCI 1

/*
 * USB Host Controller Drivers
 */
#undef CONFIG_USB_OHCI_HCD
#undef CONFIG_USB_SL811_HCD

/*
 * USB Device Class drivers
 */
#undef CONFIG_USB_AUDIO
#undef CONFIG_USB_BLUETOOTH_TTY
#undef CONFIG_USB_MIDI
#undef CONFIG_USB_ACM
#undef CONFIG_USB_PRINTER

/*
 * NOTE: USB_STORAGE enables SCSI, and 'SCSI disk support' may also be needed; see USB_STORAGE Help for more information
 */
#define CONFIG_USB_STORAGE 1
#undef CONFIG_USB_STORAGE_DEBUG
#undef CONFIG_USB_STORAGE_RW_DETECT
#undef CONFIG_USB_STORAGE_DATAFAB
#undef CONFIG_USB_STORAGE_FREECOM
#undef CONFIG_USB_STORAGE_DPCM
#undef CONFIG_USB_STORAGE_HP8200e
#undef CONFIG_USB_STORAGE_SDDR09
#undef CONFIG_USB_STORAGE_SDDR55
#undef CONFIG_USB_STORAGE_JUMPSHOT

/*
 * USB Input Devices
 */
#undef CONFIG_USB_HID

/*
 * USB HID Boot Protocol drivers
 */
#undef CONFIG_USB_KBD
#undef CONFIG_USB_MOUSE
#undef CONFIG_USB_AIPTEK
#undef CONFIG_USB_WACOM
#undef CONFIG_USB_KBTAB
#undef CONFIG_USB_POWERMATE
#undef CONFIG_USB_MTOUCH
#undef CONFIG_USB_EGALAX
#undef CONFIG_USB_XPAD
#undef CONFIG_USB_ATI_REMOTE

/*
 * USB Imaging devices
 */
#undef CONFIG_USB_MDC800
#undef CONFIG_USB_MICROTEK
#undef CONFIG_USB_HPUSBSCSI

/*
 * USB Multimedia devices
 */
#undef CONFIG_USB_DABUSB

/*
 * Video4Linux support is needed for USB Multimedia device support
 */

/*
 * USB Network Adapters
 */
#undef CONFIG_USB_CATC
#undef CONFIG_USB_KAWETH
#undef CONFIG_USB_PEGASUS
#undef CONFIG_USB_RTL8150
#undef CONFIG_USB_USBNET

/*
 * USB port drivers
 */

/*
 * USB Serial Converter support
 */
#undef CONFIG_USB_SERIAL

/*
 * USB Miscellaneous drivers
 */
#undef CONFIG_USB_EMI62
#undef CONFIG_USB_EMI26
#undef CONFIG_USB_TIGL
#undef CONFIG_USB_AUERSWALD
#undef CONFIG_USB_RIO500
#undef CONFIG_USB_LEGOTOWER
#undef CONFIG_USB_LCD
#undef CONFIG_USB_LED
#undef CONFIG_USB_CYTHERM
#undef CONFIG_USB_PHIDGETKIT
#undef CONFIG_USB_PHIDGETSERVO

/*
 * USB ATM/DSL drivers
 */

/*
 * USB Gadget Support
 */
#define CONFIG_USB_GADGET_MODULE 1
#undef CONFIG_USB_GADGET_DEBUG_FILES
#undef CONFIG_USB_GADGET_NET2280
#undef CONFIG_USB_GADGET_PXA2XX
#undef CONFIG_USB_GADGET_GOKU
#undef CONFIG_USB_GADGET_SA1100
#undef CONFIG_USB_GADGET_LH7A40X
#undef CONFIG_USB_GADGET_S3C2410
#undef CONFIG_USB_GADGET_S3C2440
#define CONFIG_USB_GADGET_S3C2413 1
#define CONFIG_USB_S3C2413_MODULE 1
#undef CONFIG_USB_GADGET_DUMMY_HCD
#undef CONFIG_USB_GADGET_OMAP
#define CONFIG_USB_GADGET_DUALSPEED 1
#undef CONFIG_USB_ZERO
#undef CONFIG_USB_ETH
#undef CONFIG_USB_GADGETFS
#define CONFIG_USB_FILE_STORAGE_MODULE 1
#undef CONFIG_USB_FILE_STORAGE_TEST
#undef CONFIG_USB_G_SERIAL

/*
 * MMC/SD Card support
 */
#define CONFIG_MMC 1
#undef CONFIG_MMC_DEBUG
#define CONFIG_MMC_BLOCK 1
#define CONFIG_MMC_S3C2413 1
#undef CONFIG_MMC_WBSD

/*
 * Kernel hacking
 */
#define CONFIG_DEBUG_KERNEL 1
#define CONFIG_MAGIC_SYSRQ 1
#undef CONFIG_SCHEDSTATS
#undef CONFIG_DEBUG_SLAB
#undef CONFIG_DEBUG_SPINLOCK
#undef CONFIG_DEBUG_KOBJECT
#undef CONFIG_DEBUG_BUGVERBOSE
#undef CONFIG_DEBUG_INFO
#define CONFIG_FRAME_POINTER 1
#define CONFIG_DEBUG_USER 1
#undef CONFIG_DEBUG_WAITQ
#define CONFIG_DEBUG_ERRORS 1
#define CONFIG_DEBUG_LL 1
#undef CONFIG_DEBUG_ICEDCC

/*
 * Security options
 */
#undef CONFIG_KEYS
#undef CONFIG_SECURITY

/*
 * Cryptographic options
 */
#undef CONFIG_CRYPTO

/*
 * Library routines
 */
#define CONFIG_CRC_CCITT 1
#define CONFIG_CRC32 1
#undef CONFIG_LIBCRC32C
#define CONFIG_ZLIB_INFLATE 1
