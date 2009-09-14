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
 * otg/hardware/isp1301-procfs.c - USB Device Core Layer
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/isp1301/isp1301-procfs.c|20070612233038|36770
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *
 */
/*!
 * @file otg/hardware/isp1301-procfs.c
 * @brief Implement /proc/isp1301 to dump ISP1301 registers.
 *
 *
 * @ingroup ISP1301TCD
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX)

#include <otg/pcd-include.h>
#include "isp1301-hardware.h"
#include "isp1301.h"

#ifdef CONFIG_ARCH_MX2ADS
#include <asm/arch/mx2.h>
#define MX2_OTG_XCVR_DEVAD                      0x18
#define MX2_SEQ_OP_REG                          0x19
#define MX2_SEQ_RD_STARTAD                      0x1a
#define MX2_I2C_OP_CTRL_REG                     0x1b
#define MX2_SCLK_TO_SCL_HPER                    0x1e
#define MX2_I2C_INTERRUPT_AND_CTRL              0x1f

#define OTG_BASE_ADDR                           0x10024000
//#define OTG_I2C_BASE                            (OTG_BASE_ADDR+0x100)

#endif /* CONFIG_ARCH_MX2ADS */

#if defined(CONFIG_OTG_ISP1301_PROCFS) || defined(_OTG_DOXYGEN)
/* Proc Filesystem *************************************************************************** */

extern struct isp1301_private isp1301_private;

#define MAX_HISTORY     6
/*! @fn struct reg_list
 *  @brief - struct definition for register
 */
struct reg_list {
        u8 reg;
        u8 size;
        char *name;
        u32 values[MAX_HISTORY];
};

#define REG(r, s)  {r, s, #r, }

/*! @name register list tables
 *
 * @{
 */

struct reg_list isp1301_prod_list[] = {
        REG(ISP1301_VENDOR_ID, 2),
        REG(ISP1301_PRODUCT_ID, 2),
        REG(ISP1301_VERSION_ID, 2),
        { 0, 1, NULL,},
};
struct reg_list isp1301_reg_list[] = {
        REG(ISP1301_OTG_CONTROL_SET, 1),
        REG(ISP1301_INTERRUPT_SOURCE, 1),
        REG(ISP1301_INTERRUPT_LATCH_SET, 1),
        REG(ISP1301_INTERRUPT_ENABLE_LOW_SET, 1),
        REG(ISP1301_INTERRUPT_ENABLE_HIGH_SET, 1),
        REG(ISP1301_MODE_CONTROL_1_SET, 1),
        { 0, 1, NULL,},
};
struct reg_list isp1301_spec_list[] = {
        REG(ISP1301_MODE_CONTROL_2_SET, 1),
        REG(ISP1301_OTG_STATUS, 1),
        { 0, 1, NULL,},
};
struct reg_list max3301e_spec_list[] = {
        REG(MAX3301E_SPECIAL_FUNCTION_1_SET, 1),
        REG(MAX3301E_SPECIAL_FUNCTION_2_SET, 1),
        { 0, 1, NULL,},
};
#ifdef CONFIG_ARCH_MX2ADS
struct reg_list mx21_spec_list[] = {
        REG(MX2_OTG_XCVR_DEVAD, 1),
        REG(MX2_SEQ_OP_REG, 1),
        REG(MX2_SEQ_RD_STARTAD, 1),
        REG(MX2_I2C_OP_CTRL_REG, 1),
        REG(MX2_SCLK_TO_SCL_HPER, 1),
        REG(MX2_I2C_INTERRUPT_AND_CTRL, 1),
        { 0, 1, NULL,},
};
/* @} */
#endif /* CONFIG_ARCH_MX2ADS */
/*! isp1301_update -
 * @param list - isp1301 registers table
 */
void isp1301_update(struct reg_list *list)
{
        for (; list && list->name; list++) {
                //TRACE_MSG1(TCD, "list: %s", list->name);
                memmove(list->values + 1, list->values, sizeof(list->values) - sizeof(u32));
                switch(list->size) {
                case 1:
                        list->values[0] = i2c_readb(list->reg);
                        break;
                case 2:
                        list->values[0] = i2c_readw(list->reg);
                        break;
                case 4:
                        list->values[0] = i2c_readl(list->reg);
                        break;
                }
        }
}

#ifdef CONFIG_ARCH_MX2ADS
/*! mx2_rb - read a byte value from I2C device port
 * @param port - device port value
 * @return - read byte value
 */
static u8 __inline__ mx2_rb(u32 port)
{
        return *(volatile u8 *) (MX2_IO_ADDRESS(port + OTG_I2C_BASE));
}

/*! mx21_update - update an reg_list
 * @param list - pointer to reg_list to update
 */
void mx21_update(struct reg_list *list)
{
        for (; list && list->name; list++) {
                memmove(list->values + 1, list->values, sizeof(list->values) - sizeof(u32));
                list->values[0] = mx2_rb(list->reg);
        }
}

#endif /* CONFIG_ARCH_MX2ADS */
/*!
 * isp1301_update_all - update all reg_list
 */
void isp1301_update_all(void)
{
        isp1301_update(isp1301_reg_list);
        switch (isp1301_private.transceiver_map->transceiver_type) {
        case isp1301:
                isp1301_update(isp1301_spec_list);
                break;
        case max3301e:
                isp1301_update(max3301e_spec_list);
                break;
        default:
                break;
        }
#ifdef CONFIG_ARCH_MX2ADS
        mx21_update(mx21_spec_list);
#endif /* CONFIG_ARCH_MX2ADS */

}



/*!
 * dohexdigit - translate a value to hex notation
 * @param cp - buffer to save translated value
 * @param val - value to translate
 *
 */
static void dohexdigit (char *cp, unsigned char val)
{
        if (val < 0xa)
                *cp = val + '0';
        else if ((val >= 0x0a) && (val <= 0x0f))
                *cp = val - 0x0a + 'a';
}

/*!
 * dohexval - translate a value to hex notation
 * @param cp - buffer to save translated value
 * @param val - value to translate
 *
 */
static void dohexval (char *cp, unsigned char val)
{
        dohexdigit (cp++, val >> 4);
        dohexdigit (cp++, val & 0xf);
}

/*! isp_1301_dump - dump a reg_list instance information
 * @param buf - place to store information
 * @param name -
 * @param fmt
 * @param reg
 * @return
 */
int isp1301_dump(char *buf, char *name, char *fmt, struct reg_list *reg)
{
        int len = 0, i;
        len += sprintf (buf + len, "%-20s  %-34s [%03x]:  ", name, reg->name, reg->reg);
        len += sprintf (buf + len, fmt, reg->values[0]);
        for (i = 1; i < MAX_HISTORY; i++)
                if (reg->values[i - 1] == reg->values[i])
                        len += sprintf (buf + len, "         ");
                else
                        len += sprintf (buf + len, fmt, reg->values[i]);
        len += sprintf (buf + len, "\n");
        return len;
}
/*! isp1301_dump_list - dump reg_list table's inforamtion
 * @param buf -
 * @param name
 * @param list
 * @return information buffer length
 */
int isp1301_dump_list(char * buf, char *name, struct reg_list *list)
{
        int len = 0;
        for (; list && list->name; list++)
                switch(list->size) {
                case 1: len += isp1301_dump(buf + len, name, "       %02x", list); break;
                case 2: len += isp1301_dump(buf + len, name, "     %04x", list); break;
                case 4: len += isp1301_dump(buf + len, name, " %08x", list); break;
                }
        return len;
}

char *isp1301_otg_control[8] = {
        "DP_PULLUP",
        "DM_PULLUP",
        "DP_PULLDOWN",
        "DM_PULLDOWN",
        "ID_PULLDOWN",
        "VBUS_DRV",
        "VBUS_DISCHRG",
        "VBUS_CHRG",
};
char *isp1301_interrupt_source[8] = {
        "VBUS_VLD",
        "SESS_VLD",
        "DP_HI",
        "ID_GND",
        "DM_HI",
        "ID_FLOAT",
        "BDIS_ACON",
        "CR_INT",
};

char *isp1301_mode_control_1[8] = {
        "SPEED_REG",
        "SUSPEND_REG",
        "DAT_SE0",
        "TRANSP_EN",
        "BDIS_ACON_EN",
        "OE_INT_EN",
        "UART_EN",
        NULL,
};

char *isp1301_mode_control_2[8] = {
        "GLOBAL_PWR_ON",
        "SPD_SUSP_CTRL",
        "BI_DI",
        "TRANSP_BDIR_0",
        "TRANSP_BDIR_1",
        "AUDIO_EN",
        "PSW_OE",
        "EN2V7",
};

char *isp1301_otg_status[8] = {
        NULL, NULL, NULL, NULL, NULL, NULL,
        "B_SESS_END",
        "B_SESS_VLD",
};

/*! isp1301_detailed - dump detail information for reg port
 * @param buf
 * @param name
 * @param reg
 * @param detail
 * @return
 */
int isp1301_detailed(char *buf, char *name, u8 reg, char **detail)
{
        u8 val;
        int i;
        int len = 0;

        val = i2c_readb(reg);
        for (i = 7; i >= 0; i--) {

                if (3 == (i % 4))
                        len += sprintf (buf + len, "\n%-20s [%02d] ", name, reg);

                if (detail[i])
                        len += sprintf (buf + len, "%14s%s ", detail[i], (val & (1 << i)) ? " " : "/");
                else
                        len += sprintf (buf + len, "%14s%s ", "",  " ");
        }
        len += sprintf (buf + len, "\n", name);

        return len;
}
/*! isp1301_dump_all -
 * @param buf
 */
int isp1301_dump_all(char *buf)
{
        int len = 0;
        len += isp1301_dump_list(buf + len, "ISP1301 Standard", isp1301_reg_list);
        switch (isp1301_private.transceiver_map->transceiver_type) {
        case isp1301:
                len += isp1301_dump_list(buf + len, "ISP1301 Extra", isp1301_spec_list);
                break;
        case max3301e:
                len += isp1301_dump_list(buf + len, "MAX3301E Extra", max3301e_spec_list);
                break;
        default:
                break;
        }
#ifdef CONFIG_ARCH_MX2ADS
        len += isp1301_dump_list(buf + len, "MX21 ADS Extra", mx21_spec_list);
#endif /* CONFIG_ARCH_MX2ADS */
        return len;
}
/*! isp 1301_dump_detail
 * @param buf
 * @return buf length
 */
int isp1301_dump_detail(char *buf)
{
        int len = 0;

        len += isp1301_detailed(buf + len, "MODE CONTROL 1", ISP1301_MODE_CONTROL_1, isp1301_mode_control_1);
        len += isp1301_detailed(buf + len, "MODE CONTROL 2", ISP1301_MODE_CONTROL_2, isp1301_mode_control_2);
        len += isp1301_detailed(buf + len, "INTERRUPT ENABLE", ISP1301_INTERRUPT_ENABLE_HIGH, isp1301_interrupt_source);

        len += isp1301_detailed(buf + len, "OTG CONTROL", ISP1301_OTG_CONTROL_SET, isp1301_otg_control);
        len += isp1301_detailed(buf + len, "INTERRUPT SOURCE", ISP1301_INTERRUPT_SOURCE, isp1301_interrupt_source);
        len += isp1301_detailed(buf + len, "OTG STATUS", ISP1301_OTG_STATUS, isp1301_otg_status);
        return len;
}


/*!
 * isp1301_device_proc_read - implement proc file system read.
 *
 * Standard proc file system read function.
 *
 * We let upper layers iterate for us, *pos will indicate which device to return
 * statistics for.
 * @param file - file pointer
 * @param buf - buffer pointer
 * @param count -
 * @param pos -
 * @return read length
 */
static ssize_t isp1301_device_proc_read_functions (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        int index;
        int i;
        u32 r;

        int config_size;

        // get a page, max 4095 bytes of data...
        //RETURN_EINVAL_UNLESS ((page = get_free_page (GFP_KERNEL)));
        RETURN_EINVAL_UNLESS ((page = GET_KERNEL_PAGE()));

        len = 0;
        index = (*pos)++;

        //printk(KERN_INFO"%s: index: %d\n", __FUNCTION__, index);
        switch(index) {
        case 0:
                len += sprintf ((char *) page + len, "ISP1301 Transceiver Registers\n");

                TRACE_MSG0(REMOVE_TCD, "UPDATING");
                isp1301_update_all();
                TRACE_MSG0(REMOVE_TCD, "UPDATE FINISHED");
                len += sprintf ((char *) page + len , "Vendor: %04x Product: %04x Revision: %04x %s\n",
                                isp1301_private.vendor, isp1301_private.product,
                                isp1301_private.revision, isp1301_private.transceiver_map->name
                                );

                len += isp1301_dump_all((char *) page + len);
                len += sprintf ((char *) page + len, "\n");

                break;

        case 1:
                len += sprintf ((char *) page + len, "\n--\n");
                len += isp1301_dump_detail((char *) page + len);
                len += sprintf ((char *) page + len, "\n");
                break;

        default:
                break;

        }

        //printk(KERN_INFO"%s: len: %d\n", __FUNCTION__, len);

        if (len > count)
                len = -EINVAL;

        else if ((len > 0) && copy_to_user (buf, (char *) page, len))
                len = -EFAULT;

        //printk(KERN_INFO"%s: len: %d\n", __FUNCTION__, len);
        free_page (page);
        return len;
}

static struct file_operations isp1301_device_proc_operations_functions = {
        read:isp1301_device_proc_read_functions,
};

/* Module init ******************************************************************************* */
/*! isp1301_procfs_init - create a proc entry for isp1301 procfs
 */
int isp1301_procfs_init (void)
{
        struct proc_dir_entry *p;

        // create proc filesystem entries
        if ((p = create_proc_entry ("isp1301", 0, 0)) == NULL)
                return -ENOMEM;

        p->proc_fops = &isp1301_device_proc_operations_functions;

        isp1301_update_all();

        return 0;
}
void isp1301_procfs_exit (void)
{
        remove_proc_entry ("isp1301", NULL);
}
#else
int isp1301_procfs_init (void) { return 0;
}
void isp1301_procfs_exit (void) { }
#endif

#endif /* defined(CONFIG_OTG_LNX) */
