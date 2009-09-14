/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*
 * otg/hardware/mxc-lnx.h
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/mxc/mxc-lnx.h|20070726000034|21621
 *
 * 	Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */

/*!
 * @file otg/hardware/mxc-lnx.h
 * @brief Hardware defines for Freescale USBOTG Hardware
 *
 * This supports the Freescale MXC implementation of the Trans Dimension
 * USBOTG.
 *
 * This is used on:
 *
 *      - i.MX21
 *      - SCM-A11
 *      - Argon+
 *      - Zeus
 *
 * @ingroup FSOTG
 * @ingroup LINUXAPI
 */

#if defined(CONFIG_ARCH_MX2ADS)
#include <linux/pci.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpt.h>
#undef GPT_BASE_ADDR
#include <asm/arch/mx2.h>
//#include <otghw/mx2ads.h>
//#include <otghw/mx2ads-hardware.h>
#endif /* defined(CONFIG_ARCH_MX2ADS) */

//#if defined(CONFIG_ARCH_SCMA11) || defined(CONFIG_ARCH_ARGONPLUS) || defined(CONFIG_ARCH_ARGONLV) || defined(CONFIG_ARCH_ZEUS)
#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91131)
#include <linux/pci.h>
//#include <otghw/scma11.h>
//#include <otghw/scma11-hardware.h>
#endif /* defined(CONFIG_ARCH_SCMA11) */



#if defined(CONFIG_ARCH_MX2ADS)

/*!
 * @name MX2ETDS
 * R1 = i.MX21 Application Processor Reference Manual Rev 0.6 2003/11/01 TO 1.0
 *      (aka "IMX21RM_TO1.pdf")
 * R2 = "USBOTG_L3_Specification_v.C1.0.pdf"
 * @{
 */
#define NUM_ETDS                32
#define DATA_BUFF_SIZE          64
#define DATA_BUFFER_TOTAL       4096
#define NUM_DATA_BUFFS          (4096/DATA_BUFF_SIZE)
#define LITTLE_ENDIAN           1


#define UDC_NAME                "MX21"
#define UDC_MAX_ENDPOINTS       32
//#define EP0_PACKETSIZE          8
#define EP0_PACKETSIZE          64


/* @} */

/*!
 * @name MX2Interrupts
 * @{
 */
#define OTG_USBWKUP                 53
#define OTG_USBDMA                  54
#define OTG_USBHOST                 55
#define OTG_USBFUNC                 56
#define OTG_USBHNP                  57
#define OTG_USBCTRL                 58
/* @} */

/*!
 * @name I2CTransceiver
 * C.f. 23.14.10 I2C OTG Transceiver Controller Registers
 * These are the I2C controller and access registers.
 *
 * N.B. I2C_ERROR is not documented.
 * @{
 */

#define I2C_BUSY                                (1 << 7)
#define I2C_ERROR                               (1 << 2)
#define I2C_HWSMODE                             (1 << 1)
#define I2C_I2COE                               (1 << 0)

#define I2C_SCLK_TO_SCL_DIVISION                (OTG_I2C_BASE+0x1E)

#define I2C_INTERRUPT_AND_CONTROL               (OTG_I2C_BASE+0x1F)

#define I2C_STATUS_MASK                         (0x7)

#define I2C_NOACK_EN                            (1 << 6)
#define I2C_RWREADY_EN                          (1 << 5)
#define I2C_OTGXCVRINT_EN                       (1 << 4)

#define I2C_NOACK                               (1 << 2)
#define I2C_RWREADY                             (1 << 1)
#define I2C_OTGXCVRINT                          (1 << 0)

#define MX2_OTG_XCVR_DEVAD                      OTG_I2C_BASE+0x18
#define MX2_SEQ_OP_REG                          OTG_I2C_BASE+0x19
#define MX2_SEQ_RD_STARTAD                      OTG_I2C_BASE+0x1a
#define MX2_I2C_OP_CTRL_REG                     OTG_I2C_BASE+0x1b
#define MX2_SCLK_TO_SCL_HPER                    OTG_I2C_BASE+0x1e
#define MX2_I2C_INTERRUPT_AND_CTRL              OTG_I2C_BASE+0x1f

/* @} */

#endif /* defined(CONFIG_ARCH_MX2ADS) */


//#if defined(CONFIG_ARCH_SCMA11) || defined(CONFIG_ARCH_ARGONPLUS) || defined(CONFIG_ARCH_ARGONLV) || defined(CONFIG_ARCH_ZEUS)
#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91131)

/*!
 * @name MX2ETDS
 * R1 = i.MX21 Application Processor Reference Manual Rev 0.6 2003/11/01 TO 1.0
 *      (aka "IMX21RM_TO1.pdf")
 * R2 = "USBOTG_L3_Specification_v.C1.0.pdf"
 * @{
 */

#define OTG_USBDMA                  54

#define NUM_ETDS                16
#define DATA_BUFF_SIZE          64
#define DATA_BUFFER_TOTAL       4096
#define NUM_DATA_BUFFS          (4096/DATA_BUFF_SIZE)
#define LITTLE_ENDIAN           1


#define UDC_NAME                "MXC"
#define UDC_MAX_ENDPOINTS       16
#define EP0_PACKETSIZE          64


/* @} */


#if defined (CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
#define OTG_BASE_ADDR                                     0x50020000
#define INT_USB_WAKEUP          35
#define INT_USB_SOF             36
#define INT_PMU_EVTMON          37
#define INT_USB_FUNC            38
#define INT_USB_DMA             39
#define INT_USB_CTRL            40
#define _reg_GPT_GPTCR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x00)))
#define _reg_GPT_GPTPR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x04)))
#define _reg_GPT_GPTSR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x08)))
#define _reg_GPT_GPTIR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x0C)))
#define _reg_GPT_GPTOCR1  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x10)))
#define _reg_GPT_GPTOCR2  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x14)))
#define _reg_GPT_GPTOCR3  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x18)))
#define _reg_GPT_GPTICR1  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x1C)))
#define _reg_GPT_GPTICR2  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x20)))
#define _reg_GPT_GPTCNT   ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x24)))

#elif defined (CONFIG_ARCH_MXC91231)
#define OTG_BASE_ADDR                                     0x50024000

#elif defined (CONFIG_ARCH_MXC91131)
#define OTG_BASE_ADDR                                     0x50024000

#define _reg_GPT_GPTCR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x00)))
#define _reg_GPT_GPTPR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x04)))
#define _reg_GPT_GPTSR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x08)))
#define _reg_GPT_GPTIR    ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x0C)))
#define _reg_GPT_GPTOCR1  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x10)))
#define _reg_GPT_GPTOCR2  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x14)))
#define _reg_GPT_GPTOCR3  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x18)))
#define _reg_GPT_GPTICR1  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x1C)))
#define _reg_GPT_GPTICR2  ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x20)))
#define _reg_GPT_GPTCNT   ((volatile __u32 *)(IO_ADDRESS(GPT_BASE_ADDR + 0x24)))


#endif /* defined (CONFIG_ARCH_XXXX) */

#define OTG_CORE_BASE                                     (OTG_BASE_ADDR+0x000)                //  base location for core
#define OTG_FUNC_BASE                                     (OTG_BASE_ADDR+0x040)                //  base location for function
#define OTG_HOST_BASE                                     (OTG_BASE_ADDR+0x080)                //  base location for host
#define OTG_DMA_BASE                                      (OTG_BASE_ADDR+0x800)                //  base location for dma
#define OTG_ETD_BASE                                      (OTG_BASE_ADDR+0x200)                //  base location for etd memory
#define OTG_EP_BASE                                       (OTG_BASE_ADDR+0x400)                //  base location for ep memory
#define OTG_SYS_BASE                                      (OTG_BASE_ADDR+0x600)                //  base location for system

//#if defined (CONFIG_ARCH_SCMA11)
#if defined (CONFIG_ARCH_MXC91231)
#define OTG_DATA_BASE                                     (OTG_BASE_ADDR+0x4000)               //  base location for data memory
#else /* defined (CONFIG_ARCH_SCMA11) */
#define OTG_DATA_BASE                                     (OTG_BASE_ADDR+0x1000)               //  base location for data memory
#endif /* defined (CONFIG_ARCH_SCMA11) */

#define OTG_SYS_CTRL                                      (OTG_SYS_BASE+0x000)                 //  base location for system

/* @} */

#endif /* defined(CONFIG_ARCH_SCMA11) */


/*!
 * @name C.f. 23.11.11 Host Registers
 */
 /*! @{ */

/* ep descriptor access
 */
static __inline__ u32 etd_word(int n, int word)
{
        u32 offset = n * 16;
        offset += word * 4;
        return OTG_ETD_BASE + offset;
}

/* ep descriptor access
 */
static __inline__ u32 ep_word(int n, int dir, int word)
{
        u32 offset = n * 2;
        offset += dir ? 1 : 0;
        offset *= 16;
        offset += word * 4;
        return OTG_EP_BASE + offset;
}

/* endpoint data buffer access
 *
 * This is a simplistic allocator, will do until we want to support ISO or host mode.
 *
 * This works because we are assuming a maximum of 16 allocate endpoints, and no
 * overlapped endpoints (both in and out are allocated).
 */

#define VOLATILE
static VOLATILE __inline__ u16 data_x_buf(int n, int dir)
{
        return 0x40 * (n * 4 + 2 * (dir ? 1 : 0));
}
static VOLATILE __inline__ u16 data_y_buf(int n, int dir)
{
        return 0x40 * (n * 4 + 2 * (dir ? 1 : 0) + 1);
}

static VOLATILE __inline__ u8 * data_x_address(int n, int dir)
{
        return (VOLATILE u8 *) IO_ADDRESS(OTG_DATA_BASE + data_x_buf(n, dir));
}
static VOLATILE __inline__ u8 * data_y_address(int n, int dir)
{
        return (VOLATILE u8 *) IO_ADDRESS(OTG_DATA_BASE + data_y_buf(n, dir));
}
/*! @} */


/* ********************************************************************************************** */

#if 0
/*!
 * rel_data_buff() - release data buff
 */
static __inline__ fs_data_buff *rel_data_buff(fs_hcpriv *fs_hcpriv, fs_data_buff *db)
{
        // Release db to the pool of available data_buffs.
        unsigned long flags;
        u16 ndx;
        if (NULL != db) {
                local_irq_save(flags);
                ndx = db - (fs_data_buff *)OTG_DATA_BASE;
                fs_hcpriv->buff_list[ndx] = fs_hcpriv->free_buffs;
                fs_hcpriv->free_buffs = ndx;
#if 0
                db->next = fs_hcpriv->free_buffs;
                fs_hcpriv->free_buffs = db;
#endif
                local_irq_restore(flags);
        }
        return(NULL);
}

/*!
 * data_buff_boff() - get data buffer offset given address
 */
static __inline__ u16 data_buff_boff(fs_data_buff *db)
{
        return((u16)(((void *) db) - ((void *) OTG_DATA_BASE)));
}

/*!
 * data_buff_addr() - get data buffer address given offset
 */
static __inline__ fs_data_buff *data_buff_addr(u16 boff)
{
        // Return the address of the fs_data_buffer that is boff bytes from the start of data buffer memory.
        return(boff + ((void *) OTG_DATA_BASE));
}
/*! @} */
#endif

/* ********************************************************************************************** */

/*!
 * @name FSUSBOTGIO
 * @brief Freescale USBOTG I/O support.
 */
 /*! @{ */

/*!
 * fs_rb() - read a byte
 * @param port
 * @return byte read
 */
static u8 __inline__ fs_rb(u32 port)
{
        return *(volatile u8 *) (IO_ADDRESS(port));
}

/*!
 * fs_rl() - read a long
 * @param port
 * @return word read
 */
static u32  __inline__ fs_rl(u32 port)
{
        return *(volatile u32 *) (IO_ADDRESS(port));
}

/*!
 * fs_wb() - write a byte
 * @param port
 * @param val
 */
static void __inline__ fs_wb(u32 port, u8 val)
{
        *(volatile u8 *)(IO_ADDRESS(port)) = val;
}

/*!
 * fs_orb() - or a byte
 * @param port
 * @param val
 */
static void __inline__ fs_orb(u32 port, u8 val)
{
        u8 set =  fs_rb(port) | val;
        *(volatile u8 *)(IO_ADDRESS(port)) = set;
}

/*!
 * fs_andb() - and a byte
 * @param port
 * @param val
 */
static void __inline__ fs_andb(u32 port, u8 val)
{
        u8 set =  fs_rb(port) & val;
        *(volatile u8 *)(IO_ADDRESS(port)) = set;
}

/*!
 * fs_wl() - write a long
 * @param port
 * @param val
 */
static void __inline__ fs_wl(u32 port, u32 val)
{
        u32 set;
        *(volatile u32 *)(IO_ADDRESS(port)) = val;
        set = fs_rl(port);
}

/*!
 * fs_orl() - or a long
 * @param port
 * @param val
 */
static void __inline__ fs_orl(u32 port, u32 val)
{
        u32 set = fs_rl(port);
        *(volatile u32 *)(IO_ADDRESS(port)) = (set | val);
}

/*!
 * fs_andl() - and a long
 * @param port
 * @param val
 */
static void __inline__ fs_andl(u32 port, u32 val)
{
        u32 set = fs_rl(port);
        *(volatile u32 *)(IO_ADDRESS(port)) = (set & val);
}

/*!
 * fs_wl_set() - set a word and verify
 * @param tag
 * @param port
 * @param val
 */
static void inline fs_wl_set(otg_tag_t tag, u32 port, u32 val)
{
        u32 set;
        *(volatile u32 *)(IO_ADDRESS(port)) = val;
#if 0
        set = fs_rl(port);
        if ((set & val) != val) {
                TRACE_MSG1(tag, "SET FAILED: %08x", set);
        }
#endif
}

#if 1
/*!
 * fs_wl_clr() - clr a word and verify
 * @param tag
 * @param port
 * @param clr
 */
static void inline fs_wl_clr(otg_tag_t tag, u32 port, u32 clr)
{
        u32 set;
        *(volatile u32 *)(IO_ADDRESS(port)) = clr;
        set = fs_rl(port);
        if (set & clr) {
                TRACE_MSG1(tag, "CLEAR FAILED 1: %08x", set);
                *(volatile u32 *)(IO_ADDRESS(port)) = clr;
                set = fs_rl(port);
#if 1
                if (set & clr)
                        TRACE_MSG1(tag, "CLEAR FAILED 2: %08x", set);
#endif
        }
}
#endif

/*!
 * fs_memcpy32() - emulate memcpy using long copy
 * @param dp destination pointer
 * @param sp source pointer
 * @param words number of 32bit words to copy
 *
 */
static void inline fs_memcpy32(u32 *dp, u32 *sp, volatile int words)
{
        while (words--) *dp++ = *sp++;
}
/*!
 * fs_memcpy() - emulate memcpy using byte copy
 * @param dp destination pointer
 * @param sp source pointer
 * @param bytes number of 8bit bytes to copy
 */
static void inline fs_memcpy(u8 *dp, u8 *sp, volatile int bytes)
{
        while (bytes--) *dp++ = *sp++;
}
/*!
 * fs_clear_words() - clear memory
 * @param addr address to clear from
 * @param words number of 32bit words to clear.
 */
static void inline fs_clear_words(volatile u32 *addr, int words)
{
        while (words--) *addr++ = 0;
}



/*! @} */

//#if defined(CONFIG_ARCH_SCMA11) || defined(CONFIG_ARCH_ARGONPLUS) || defined(CONFIG_ARCH_ARGONLV) || defined(CONFIG_ARCH_ZEUS)
#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321) || defined(CONFIG_ARCH_MXC91131)

#define _reg_CRM_AP_ASCSR       ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x00)))
#define _reg_CRM_AP_ACDR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x04)))
#define _reg_CRM_AP_ACDER1      ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x08)))
#define _reg_CRM_AP_ACDER2      ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x0c)))

#define _reg_CRM_AP_ACGCR       ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x10)))
#define _reg_CRM_AP_ACCGCR      ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x14)))
#define _reg_CRM_AP_AMLPMRA     ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x18)))
#define _reg_CRM_AP_AMLPMRB     ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x1c)))

#define _reg_CRM_AP_AMLPMRC     ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x20)))
#define _reg_CRM_AP_AMLPRMD     ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x24)))
#define _reg_CRM_AP_AMLPRME1    ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x28)))
#define _reg_CRM_AP_AMLPRME2    ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x2c)))

#define _reg_CRM_AP_AMLPMRF     ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x30)))
#define _reg_CRM_AP_AMLPMRG     ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x34)))
#define _reg_CRM_AP_APGCR       ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x38)))
#define _reg_CRM_AP_ACSR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x3c)))

#define _reg_CRM_AP_ADCR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x40)))
#define _reg_CRM_AP_ACR         ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x44)))
#define _reg_CRM_AP_AMCR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x48)))
#define _reg_CRM_AP_APCR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x4c)))

#define _reg_CRM_AP_AMORA       ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x50)))
#define _reg_CRM_AP_AMORB       ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x54)))
#define _reg_CRM_AP_AGPR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x58)))
#define _reg_CRM_AP_APRA        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x5c)))

#define _reg_CRM_AP_APRB        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x60)))
#define _reg_CRM_AP_APOR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x64)))


//#define _reg_CRM_AP_APOR        ((volatile __u32 *)(IO_ADDRESS(CRM_AP_BASE_ADDR + 0x64)))

#define BCTRL_BASE_ADDR         0xb4000000
#define _reg_BCTRL_VERSION      ((volatile __u16 *)(IO_ADDRESS(BCTRL_BASE_ADDR + 0x0)))
#define _reg_BCTRL_STATUS       ((volatile __u16 *)(IO_ADDRESS(BCTRL_BASE_ADDR + 0x2)))
#define _reg_BCTRL_1            ((volatile __u16 *)(IO_ADDRESS(BCTRL_BASE_ADDR + 0xa)))
#define _reg_BCTRL_2            ((volatile __u16 *)(IO_ADDRESS(BCTRL_BASE_ADDR + 0xa)))

#define BCTRL_2_USBSP           (1 << 9)
#define BCTRL_2_USBSD           (1 << 10)


#endif /* defined(CONFIG_MACH_SCMA11EVB) || defined(CONFIG_MACH_SCMA11BB) || defined(CONFIG_MACH_ARGONPLUSEVB) */
