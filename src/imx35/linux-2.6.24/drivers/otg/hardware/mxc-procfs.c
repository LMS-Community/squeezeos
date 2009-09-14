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
 * otg/hardware/mxc-procfs.c - USB Device Core Layer
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/mxc/mxc-procfs.c|20070614183950|47700
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 */
/*!
 * @file otg/hardware/mxc-procfs.c
 * @brief FREESCAELE Procfs register dump
 *
 *
 * @ingroup FSOTG
 *
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

#include <otg/pcd-include.h>
#include <linux/module.h>
//#include <asm/arch/mx2.h>

//#include "mx2ads.h"
//#include <otghw/mx2ads-hardware.h>
#include "mxc-lnx.h"
#include "mxc-hardware.h"


/* ********************************************************************************************* */
/*! char *fs_hnpstate - hnp state array */

char * fs_hnpstate[32] = {
        "b_idle",               // 0
        "b_master",             // 1
        "b_slave",              // 2
        "b_srp_init_v",         // 3
        "b_srb_init_d",         // 4
        "b_short_db",           // 5
        "b_wait_conn_a",        // 6
        "b_wait_conn_b",        // 7
        "b_wait_bus_lo",        // 8
        "",                     // 9
        "b_wait_rst",           // a
        "",                     // b
        "",                     // c
        "",                     // d
        "",                     // e
        "",                     // f
        "a_idle",               // 0
        "a_master",             // 1
        "a_slave",              // 2
        "a_wait_vpluse",        // 3
        "a_wait_dpulse",        // 4
        "a_wait_conn_a",        // 5
        "",                     // 6
        "a_wait_conn_b",        // 7
        "a_wait_vrise",         // 8
        "a_suspend",            // 9
        "a_wait_vfall",         // a
        "a_vbus_err",           // b
        "conn_debounce",        // c
        "a_wait_abreq",         // d
        "a_wait_rst",           // e
        "",                     // f
};

/*! char *fs_core_regs - USBOTG Module Registers array */
char * fs_core_regs[] = {
                                                        // USBOTG Module Registers
        "OTG_CORE_HWMODE (0x000)",
        "OTG_CORE_CINT_STAT (0x004)" ,
        "OTG_CORE_CINT_STEN (0x008)" ,
        "OTG_CORE_CLK_CTRL (0x00c)",
        "OTG_CORE_RST_CTRL (0x010)",
        "OTG_CORE_FRM_INTVL (0x014)",
        "OTG_CORE_FRM_REMAIN (0x018)",
        "OTG_CORE_HNP_CSTAT (0x01c)",
        "OTG_CORE_HNP_TIMER1 (0x020)",
        "OTG_CORE_HNP_TIMER2 (0x024)",
        "OTG_CORE_HNP_T3PCR (0x028)",
        "OTG_CORE_HINT_STAT (0x02c)",
        "OTG_CORE_HINT_STEN (0x030)",
        "OTG_CORE_CPUEPSEL_STAT (0x034)",
        "UNUSED (0x038)",
        "OTG_CORE_INTERRUPT_STEN (0x03c)",

                                                        // USB Function
        "OTG_FUNC_CMD_STAT (0x040)", "OTG_FUNC_DEV_ADDR (0x044)", "OTG_FUNC_SINT_STAT (0x048)", "OTG_FUNC_SINT_STEN (0x04c)",
        "OTG_FUNC_XINT_STAT (0x050)", "OTG_FUNC_YINT_STAT (0x054)", "OTG_FUNC_XYINT_STEN (0x058)", "OTG_FUNC_XFILL_STAT (0x05c)",
        "OTG_FUNC_YFILL_STAT (0x060)", "OTG_FUNC_EP_EN (0x064)", "OTG_FUNC_EP_RDY (0x068)", "OTG_FUNC_IINT (0x06c)",
        "OTG_FUNC_EP_DSTAT (0x070)", "OTG_FUNC_EP_DEN (0x074)", "OTG_FUNC_EP_TOGGLE (0x078)", "OTG_FUNC_FRM_NUM (0x07c)",

                                                        // USB Host
        "OTG_HOST_CONTROL (0x080)", "OTG_HOST_SINT_STAT (0x084)", "OTG_HOST_SINT_STEN (0x088)", "HOST_UNUSED (0x08c)",
        "HOST_UNUSED (0x090)", "OTG_HOST_XINT_STAT (0x094)", "OTG_HOST_YINT_STAT (0x098)", "HOST_UNUSED (0x09c)",
        "OTG_HOST_XYINT_STEN (0x0a0)", "HOST_UNUSED (0x0a4)", "OTG_HOST_XFILL_STAT (0x0a8)", "OTG_HOST_YFILL_STAT (0x0ac)",
        "HOST_UNUSED (0x0b0)", "HOST_UNUSED (0x0b4)", "HOST_UNUSED (0x0b8)", "HOST_UNUSED (0x0bc)",

                                                        // USB Host
        "OTG_HOST_ETD_EN (0x0c0)", "HOST_UNUSED (0x0c4)", "OTG_HOST_DIR_ROUTE (0x0c8)", "OTG_HOST_IINT (0x0cc)",
        "OTG_HOST_EP_DSTAT (0x0d0)", "OTG_HOST_ETD_DONE (0x0d4)", "HOST_UNUSED (0x0d8)", "HOST_UNUSED (0x0dc)",
        "OTG_HOST_FRM_NUM (0x0e0)", "OTG_HOST_LSP_THRESH (0x0e4)", "OTG_HOST_HUB_DESCA (0x0e8)", "OTG_HOST_HUB_DESCB (0x0ec)",
        "OTG_HOST_HUB_STAT (0x0f0)", "OTG_HOST_PORT_STATUS_1 (0x0f4)", "OTG_HOST_PORT_STATUS_2 (0x0f8)", "OTG_HOST_PORT_STATUS_3 (0x0fc)",

};


#if 0
char *fs_i2c_regs1[] = {
        "VENDOR_ID_LOW (0x100)", "VENDOR_ID_HIGH (0x101)",
        "PRODUCT_ID_LOW (0x102)", "PRODUCT_ID_HIGH (0x103)",

        "MODE_CONTROL_1_SET (0x104)", "MODE_CONTROL_1_CLR (0x105)",
        "OTG_CONTROL_SET (0x106)", "OTG_CONTROL_CLR (0x107)",
        "INTERRUPT_SOURCE (0x108)", "RESERVED (0x109)",
        "INT_LAT_REG_SET (0x10a)", "INT_LAT_REG_CLR (0x10b)",
        "INT_FALSE_REG_SET (0x10c)", "INT_FALSE_REG_CLR (0x10d)",
        "INT_TRUE_REG_SET (0x10e)", "INT_TRUE_REG_CLR (0x10f)",
        "OTG_STATUS (0x110)", "UNUSED (0x111)",
        "MODE_CONTROL_2_SET (0x112)", "MODE_CONTROL_2_CLR (0x113)",

        "BCD_DEV_LOW (0x114)", "BCD_DEV_HIGH (0x115)",

        "RESERVED (0x116)", "RESERVED (0x117)",
        "OTG_XCVR_DEVAD (0x118)", "SEQ_OP_REG (0x119)",
        "SEQ_RD_STARTAD (0x11a)", "I2C_OP_CTRL_REG (0x11b)",
        "RESERVED (0x11c)", "RESERVED (0x11d)",
        "SCLK_TO_SCL_HPER (0x11e)", "I2C_INTERRUPT_AND_CTRL (0x11f)",
};

char *fs_i2c_regs2[] = {
        "VENDOR_ID", "PRODUCT_ID",

        "MODE_CONTROL_1_SETCLR", "OTG_CONTROL_SETCLR",
        "INTERRUPT_SOURCE", "INT_LAT_REG_SETCLR",
        "INT_FALSE_REG_SETCLR", "INT_TRUE_REG_SETCLR",
        "OTG_STATUS", "MODE_CONTROL_2_SETCLR",

        "BCD_DEV",
};

char *fs_i2c_regs3[] = {
        "OTG_XCVR_DEVAD",
        "SEQ_OP_REG",
        "SEQ_RD_STARTAD",
        "I2C_OP_CTRL_REG",
        "RESERVED",
        "RESERVED",
        "SCLK_TO_SCL_HPER",
        "I2C_INTERRUPT_AND_CTRL",
};
#endif

/*! char* fs_etd_momory - etd memory array */
char * fs_etd_memory[] = {
        // 0x200        ETD Memory Access
        "ETD 0 OUT WORD 0", "ETD 0 OUT WORD 1", "ETD 0 OUT WORD 2", "ETD 0 OUT WORD 3",
        "ETD 0 IN  WORD 0", "ETD 0 IN  WORD 1", "ETD 0 IN  WORD 2", "ETD 0 IN  WORD 3",
        "ETD 1 OUT WORD 0", "ETD 1 OUT WORD 1", "ETD 1 OUT WORD 2", "ETD 1 OUT WORD 3",
        "ETD 1 IN  WORD 0", "ETD 1 IN  WORD 1", "ETD 1 IN  WORD 2", "ETD 1 IN  WORD 3",
        "ETD 2 OUT WORD 0", "ETD 2 OUT WORD 1", "ETD 2 OUT WORD 2", "ETD 2 OUT WORD 3",
        "ETD 2 IN  WORD 0", "ETD 2 IN  WORD 1", "ETD 2 IN  WORD 2", "ETD 2 IN  WORD 3",
        "ETD 3 OUT WORD 0", "ETD 3 OUT WORD 1", "ETD 3 OUT WORD 2", "ETD 3 OUT WORD 3",
        "ETD 3 IN  WORD 0", "ETD 3 IN  WORD 1", "ETD 3 IN  WORD 2", "ETD 3 IN  WORD 3",
        "ETD 4 OUT WORD 0", "ETD 4 OUT WORD 1", "ETD 4 OUT WORD 2", "ETD 4 OUT WORD 3",
        "ETD 4 IN  WORD 0", "ETD 4 IN  WORD 1", "ETD 4 IN  WORD 2", "ETD 4 IN  WORD 3",
        "ETD 5 OUT WORD 0", "ETD 5 OUT WORD 1", "ETD 5 OUT WORD 2", "ETD 5 OUT WORD 3",
        "ETD 5 IN  WORD 0", "ETD 5 IN  WORD 1", "ETD 5 IN  WORD 2", "ETD 5 IN  WORD 3",
        "ETD 6 OUT WORD 0", "ETD 6 OUT WORD 1", "ETD 6 OUT WORD 2", "ETD 6 OUT WORD 3",
        "ETD 6 IN  WORD 0", "ETD 6 IN  WORD 1", "ETD 6 IN  WORD 2", "ETD 6 IN  WORD 3",
        "ETD 7 OUT WORD 0", "ETD 7 OUT WORD 1", "ETD 7 OUT WORD 2", "ETD 7 OUT WORD 3",
        "ETD 7 IN  WORD 0", "ETD 7 IN  WORD 1", "ETD 7 IN  WORD 2", "ETD 7 IN  WORD 3",
        "ETD 8 OUT WORD 0", "ETD 8 OUT WORD 1", "ETD 8 OUT WORD 2", "ETD 8 OUT WORD 3",
        "ETD 8 IN  WORD 0", "ETD 8 IN  WORD 1", "ETD 8 IN  WORD 2", "ETD 8 IN  WORD 3",
        "ETD 9 OUT WORD 0", "ETD 9 OUT WORD 1", "ETD 9 OUT WORD 2", "ETD 9 OUT WORD 3",
        "ETD 9 IN  WORD 0", "ETD 9 IN  WORD 1", "ETD 9 IN  WORD 2", "ETD 9 IN  WORD 3",
        "ETD A OUT WORD 0", "ETD A OUT WORD 1", "ETD A OUT WORD 2", "ETD A OUT WORD 3",
        "ETD A IN  WORD 0", "ETD A IN  WORD 1", "ETD A IN  WORD 2", "ETD A IN  WORD 3",
        "ETD B OUT WORD 0", "ETD B OUT WORD 1", "ETD B OUT WORD 2", "ETD B OUT WORD 3",
        "ETD B IN  WORD 0", "ETD B IN  WORD 1", "ETD B IN  WORD 2", "ETD B IN  WORD 3",
        "ETD C OUT WORD 0", "ETD C OUT WORD 1", "ETD C OUT WORD 2", "ETD C OUT WORD 3",
        "ETD C IN  WORD 0", "ETD C IN  WORD 1", "ETD C IN  WORD 2", "ETD C IN  WORD 3",
        "ETD D OUT WORD 0", "ETD D OUT WORD 1", "ETD D OUT WORD 2", "ETD D OUT WORD 3",
        "ETD D IN  WORD 0", "ETD D IN  WORD 1", "ETD D IN  WORD 2", "ETD D IN  WORD 3",
        "ETD E OUT WORD 0", "ETD E OUT WORD 1", "ETD E OUT WORD 2", "ETD E OUT WORD 3",
        "ETD E IN  WORD 0", "ETD E IN  WORD 1", "ETD E IN  WORD 2", "ETD E IN  WORD 3",
        "ETD F OUT WORD 0", "ETD F OUT WORD 1", "ETD F OUT WORD 2", "ETD F OUT WORD 3",
        "ETD F IN  WORD 0", "ETD F IN  WORD 1", "ETD F IN  WORD 2", "ETD F IN  WORD 3",
};

/*!  char * fs_ep_memory - endpoint memory array */
char * fs_ep_memory[] = {
        // 0x400        EP Memory Access

        "EP 0 OUT WORD 0", "EP 0 OUT WORD 1", "EP 0 OUT WORD 2", "EP 0 OUT WORD 3",
        "EP 0 IN  WORD 0", "EP 0 IN  WORD 1", "EP 0 IN  WORD 2", "EP 0 IN  WORD 3",
        "EP 1 OUT WORD 0", "EP 1 OUT WORD 1", "EP 1 OUT WORD 2", "EP 1 OUT WORD 3",
        "EP 1 IN  WORD 0", "EP 1 IN  WORD 1", "EP 1 IN  WORD 2", "EP 1 IN  WORD 3",
        "EP 2 OUT WORD 0", "EP 2 OUT WORD 1", "EP 2 OUT WORD 2", "EP 2 OUT WORD 3",
        "EP 2 IN  WORD 0", "EP 2 IN  WORD 1", "EP 2 IN  WORD 2", "EP 2 IN  WORD 3",
        "EP 3 OUT WORD 0", "EP 3 OUT WORD 1", "EP 3 OUT WORD 2", "EP 3 OUT WORD 3",
        "EP 3 IN  WORD 0", "EP 3 IN  WORD 1", "EP 3 IN  WORD 2", "EP 3 IN  WORD 3",
        "EP 4 OUT WORD 0", "EP 4 OUT WORD 1", "EP 4 OUT WORD 2", "EP 4 OUT WORD 3",
        "EP 4 IN  WORD 0", "EP 4 IN  WORD 1", "EP 4 IN  WORD 2", "EP 4 IN  WORD 3",
        "EP 5 OUT WORD 0", "EP 5 OUT WORD 1", "EP 5 OUT WORD 2", "EP 5 OUT WORD 3",
        "EP 5 IN  WORD 0", "EP 5 IN  WORD 1", "EP 5 IN  WORD 2", "EP 5 IN  WORD 3",
        "EP 6 OUT WORD 0", "EP 6 OUT WORD 1", "EP 6 OUT WORD 2", "EP 6 OUT WORD 3",
        "EP 6 IN  WORD 0", "EP 6 IN  WORD 1", "EP 6 IN  WORD 2", "EP 6 IN  WORD 3",
        "EP 7 OUT WORD 0", "EP 7 OUT WORD 1", "EP 7 OUT WORD 2", "EP 7 OUT WORD 3",
        "EP 7 IN  WORD 0", "EP 7 IN  WORD 1", "EP 7 IN  WORD 2", "EP 7 IN  WORD 3",
        "EP 8 OUT WORD 0", "EP 8 OUT WORD 1", "EP 8 OUT WORD 2", "EP 8 OUT WORD 3",
        "EP 8 IN  WORD 0", "EP 8 IN  WORD 1", "EP 8 IN  WORD 2", "EP 8 IN  WORD 3",
        "EP 9 OUT WORD 0", "EP 9 OUT WORD 1", "EP 9 OUT WORD 2", "EP 9 OUT WORD 3",
        "EP 9 IN  WORD 0", "EP 9 IN  WORD 1", "EP 9 IN  WORD 2", "EP 9 IN  WORD 3",
        "EP A OUT WORD 0", "EP A OUT WORD 1", "EP A OUT WORD 2", "EP A OUT WORD 3",
        "EP A IN  WORD 0", "EP A IN  WORD 1", "EP A IN  WORD 2", "EP A IN  WORD 3",
        "EP B OUT WORD 0", "EP B OUT WORD 1", "EP B OUT WORD 2", "EP B OUT WORD 3",
        "EP B IN  WORD 0", "EP B IN  WORD 1", "EP B IN  WORD 2", "EP B IN  WORD 3",
        "EP C OUT WORD 0", "EP C OUT WORD 1", "EP C OUT WORD 2", "EP C OUT WORD 3",
        "EP C IN  WORD 0", "EP C IN  WORD 1", "EP C IN  WORD 2", "EP C IN  WORD 3",
        "EP D OUT WORD 0", "EP D OUT WORD 1", "EP D OUT WORD 2", "EP D OUT WORD 3",
        "EP D IN  WORD 0", "EP D IN  WORD 1", "EP D IN  WORD 2", "EP D IN  WORD 3",
        "EP E OUT WORD 0", "EP E OUT WORD 1", "EP E OUT WORD 2", "EP E OUT WORD 3",
        "EP E IN  WORD 0", "EP E IN  WORD 1", "EP E IN  WORD 2", "EP E IN  WORD 3",
        "EP F OUT WORD 0", "EP F OUT WORD 1", "EP F OUT WORD 2", "EP F OUT WORD 3",
        "EP F IN  WORD 0", "EP F IN  WORD 1", "EP F IN  WORD 2", "EP F IN  WORD 3",
};

/*!  char * fs_control_regs - control registers array */
char * fs_control_regs[] = {
        // 0x600        USB Control Registers
        "OTG_SYS_CTRL",
};

/*!  char * fs_dma_regs - dma registers array */
char * fs_dma_regs[] = {
        // 0x800        DMA Registers
        "OTG_DMA_REV_NUM (0x800)", "OTG_DMA_DINT_STAT (0x804)", "OTG_DMA_DINT_STEN (0x808)", "OTG_DMA_ETD_ERR (0x80c)",
        "OTG_DMA_EP_ERR (0x810)", "DMA UNUSED (0x814)", "DMA UNUSED (0x818)", "DMA UNUSED (0x81c)",
        "OTG_DMA_ETD_EN (0x820)", "OTG_DMA_EP_EN (0x824)", "OTG_DMA_ETD_ENXREQ (0x828)", "OTG_DMA_EP_ENXREQ (0x82c)",
        "OTG_DMA_ETD_ENXYREQ (0x830)", "OTG_DMA_EP_ENXYREQ (0x834)", "OTG_DMA_ETD_BURST4 (0x838)", "OTG_DMA_EP_BURST4 (0x83c)",
        "OTG_DMA_MISC_CTRL (0x840)", "OTG_DMA_ETD_CH_CLR (0x844)", "OTG_DMA_EP_CH_CLR (0x848)", "DMA UNUSED (0x84c)",
};

char * fs_ep_msa[] = {
        "EP 0 OUT MSA", "EP 0 IN MSA",
        "EP 1 OUT MSA", "EP 1 IN MSA",
        "EP 2 OUT MSA", "EP 2 IN MSA",
        "EP 3 OUT MSA", "EP 3 IN MSA",
        "EP 4 OUT MSA", "EP 4 IN MSA",
        "EP 5 OUT MSA", "EP 5 IN MSA",
        "EP 6 OUT MSA", "EP 6 IN MSA",
        "EP 7 OUT MSA", "EP 7 IN MSA",
        "EP 8 OUT MSA", "EP 8 IN MSA",
        "EP 9 OUT MSA", "EP 9 IN MSA",
        "EP A OUT MSA", "EP A IN MSA",
        "EP B OUT MSA", "EP B IN MSA",
        "EP C OUT MSA", "EP C IN MSA",
        "EP D OUT MSA", "EP D IN MSA",
        "EP E OUT MSA", "EP E IN MSA",
        "EP F OUT MSA", "EP F IN MSA",
};

char *fs_ureg_name(u32 port)
{
        u32 low = port & 0xfff;

        //TRACE_MSG32("PORT: %x", (int)port);

        port = (port >> 2) & 0xfff;

        //TRACE_MSG32("PORT: %x", (int)port);

        if ((int)port < (0x100 >> 2))
                return (port < (sizeof(fs_core_regs)/4)) ? fs_core_regs[port] : "unknown";

        port -= 0x100 >> 2;

        if ((int)port < (0x100 >> 2)) {
                low &= 0x1f;
                #if 0
                if (low < 0x18) {
                        low >>= 2;
                        return (low < (sizeof(fs_i2c_regs1)/4)) ? fs_i2c_regs1[low] : "unknown";
                }
                low -= 0x18;
                return (low < sizeof(fs_i2c_regs3)/4) ? fs_i2c_regs3[low] : "unknown";
                return (low < sizeof(fs_i2c_regs1)/4) ? fs_i2c_regs1[low] : "unknown";
                #endif
                return "unknown";
        }

        port -= 0x100 >> 2;

        if ((int)port < (0x200 >> 2))
                return (port < (sizeof(fs_etd_memory)/4)) ? fs_etd_memory[port] : "unknown";

        port -= 0x200 >> 2;

        if ((int)port < (0x200 >> 2))
                return (port < (sizeof(fs_ep_memory)/4)) ? fs_ep_memory[port] : "unknown";

        port -= 0x200 >> 2;

        if ((int)port < (0x200 >> 2))
                return (port < (sizeof(fs_control_regs)/4)) ? fs_control_regs[port] : "unknown";

        port -= 0x200 >> 2;

        if ((int)port < (0x180>>2))
                return (port < (sizeof(fs_dma_regs)/4)) ? fs_dma_regs[port] : "unknown";

        port -= 0x180 >> 2;

        if ((int)port < (0x100>>2))
                return (port < (sizeof(fs_ep_msa)/4)) ? fs_ep_msa[port] : "unknown";

        return "unknown";

}


/* Proc Filesystem *************************************************************************** */



u8 udc_regs1[0x1000];
u8 udc_regs2[0x1000];

u32 zeros[100];



void fs_copy(u8 *dp)
{
        fs_memcpy32((u32 *) (dp + 0x000), (u32 *)IO_ADDRESS(OTG_CORE_BASE), 0x100/4);
        //fs_memcpy ((u8 *)  (dp + 0x100), (u8 *)IO_ADDRESS(OTG_I2C_BASE), 0x100);
        fs_memcpy32((u32 *) (dp + 0x200), (u32 *)IO_ADDRESS(OTG_ETD_BASE), 0xe00/4);
}
void fs_clear(volatile u32 *addr, int words)
{
        while (words--) *addr++ = 0;
}


/* *
 * dohex
 *
 */
static void dohexdigit (char *cp, unsigned char val)
{
        if (val < 0xa) {
                *cp = val + '0';
        } else if ((val >= 0x0a) && (val <= 0x0f)) {
                *cp = val - 0x0a + 'a';
        }
}

/* *
 * dohex
 *
 */
static void dohexval (char *cp, unsigned char val)
{
        dohexdigit (cp++, val >> 4);
        dohexdigit (cp++, val & 0xf);
}

int fs_regl(char *page, u32 reg)
{
        u32 offset = reg & 0xfff;
        u32 *p1 = (u32 *) (udc_regs1 + offset);
        u32 *p2 = (u32 *) (udc_regs2 + offset);

        if (*p1 == *p2)
                return sprintf (page, "  %-34s [%03x]: %08x\n", fs_ureg_name(reg), reg & 0xfff, *p2);
        else
                return sprintf (page, "  %-34s [%03x]: %08x (%08x)\n", fs_ureg_name(reg), reg & 0xfff, *p2, *p1);
}

int fs_regb(char *page, u32 reg)
{
        u32 offset = reg & 0xfff;
        u8 *p1 = (u8 *) (udc_regs1 + offset);
        u8 *p2 = (u8 *) (udc_regs2 + offset);

        if (*p1 == *p2)
                return sprintf (page, "  %-34s [%03x]: %02x\n", fs_ureg_name(reg), reg & 0xfff, *p2);
        else
                return sprintf (page, "  %-34s [%03x]: %02x (%02x)\n", fs_ureg_name(reg), reg & 0xfff, *p2, *p1);
}

int fs_regb2(char *page, u32 reg)
{
        u32 offset = reg & 0xfff;
        u8 *p1 = (u8 *) (udc_regs1 + offset);
        u8 *p2 = (u8 *) (udc_regs2 + offset);

        if (!memcmp(p1, p2, 2))
                return sprintf (page, "  %-34s [%03x]: %02x %02x\n",
                                fs_ureg_name(reg), reg & 0xfff, p2[1], p2[0]);
        else
                return sprintf (page, "  %-34s [%03x]: %02x %02x (%02x %02x)\n",
                                fs_ureg_name(reg+1), reg & 0xfff,
                                p2[1], p2[0], p1[1], p1[0]);
}

int fs_regb4(char *page, u32 reg)
{
        u32 offset = reg & 0xfff;
        u8 *p1 = (u8 *) (udc_regs1 + offset);
        u8 *p2 = (u8 *) (udc_regs2 + offset);

        if (!memcmp(p1, p2, 4))
                return sprintf (page, "  %-34s [%03x]: %02x %02x %02x %02x\n",
                                fs_ureg_name(reg), reg & 0xfff, p2[3], p2[2], p2[1], p2[0]);
        else
                return sprintf (page, "  %-34s [%03x]: %02x %02x %02x %02x (%02x %02x %02x %02x)\n",
                                fs_ureg_name(reg), reg & 0xfff,
                                p2[3], p2[2], p2[1], p2[0], p1[3], p1[2], p1[1], p1[0]);
}

int fs_etd(char *page, u32 reg)
{
        u32 offset = reg & 0xfff;
        u32 *p1 = (u32 *) (udc_regs1 + offset);
        u32 *p2 = (u32 *) (udc_regs2 + offset);

        if (!memcmp(p1, p2, 16)) {
                if ((reg == OTG_ETD_BASE) || memcmp(zeros, p2, 16))
                        return sprintf (page, "  %-34s [%03x]: %08x %08x %08x %08x\n",
                                        fs_ureg_name(reg), reg & 0xfff, p2[3], p2[2], p2[1], p2[0]);
                return 0;
        }
        else
                return sprintf (page, "  %-34s [%03x]: %08x %08x %08x %08x (%08x %08x %08x %08x)\n",
                                fs_ureg_name(reg), reg & 0xfff,
                                p2[3], p2[2], p2[1], p2[0], p1[3], p1[2], p1[1], p1[0]);
}

int fs_ep(char *page, u32 reg)
{
        u32 offset = reg & 0x7ff;
        u32 *p1 = (u32 *) (udc_regs1 + offset);
        u32 *p2 = (u32 *) (udc_regs2 + offset);

        if (!memcmp(p1, p2, 16)) {
                if (memcmp(zeros, p2, 16))
                        return sprintf (page, "  %-34s [%03x]: %08x %08x %08x %08x\n",
                                        fs_ureg_name(reg), reg & 0xfff, p2[0], p2[1], p2[2], p2[3]);
                return 0;
        }
        else
                return sprintf (page, "  %-34s [%03x]: %08x %08x %08x %08x (%08x %08x %08x %08x)\n",
                                fs_ureg_name(reg), reg & 0xfff,
                                p2[0], p2[1], p2[2], p2[3], p1[0], p1[1], p1[2], p1[3]);
}
/*! dump_mem - dump mem information
 * @param page - buffer to save information
 * @param dp - memory point to dump
 * @param count - message length to dump
 */
int dump_mem(char *page, u8 * dp, int count)
{
        return sprintf (page,
                        "[%08x] %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n",
                        dp,
                        dp[0], dp[1], dp[2], dp[3], dp[4], dp[5], dp[6], dp[7],
                        dp[8], dp[9], dp[10], dp[11], dp[12], dp[13], dp[14], dp[15]
                       );
}

/*! fs_cstat -
 * @param page
 */

static int fs_cstat(char *page)
{
        u32 hnp_cstat = fs_rl(OTG_CORE_HNP_CSTAT);
        u32 cint_stat = fs_rl(OTG_CORE_CINT_STAT);
        u32 hint_stat = fs_rl(OTG_CORE_HINT_STAT);
        int len = 0;

        //TRACE_MSG32("cint_stat: %08x", cint_stat);
        //TRACE_MSG32("hnp_cstat: %08x", hnp_cstat);
        //TRACE_MSG32("hint_stat: %08x", hint_stat);
        //
        len += sprintf(page + len, "\nHNP Status: ");

        if (hnp_cstat & MODULE_HNPDAT) len += sprintf(page + len, "HNPDAT ");
        if (hnp_cstat & MODULE_VBUSBSE) len += sprintf(page + len, "VBUSBSE ");
        if (hnp_cstat & MODULE_VBUSABSV) len += sprintf(page + len, "VBUSABSV ");
        if (hnp_cstat & MODULE_VBUSGTAVV) len += sprintf(page + len, "VBUSGTAVV ");

        if (hnp_cstat & MODULE_ARMTHNPE) len += sprintf(page + len, "ARMTHNPE ");
        if (hnp_cstat & MODULE_BHNPEN) len += sprintf(page + len, "BHNPEN ");

        if (hnp_cstat & MODULE_SLAVE) len += sprintf(page + len, "SLAVE ");
        if (hnp_cstat & MODULE_MASTER) len += sprintf(page + len, "MASTER ");
        if (hnp_cstat & MODULE_BGEN) len += sprintf(page + len, "BGEN ");
        if (hnp_cstat & MODULE_CMPEN) len += sprintf(page + len, "CMPEN ");
        if (hnp_cstat & MODULE_ISBDEV) len += sprintf(page + len, "ISBDEV ");
        if (hnp_cstat & MODULE_ISADEV) len += sprintf(page + len, "ISADEV");

        if (hnp_cstat & MODULE_SWVBUSPUL) len += sprintf(page + len, "SWVBUSPUL ");
        if (hnp_cstat & MODULE_SWAUTORST) len += sprintf(page + len, "SWAUTORS T");
        if (hnp_cstat & MODULE_SWPUDP) len += sprintf(page + len, "SWPUDP ");

        len += sprintf(page + len, "HNPSTAT: %02x ", (hnp_cstat >> 4) & 0x1f);

        if (hnp_cstat & MODULE_CLRERROR) len += sprintf(page + len, "CLRERROR ");
        if (hnp_cstat & MODULE_ADROPBUS) len += sprintf(page + len, "ADROPBUS ");
        if (hnp_cstat & MODULE_ABBUSREQ) len += sprintf(page + len, "ABBUSREQ ");

        len += sprintf(page + len, "\n");
        return len;
}

/* *
 * fs_device_proc_read - implement proc file system read.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Standard proc file system read function.
 *
 * We let upper layers iterate for us, *pos will indicate which device to return
 * statistics for.
 */
static ssize_t fs_device_proc_read_functions (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        int index;
        int i;
        u32 r;

        u8 config_descriptor[512];
        int config_size;

        //struct list_head *lhd;

        // get a page, max 4095 bytes of data...
        if (!(page = GET_KERNEL_PAGE())) {
                return -ENOMEM;
        }

        len = 0;
        index = (*pos)++;

        switch(index) {

        case 0:
                len += sprintf ((char *) page + len, "MX21 OTG Dump\n");
                fs_copy(udc_regs2);

                r = udc_regs2[0];
                switch (r & MODULE_CRECFG) {
                case 0: len += sprintf ((char *) page + len, "Hardware HNP\n"); break;
                case 1: len += sprintf ((char *) page + len, "Host Only\n"); break;
                case 2: len += sprintf ((char *) page + len, "Function Only\n"); break;
                case 3: len += sprintf ((char *) page + len, "Software HNP\n"); break;
                }
                len += sprintf ((char *) page + len, "OTG Transceiver: ");
                switch (((r & MODULE_OTGXCVR) >> 6) & MODULE_CRECFG) {
                case 0: len += sprintf ((char *) page + len, "Differential / Differential\n"); break;
                case 2: len += sprintf ((char *) page + len, "Single-Ended / Differential\n"); break;
                case 1: len += sprintf ((char *) page + len, "Differential / Single-Ended\n"); break;
                case 3: len += sprintf ((char *) page + len, "Single-Ended / Single-Ended\n"); break;
                }
                break;

        case 1:
                len += sprintf ((char *) page + len, "\nUSB Control\n");
                len += fs_regl((char *) page + len, OTG_SYS_CTRL);
                break;

        case 2:
                len += sprintf ((char *) page + len, "\nUSB Core\n");
                for (i = OTG_CORE_HWMODE; i <= OTG_CORE_INTERRUPT_STEN; i += 4)
                        len += fs_regl((char *) page + len, i);
                len += fs_cstat((char *)page + len);
                break;

        case 3:
                len += sprintf ((char *) page + len, "\nUSB Host\n");
                for (i = OTG_HOST_CONTROL; i <= OTG_HOST_PORT_STATUS_3; i += 4)
                        len += fs_regl((char *) page + len, i);
                break;

        case 4:
                len += sprintf ((char *) page + len, "\nUSB ETD\n");
                for (i = OTG_ETD_BASE; i < OTG_EP_BASE; i += 16)
                        len += fs_etd((char *) page + len, i);
                break;

        case 5:
                len += sprintf ((char *) page + len, "\nUSB Func\n");
                for (i = OTG_FUNC_CMD_STAT; i <= OTG_FUNC_FRM_NUM; i += 4)
                        len += fs_regl((char *) page + len, i);
                break;

        case 6:
                len += sprintf ((char *) page + len, "\nUSB EP\n");
                //for (i = OTG_EP_BASE; i < OTG_SYS_BASE; i += 16)
                //        len += fs_ep((char *) page + len, i);
                for (i = OTG_EP_BASE; i < 8; i++)
                        len += fs_ep((char *) page + len, i*16);
                break;

        case 7:
                len += sprintf ((char *) page + len, "\nUSB DMA\n");
                for (i = OTG_DMA_REV_NUM; i <= OTG_DMA_EP_CH_CLR; i += 4)
                        len += fs_regl((char *) page + len, i);
                break;
        case 8:
                len += sprintf ((char *) page + len, "\nUSB DMA MSA\n");
                for (i = OTG_DMA_EPN_MSA(0); i <= OTG_DMA_EPN_MSA(32); i += 4)
                        len += fs_regl((char *) page + len, i);
                break;
        case 9:
                len += sprintf ((char *) page + len, "\nUSB Bufs\n");
                break;

        case 10:
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_OUT), 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_OUT)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_OUT)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_OUT)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_OUT), 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_OUT)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_OUT)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_OUT)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                break;

        case 11:
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_IN), 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_IN)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_IN)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(0, USB_DIR_IN)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_IN), 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_IN)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_IN)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(0, USB_DIR_IN)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                break;

        case 12:
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_OUT), 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_OUT)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_OUT)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_OUT)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_OUT), 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_OUT)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_OUT)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_OUT)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                break;

        case 13:
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_IN), 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_IN)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_IN)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_x_address(1, USB_DIR_IN)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_IN), 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_IN)+16, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_IN)+32, 16);
                len += dump_mem((char *) page + len, (u8 *)data_y_address(1, USB_DIR_IN)+48, 16);
                len += sprintf ((char *) page + len, "\n");
                break;
                #if 0
        case 9:
                len += sprintf ((char *) page + len, "\nUSB OTG\n");
                for (i = OTG_I2C_BASE; i < MX2_OTG_XCVR_DEVAD; i += 1)
                        len += fs_regb((char *) page + len, i);
                break;
        case 10:
                len += sprintf ((char *) page + len, "\nUSB I2C Control\n");
                for (i = MX2_OTG_XCVR_DEVAD; i <= MX2_I2C_INTERRUPT_AND_CTRL; i += 1)
                        len += fs_regb((char *) page + len, i);
                break;
                #endif





        default:
                memcpy(udc_regs1, udc_regs2, sizeof(udc_regs1));
                break;
        }

        //printk(KERN_INFO"%s: len: %d count: %d\n", __FUNCTION__, len, count);

        if (len > count) {
                //printk(KERN_INFO"%s: len > count\n", __FUNCTION__);
                //printk(KERN_INFO"%s", page);
                len = -EINVAL;
        }
        else if ((len > 0) && copy_to_user (buf, (char *) page, len)) {
                //printk(KERN_INFO"%s: EFAULT\n", __FUNCTION__);
                len = -EFAULT;
        }
        else {
                //printk(KERN_INFO"%s: OK\n", __FUNCTION__);
        }
        free_page (page);
        return len;
}

/*!  struct file_operations fs_device_proc_operations_functions */
static struct file_operations fs_device_proc_operations_functions = {
        read:fs_device_proc_read_functions,
};




/* Module init ******************************************************************************* */


int mxc_procfs_init (void)
{
        {
                struct proc_dir_entry *p;

                // create proc filesystem entries
                if ((p = create_proc_entry ("mxclib", 0, 0)) == NULL)
                        return -ENOMEM;
                p->proc_fops = &fs_device_proc_operations_functions;
        }

        //printk(KERN_INFO"%s: %08x", __FUNCTION__, IO_ADDRESS(OTG_ETD_BASE));
        //fs_clear((volatile u32 *)IO_ADDRESS(OTG_ETD_BASE), 0x200);
        //fs_clear((volatile u32 *)IO_ADDRESS(OTG_EP_BASE), 0x200);
        fs_copy(udc_regs1);
        memset(udc_regs1 + 0x18, 0, 4);

        return 0;
}

void mxc_procfs_exit (void)
{
        // remove proc filesystem entry
        remove_proc_entry ("mxclib", NULL);
}


#endif /* defined(CONFIG_OTG_LNX) */
