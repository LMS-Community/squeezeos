
/* handy sizes */
#define SZ_1K                           0x00000400
#define SZ_4K                           0x00001000
#define SZ_8K                           0x00002000
#define SZ_16K                          0x00004000
#define SZ_32K                          0x00008000
#define SZ_64K                          0x00010000
#define SZ_128K                         0x00020000
#define SZ_256K                         0x00040000
#define SZ_512K                         0x00080000

#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000
#define SZ_64M                          0x04000000
#define SZ_128M                         0x08000000
#define SZ_256M                         0x10000000
#define SZ_512M                         0x20000000

#define SZ_1G                           0x40000000
#define SZ_2G                           0x80000000

#if defined(CONFIG_S3C24A0A)
#define ROM_BASE0               0x00000000      /* base address of rom bank 0 */
#define ROM_BASE1               0x04000000      /* base address of rom bank 1 */
#define DRAM_BASE0              0x10000000      /* base address of dram bank 0 */
#define DRAM_BASE1              0x20000000      /* base address of dram bank 1 */
#elif defined(CONFIG_S3C2440A)||defined(CONFIG_S3C2410)
#define ROM_BASE0               0x00000000      /* base address of rom bank 0 */
#define ROM_BASE1               0x08000000      /* base address of rom bank 1 */
#define DRAM_BASE0              0x30000000      /* base address of dram bank 0 */
#endif


#define DRAM_BASE               DRAM_BASE0

#define DRAM_SIZE               SZ_64M

/* total memory required by vivi */
#define UBOOT_SIZE		(2*1024*1024)
/* size of vivi binary */
#define MAX_UBOOT_BIN_SIZE	(256*1024)
#define MMU_TLB_SIZE		(16*1024)
#define HEAP_SIZE		(1*1024*1024)
#define STACK_SIZE		(512*1024)

/* base address for vivi */
#define UBOOT_BASE		(DRAM_BASE + DRAM_SIZE - UBOOT_SIZE)
//#define UBOOT_BASE	        TEXT_BASE	
#define MMU_TLB_BASE		(UBOOT_BASE + MAX_UBOOT_BIN_SIZE)
#define HEAP_BASE		(UBOOT_BASE + (512*1024))
#define STACK_BASE		(UBOOT_BASE + UBOOT_SIZE - STACK_SIZE)

#define RAM_SIZE		(UBOOT_BASE - DRAM_BASE)
#define RAM_BASE		DRAM_BASE

