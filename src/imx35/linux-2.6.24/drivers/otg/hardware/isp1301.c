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
 * otg/hardware/isp1301.c -- USB Transceiver Controller driver
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/platform/isp1301/isp1301.c|20070612233038|28967
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *      Copyright (c) 2005-2007 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/isp1301.c
 * @brief ISP1301 OTG Transceiver Driver.
 *
 * This is the generic ISP1301 TCD core support.
 *
 * Notes:
 *
 * 1. The ISP1301 can control the speed and suspend directly, would it be
 * appropriate to allow state machine to control this.
 *
 * 2. The ISP1301 has auto connect feature, can this be used without change
 * to state machine.
 *
 * 3. The ISP1301 can control the ADR/PSW pin to enable / disable external
 * charge pump.
 *
 * @ingroup ISP1301TCD
 */

#include <otg/otg-compat.h>

#if defined(CONFIG_OTG_ISP1301) || defined(_OTG_DOXYGEN)

//#include <asm/irq.h>
//#include <asm/system.h>
//#include <asm/io.h>
//#include <linux/i2c.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
//#include <otg/otg-task.h>
#include <otg/otg-hcd.h>
#include <otg/otg-tcd.h>
#include <otg/otg-ocd.h>
#include <otg/otg-pcd.h>

//#include <linux/i2c.h>

#include "isp1301-hardware.h"
#include "isp1301.h"


//#ifdef CONFIG_OMAP_H2
//#include <asm/arch/gpio.h>
//#include <asm/arch/mux.h>
//#endif /* CONFIG_OMAP_H2 */

/*! @var struct otg_transceiver_map isp1301_transceiver_map
 */
struct otg_transceiver_map isp1301_transceiver_map[] = {
        { isp1301, 0x4cc, 0x1301, 0x00, "Phillips ISP1301", },
        { max3301e, 0x6a0b, 0x0133, 0x00, "Maxim MAX3301E", },
        { 0, 0, 0, 0, "Unknown Transceiver", },
};

struct isp1301_private isp1301_private;
struct i2c_client *omap_i2c_client;
//static struct file *i2c_file;
#define MAX_I2C 16



/* ********************************************************************************************* */
#define TRACE_I2CB(t,r) TRACE_MSG3(t, "%-40s[%02x] %02x", #r, r, i2c_readb(r))
#define TRACE_I2CW(t,r) TRACE_MSG3(t, "%-40s[%02x] %04x", #r, r, i2c_readw(r))
#define TRACE_GPIO(t,b,r) TRACE_MSG2(t, "%-40s %04x", #r, readw(b + r))
#define TRACE_REGL(t,r) TRACE_MSG2(t, "%-40s %08x", #r, readl(r))



/*! isp1301_int_src - update interrupt source test mask
 *
 * This sets the current mask and updates the interrupt registers to match.
 * @param otg - otg_instance pointer
 * @param int_src -
 */
void isp1301_int_src(struct otg_instance *otg, u8 int_src)
{
        TRACE_MSG1(otg->tcd->TAG, "setting int_src %02x", int_src);
        isp1301_private.int_src = int_src;
        i2c_writeb(ISP1301_INTERRUPT_ENABLE_LOW_CLR, ~int_src);
        i2c_writeb(ISP1301_INTERRUPT_ENABLE_HIGH_CLR, ~int_src);
        i2c_writeb(ISP1301_INTERRUPT_ENABLE_LOW_SET, int_src);
        i2c_writeb(ISP1301_INTERRUPT_ENABLE_HIGH_SET, int_src);
        i2c_writeb(ISP1301_INTERRUPT_LATCH_CLR, int_src);
}

/*! isp1301_int_src_set - add to the interrupt source mask
 * @param otg - otg_instance pointer
 * @param int_src - interrupt maks to add
 */
void isp1301_int_src_set(struct otg_instance *otg, u8 int_src)
{
        isp1301_int_src(otg, int_src |= isp1301_private.int_src);
}

/*! isp1301_int_src_clr - remove from the interrup source mask
 * @param otg - otg_instance pointer
 * @param int_src - interrupt mask to remove
 */
void isp1301_int_src_clr(struct otg_instance *otg, u8 int_src)
{
        isp1301_int_src(otg, int_src = isp1301_private.int_src & ~int_src);
}

/* ********************************************************************************************* */

/*!create a type for struct  struct isp1301_test
 *  @brief typedef struct isp1301_test isp1301_test_t
*/
typedef struct isp1301_test {
        u32     input;
        char   *name;
        u8      mask;
} isp1301_test_t;

#define OV(i, m) {i, #i, m}

/*! struct isp1301_test isp1301_int_src_tests */

struct isp1301_test isp1301_int_src_tests[9] = {

        OV(VBUS_VLD, ISP1301_VBUS_VLD),
        OV(A_SESS_VLD, ISP1301_SESS_VLD),
        OV(DP_HIGH, ISP1301_DP_HI),
        OV(ID_GND, ISP1301_ID_GND),
        OV(DM_HIGH, ISP1301_DM_HI),
        OV(ID_FLOAT, ISP1301_ID_FLOAT),
        OV(BDIS_ACON, ISP1301_BDIS_ACON),
        OV(CR_INT_DET, ISP1301_CR_INT),
        {0, NULL, 0},
};

/*! struct isp1301_test isp1301_otg_status-tests */
struct isp1301_test isp1301_otg_status_tests[3] = {

        OV(B_SESS_END, ISP1301_B_SESS_END),
        OV(B_SESS_VLD, ISP1301_B_SESS_VLD),
        {0, NULL, 0},
};

/*! isp1301_event() - set input bit based on current settings and interrupt mask
 * @param otg - otg_instance pointer
 * @param tests -
 * @param val
 * @param testmask
 * @param msg
 * @return inputs mask value
 */
otg_current_t isp1301_event(struct otg_instance *otg, struct isp1301_test *tests, int val, int testmask, char *msg)
{
        int i;
        otg_current_t inputs = 0;
        TRACE_MSG3(otg->tcd->TAG, "val: %02x mask: %02x %s", val, testmask, msg);
        for (i = 0; tests[i].mask; i++) {
                otg_current_t input = tests[i].input;
                u8 mask = tests[i].mask;
                CONTINUE_UNLESS(mask & testmask);
                inputs |= (val & mask) ? input : _NOT(input);
                TRACE_MSG6(otg->tcd->TAG, "%s%s %02x %08x inputs: %08x %08x",
                                tests[i].name, (val & mask) ? "" : "/", tests[i].mask, tests[i].input,
                                (u32) (inputs >> 32), (u32)(inputs & 0xffffffff)
                                );
        }
        return inputs;
}



int isp1301_bh_first;
int isp1301_debounce;
otg_tick_t isp1301_ticks;

void isp1301_info(char *msg, u8 value)
{
        #if 0
        otg_tick_t new_ticks = otg_tmr_ticks();
        otg_tick_t ticks = otg_tmr_elapsed(&new_ticks, &isp1301_ticks);

        if (ticks < 10000)
                TRACE_MSG3(PCD, "ISP1301 %d uS %s: %02x", (u32)(ticks & 0xffffffff), msg, value);
        else if (ticks < 10000000)
                TRACE_MSG3(PCD, "ISP1301 %d mS %s: %02x", (u32)((ticks >> 10) & 0xffffffff), msg, value);
        else
                TRACE_MSG3(PCD, "ISP1301 %d S  %s: %02x", (u32)((ticks >> 20) & 0xffffffff), msg, value);

        isp1301_ticks = new_ticks;
        #endif
}


void isp1301_trace(u8 int_src, u8 int_changed, char *msg)
{

        #if 0
        otg_tick_t new_ticks = otg_tmr_ticks();
        otg_tick_t ticks = otg_tmr_elapsed(&new_ticks, &isp1301_ticks);

        if (ticks < 10000)
                TRACE_MSG4(PCD, "ISP1301 %d uS %s: src: %02x chng: %02x",
                                (u32)(ticks & 0xffffffff), msg, int_src, int_changed);
        else if (ticks < 10000000)
                TRACE_MSG4(PCD, "ISP1301 %d mS %s src: %02x chng: %02x",
                                (u32)((ticks >> 10) & 0xffffffff), msg, int_src, int_changed);
        else
                TRACE_MSG4(PCD, "ISP1301 %d S  %s src: %02x chng: %02x ",
                                (u32)((ticks >> 20) & 0xffffffff), msg, int_src, int_changed);

        isp1301_ticks = new_ticks;
        #endif
}

#if 0
/*! isp1301_bh
 */
void *isp1301_bh(void *data)
{
        struct otg_instance *otg = (struct otg_instance *) data;

        u8 otg_status = 0, special_function_22 = 0;
        u8 int_lat, int_src1 = 0, int_src_prev = 0;
        #if defined(CONFIG_MX2_TO11)
        u8 int_src2 = 0;
        #endif /* defined(CONFIG_MX2_TO11) */
        static u8 otg_status_saved, special_function_22_saved;
        static u8 int_src_saved;

        int count = 0;

        printk(KERN_INFO"%s:\n", __FUNCTION__);
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG0(otg->tcd->TAG, "--");

        /*if isp1301_private is NOT filled before then return */
        RETURN_NULL_UNLESS (isp1301_private.ready);

        do {
                otg_current_t inputs = 0;

                printk(KERN_INFO"%s: loop\n", __FUNCTION__);

                /* read the interrupt latch and and clear latch and update otg
                 */
                if ((int_lat = i2c_readb(ISP1301_INTERRUPT_LATCH_SET))) {
                        //TRACE_MSG1(TCD, "CLEARING INT LAT: %02x", int_lat);
                        isp1301_info("INT LAT", int_lat);
                        i2c_writeb(ISP1301_INTERRUPT_LATCH_CLR, int_lat);
                }

                isp1301_info("INT_LAT", int_lat);

                /* If int_lat shows a change or we are forcing an update, read interrupt source.
                 * Note that we mask DP_HI and DM_HI when LOC_CONN is set, we don't want them
                 * when USB Bus is in use.
                 *
                 * Mask with bits we are currently interested in.
                 *
                 * Ensure that we have two matching reads to ensure that value is stable.
                 * Also ensure that interrupt source is non-zero.
                 */

                int_src1 = i2c_readb(ISP1301_INTERRUPT_SOURCE);

                printk(KERN_INFO"%s: int_src: %02x %02x", __FUNCTION__, int_src_saved, int_src1);

                BREAK_IF(int_src1 == int_src_prev);
                int_src_prev = int_src1;

                isp1301_trace(int_src1, int_src_saved ^ int_src1, "INT_SRC_1");

                #if defined(CONFIG_MX2_TO11)
                do {
                        int_src2 = i2c_readb(ISP1301_INTERRUPT_SOURCE);         // re-read interrupt source
                        BREAK_IF(int_src1 && (int_src1 & (ISP1301_ID_GND | ISP1301_ID_FLOAT)) &&
                                        (int_src1 == int_src2));           // finished if non-zero and same
                        isp1301_trace(int_src2, int_src_saved ^ int_src2, "INT_SRC_2");
                        int_src1 = int_src2;                                    // save most recent read value
                } while (1);
                #endif /* defined(CONFIG_MX2_TO11) */



                #if 0
                while ( (int_src1 != (int_src2 = i2c_readb(ISP1301_INTERRUPT_SOURCE) & isp1301_private.int_src))) {
                        isp1301_trace(int_src2, int_src_saved ^ int_src2, "INT_SRC_2");
                        int_src1 = int_src2;
                }
                #endif

                if (isp1301_bh_first || int_src_saved ^ (int_src1 & isp1301_private.int_src)) {
                        int_src_saved = int_src1;
                        inputs |= isp1301_event(otg, isp1301_int_src_tests, int_src1,
                                        isp1301_private.int_src, "ISP1301 INT SRC");
                        if (int_src1 & ISP1301_VBUS_VLD) {
                                TRACE_MSG0(otg->tcd->TAG, "VBUS_VLD, setting B_SESS_VLD");
                                inputs |= B_SESS_VLD;
                        }
                        else if (!(int_src1 & ISP1301_SESS_VLD)) {
                                TRACE_MSG0(otg->tcd->TAG, "A_SESS_VLD/, setting B_SESS_VLD/");
                                inputs |= B_SESS_VLD_;
                        }
                }

                #if 1
                /* further work if B-Device
                 */
                if ( !(int_src1 & ISP1301_ID_GND) || !(int_src_saved & ISP1301_ID_GND) ) {
                        /* read the otg_status register and update otg state machine
                         *
                         * XXX this needs to be conditional on Transceiver type. The
                         * MAX3301E for example supports sess_end in a separate register.
                         */
                        switch (isp1301_private.transceiver_map->transceiver_type) {
                        case isp1301:
                                otg_status = i2c_readb(ISP1301_OTG_STATUS);
                                isp1301_trace(otg_status, int_src_saved ^ int_src1, "OTG_STATUS");
                                TRACE_MSG3(otg->tcd->TAG, "otg_status_saved: %02x otg_status: %02x chng: %02x",
                                                otg_status_saved, otg_status, otg_status_saved ^ otg_status);

                                if (isp1301_bh_first || (otg_status_saved ^ otg_status)) {
                                        inputs |= isp1301_event(otg, isp1301_otg_status_tests, otg_status, 0xff,
                                                        "ISP1301 OTG STATUS");
                                        otg_status_saved = otg_status;
                                }

                                #if 0
                                isp1301_info("OTG STATUS", otg_status);
                                if (isp1301_bh_first)
                                        otg_status_saved = ~otg_status;

                                otg_status_saved = isp1301_update_otg_status( otg, otg_status,
                                                otg_status_saved ^ otg_status);
                                #endif
                                break;

                                // XXX
                        case max3301e:
                                special_function_22 = i2c_readb(MAX3301E_SPECIAL_FUNCTION_2_SET);
                                break;
                        case unknown_transceiver:
                                break;
                        }
                }
                #endif
                if (inputs) {
                        u32 reset = (u32)(inputs >> 32);
                        u32 set = (u32) (inputs & 0xffffffff);
                        TRACE_MSG3(otg->tcd->TAG, "ISP1301: RESET: %08x SET: %08x %s", reset, set, "QUEUE");
                        otg_event(otg, inputs, otg->tcd->TAG, "ISP1301");
                }
                isp1301_bh_first = 0;

        } while ((int_lat & isp1301_private.int_src) && (count++ < 6));
        TRACE_MSG1(otg->tcd->TAG, "count: %d", count);

        otg->tcd->vbus = BOOLEAN(int_src_saved & ISP1301_VBUS_VLD);
        otg->tcd->id = BOOLEAN(int_src_saved & ISP1301_ID_GND);
        printk(KERN_INFO"%s: finis\n", __FUNCTION__);
        return NULL;
}

#else
/*! isp1301_bh -
 * @param data - otg_instance pointer
 */

void *isp1301_bh(void *data)
{
        u8 int_lat, int_src1 = 0, otg_status = 0;
        otg_current_t inputs = 0;

        struct otg_instance *otg = (struct otg_instance *) data;

	TRACE_MSG0(otg->tcd->TAG, "--");

        int_lat = i2c_readb(ISP1301_INTERRUPT_LATCH_SET);
        i2c_writeb(ISP1301_INTERRUPT_LATCH_CLR, int_lat);

        int_src1 = i2c_readb(ISP1301_INTERRUPT_SOURCE);
//	printk(KERN_INFO"%s: interrupt source: %x latch: %x\n", __FUNCTION__, int_src1, int_lat);
        inputs |= isp1301_event(otg, isp1301_int_src_tests, int_src1,
                        isp1301_private.int_src, "ISP1301 INT SRC");
        if (int_src1 & ISP1301_VBUS_VLD) {
                TRACE_MSG0(otg->tcd->TAG, "VBUS_VLD, setting B_SESS_VLD");
                inputs |= B_SESS_VLD;
        }
        else if (!(int_src1 & ISP1301_SESS_VLD)) {
                TRACE_MSG0(otg->tcd->TAG, "A_SESS_VLD/, setting B_SESS_VLD/");
                inputs |= B_SESS_VLD_;
        }

	//Extra work for otg
	otg_status = i2c_readb(ISP1301_OTG_STATUS);
        inputs |= isp1301_event(otg, isp1301_otg_status_tests, otg_status, 0xff, "ISP1301 OTG STATUS");
	//End of extra work for otg


        if (inputs) {
                //u32 reset = (u32)(inputs >> 32);
                //u32 set = (u32) (inputs & 0xffffffff);
		TRACE_MSG2(otg->tcd->TAG, "inputs: %x otg->task: %x\n", inputs, otg->task);
                if (otg->task)
                        otg_event(otg, inputs, otg->tcd->TAG, "ISP1301");
        }
        otg->tcd->vbus = BOOLEAN(int_src1 & ISP1301_VBUS_VLD);
        otg->tcd->id = BOOLEAN(int_src1 & ISP1301_ID_GND);
        return NULL;
}
#endif

/*! isp1301_work - process isp1301 task work
 * @param data - otg_instance pointer
 */
void *isp1301_work(otg_task_arg_t data)
{
        struct otg_instance *otg = (struct otg_instance *) data;
        TRACE_MSG0(otg->tcd->TAG, "ISP1301 WORK AWAKE");
        return isp1301_private.work_proc(data);
}

/*! isp1301_bh_wakeup - wakeup the isp1301 bottom half
 * @param otg - otg_instance pointer
 * @param  first
 */
void isp1301_bh_wakeup(struct otg_instance *otg, int first)
{
        TRACE_MSG1(otg->tcd->TAG, "ISP1301 WAKEUP%s", first ? " FIRST" : "");
        isp1301_bh_first |= first;
	otg_up_work(isp1301_private.task);
}


/* ********************************************************************************************** */
#if 0
/*! isp1301_vbus - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 */
int isp1301_vbus (struct otg_instance *otg)
{
        return isp1301_private.int_src & ISP1301_VBUS_VLD ? 1 : 0;
}

/*! isp1301_id - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 */
int isp1301_id (struct otg_instance *otg)
{
        return isp1301_private.int_src & ISP1301_ID_GND ? 1 : 0;
}
#endif

/* ********************************************************************************************* */
/*! isp1301_tcd_en() - used to enable
 * @param otg - otg_instance pointer
 * @param flag -
 *
 */
void isp1301_tcd_init(struct otg_instance *otg, u8 flag)
{
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "SET");
                break;
        case PULSE:
                TRACE_MSG0(otg->tcd->TAG, "PULSE");
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "RESET");
                break;
        }
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        isp1301_bh_wakeup(otg, TRUE);
        otg_event(otg, OCD_OK, otg->tcd->TAG, "ISP1301 OK");
}

/*! isp1301_tcd_en() - used to enable
 * @param otg - otg_instance pointer
 * @param flag -
 *
 */
void isp1301_tcd_en(struct otg_instance *otg, u8 flag)
{
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "SET");
                break;
        case PULSE:
                TRACE_MSG0(otg->tcd->TAG, "PULSE");
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "RESET");
                break;
        }
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        isp1301_bh_wakeup(otg, TRUE);
}


/*! isp1301_chrg_vbus - used to enable or disable B-device Vbus charging
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_chrg_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "CHRG_VBUS_SET");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_VBUS_CHRG);
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "CHRG_VBUS_RESET");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_VBUS_RESET);
                break;
        case PULSE:
                break;
        }
}

/*! isp1301_drv_vbus - used to enable or disable A-device driving Vbus
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_drv_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "DRV_VBUS_SET");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_VBUS_DRV);
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "DRV_VBUS_RESET");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_VBUS_RESET);
                break;
        }
}

/*! isp1301_dischrg_vbus - used to enable Vbus discharge
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dischrg_vbus(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "OUTPUT: TCD_DISCHRG_VBUS_SET");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_VBUS_DISCHRG);
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "OUTPUT: TCD_DISCHRG_VBUS_RESET");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_VBUS_RESET);
                break;
        }
}

/*! isp1301_mx21_vbus_drain - used to enable Vbus discharge
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "OUTPUT: TCD_DISCHRG_VBUS_SET");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_VBUS_DISCHRG);
                break;
        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "OUTPUT: TCD_DISCHRG_VBUS_RESET");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_VBUS_RESET);
                break;
        }
}

/*! isp1301_dp_pullup_func - used to enable or disable peripheral connecting to bus
 *
 * C.f. 5.1.6, 5.1.7, 5.2.4 and 5.2.5
 *
 *                              host    peripheral
 *              d+ pull-up      clr     set
 *              d+ pull-down    set     clr
 *
 *              d- pull-up      clr     clr
 *              d- pull-down    set     set
 *
 */
/*! isp1301_dp_pullup_func
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dp_pullup_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                isp1301_private.flags |= ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN SET - Set DP PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DP_PULLUP);
                break;

        case RESET:
                isp1301_private.flags &= ~ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN RESET - Clr DP PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_DP_PULLUP);
                break;
        }
}

/*! isp1301_dm_pullup_func - used to enable or disable peripheral connecting to bus
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dm_pullup_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                isp1301_private.flags |= ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN SET - Set DM PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DM_PULLUP);
                break;

        case RESET:
                isp1301_private.flags &= ~ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN RESET - Clr DM PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_DM_PULLUP);
                break;
        }
}

/*! isp1301_dp_pulldown_func - used to enable or disable peripheral connecting to bus
 *
 * C.f. 5.1.6, 5.1.7, 5.2.4 and 5.2.5
 *
 *                              host    peripheral
 *              d+ pull-up      clr     set
 *              d+ pull-down    set     clr
 *
 *              d- pull-up      clr     clr
 *              d- pull-down    set     set
 *
 */
/*! isp1301_dp_pulldown_func -
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dp_pulldown_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                isp1301_private.flags |= ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN SET - Set DP PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DP_PULLDOWN);
                break;

        case RESET:
                isp1301_private.flags &= ~ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN RESET - Clr DP PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_DP_PULLDOWN);
                break;
        }
}

/*! isp1301_dm_pulldown_func - used to enable or disable peripheral connecting to bus
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dm_pulldown_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                isp1301_private.flags |= ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN SET - Set DM PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DM_PULLDOWN);
                break;

        case RESET:
                isp1301_private.flags &= ~ISP1301_LOC_CONN;
                TRACE_MSG0(otg->tcd->TAG, "ISP1301_LOC_CONN RESET - Clr DM PULLUP");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_DM_PULLDOWN);
                break;
        }
}

/*! isp1301_peripheral_host_func - used to enable or disable peripheral connecting to bus
 *
 * A-Device             D+ pulldown     D- pulldown
 *      idle            set             set
 *      host            set             set
 *      peripheral      reset           set
 *
 * B-Device
 *      idle            set             set
 *      host            set             set
 *      peripheral      reset           set
 */
void isp1301_peripheral_host_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:       // peripheral
                TRACE_MSG0(otg->tcd->TAG, "SET - CLR DP PULLDOWN");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_DP_PULLDOWN);
                break;

        case RESET:     // host
                TRACE_MSG0(otg->tcd->TAG, "RESET - SET DM PULLDOWN");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DP_PULLDOWN);
                break;
        }
}

/*! isp1301_dm_det_func - used to enable or disable D- detect
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dm_det_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting DM_HI detect");
                isp1301_int_src_set(otg, ISP1301_DM_HI);
                isp1301_bh_wakeup(otg, TRUE);
                isp1301_debounce = 0;
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting DM_HI detect");
                isp1301_int_src_clr(otg, ISP1301_DM_HI);
                isp1301_bh_wakeup(otg, TRUE);
                isp1301_debounce = 1;
                break;
        }
}

/*! isp1301_dp_det_func - used to enable or disable D+ detect
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_dp_det_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting DP_HI detect");
                isp1301_int_src_set(otg, ISP1301_DP_HI);
                isp1301_bh_wakeup(otg, TRUE);
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting DP_HI detect");
                isp1301_int_src_clr(otg, ISP1301_DP_HI);
                isp1301_bh_wakeup(otg, TRUE);
                break;
        }
}

/*! isp1301_cr_det_func - used to enable or disable D+ detect
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_cr_det_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting CR_INT detect");
                isp1301_int_src_set(otg, ISP1301_CR_INT);
                isp1301_bh_wakeup(otg, TRUE);
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting CR_INT detect");
                isp1301_int_src_clr(otg, ISP1301_CR_INT);
                isp1301_bh_wakeup(otg, TRUE);
                break;
        }
}

/*! isp1301_bdis_acon_func - used to enable or disable auto a-connect
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_bdis_acon_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting BDIS ACON");
                isp1301_int_src_set(otg, ISP1301_BDIS_ACON);
                i2c_writeb(ISP1301_MODE_CONTROL_1_SET, ISP1301_BDIS_ACON_EN);
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting BDIS ACON");
                i2c_writeb(ISP1301_MODE_CONTROL_1_CLR, ISP1301_BDIS_ACON_EN);
                isp1301_int_src_clr(otg, ISP1301_BDIS_ACON);
                break;
        }
}

/*! isp1301_id_pulldown_func - used to enable or disable ID pulldown
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_id_pulldown_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting ID PULLDOWN");
                i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_ID_PULLDOWN);
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting ID PULLDOWN");
                i2c_writeb(ISP1301_OTG_CONTROL_CLR, ISP1301_ID_PULLDOWN);
                break;
        }
}

/*! isp1301_audio_func - used to enable or disable Carkit Interrupt
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_audio_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "SET AUDIO_EN");
                //isp1301_int_src_set(otg, ISP1301_CR_INT);
                i2c_writeb(ISP1301_MODE_CONTROL_2_SET, ISP1301_AUDIO_EN);
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "RESET AUDIO_EN");
                i2c_writeb(ISP1301_MODE_CONTROL_2_CLR, ISP1301_AUDIO_EN);
                //isp1301_int_src_clr(otg, ISP1301_CR_INT);
                break;
        }
}

/*! isp1301_uart_func - used to enable or disable transparent uart mode
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_uart_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting UART_EN");
                i2c_writeb(ISP1301_MODE_CONTROL_1_SET, ISP1301_UART_EN);
                /* XXX enable uart */
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting UART_EN");
                i2c_writeb(ISP1301_MODE_CONTROL_1_CLR, ISP1301_UART_EN);
                /* XXX disable uart */
                break;
        }
}

/*! isp1301_mono_func - used to enable or disable mono audio connection
 * @param otg - otg_instance pointer
 * @param flag -
 */
void isp1301_mono_func(struct otg_instance *otg, u8 flag)
{
        //TRACE_MSG0(otg->tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(otg->tcd->TAG, "setting MONO");
                /* XXX enable mono output */
                break;

        case RESET:
                TRACE_MSG0(otg->tcd->TAG, "reseting MONO");
                /* XXX disable mono output */
                break;
        }
}


/* ********************************************************************************************* */
#ifdef CONFIG_OTG_ISP1301_PROCFS
extern int isp1301_procfs_init (void);
extern void isp1301_procfs_exit (void);
#endif /* CONFIG_OTG_ISP1301_PROCFS */

/*! isp1301_mod_init
 * @param otg - otg_instance pointer
 * @param work_proc - task process function
 */
int isp1301_mod_init(struct otg_instance *otg, otg_task_proc_t work_proc)
{
        /* Setup work item, use isp1301_bh directly if platform is not
         * providing a bottom half wrapper.
         */
        TRACE_MSG0(otg->tcd->TAG, "1. setup work item");

	isp1301_private.work_proc = work_proc ? work_proc : &isp1301_bh;

	RETURN_EINVAL_UNLESS(( isp1301_private.task = otg_task_init2("isp1301", isp1301_work, otg, otg->tcd->TAG)));
	otg->task = isp1301_private.task;
//	isp1301_private.task->debug = TRUE;
	otg_task_start(isp1301_private.task);
        return 0;
}

/*! isp1301_mod_exit
 */
void isp1301_mod_exit(struct otg_instance *otg)
{
#ifdef CONFIG_OTG_ISP1301_PROCFS
        isp1301_procfs_exit ();
#endif /* CONFIG_OTG_ISP1301_PROCFS */
	otg_task_exit(isp1301_private.task);
}

/* ********************************************************************************************* */
/*! isp1301_configure - configure the ISP1301 for this host
 * @param otg - otg instance
 * @param tx_mode - the type of connection between the host USB and the ISP1301
 * @param spd_ctrl - suspend control method
 */
void  isp1301_configure(struct otg_instance *otg, isp1301_tx_mode_t tx_mode, isp1301_spd_ctrl_t spd_ctrl)
{
        //u16 vendor = 0, product = 0, revision = 0;
        //u16 vendor = 0, product = 0, revision = 0;
        //u8 mode1, mode2, control;
        u8 mode1, mode2;

        struct otg_transceiver_map *map = isp1301_transceiver_map;


#if defined(CONFIG_OTG_ISP1301_MX2ADS) || defined(CONFIG_OTG_ISP1301_MX2ADS_MODULE)
        u32 vp;
        TRACE_MSG0(otg->tcd->TAG, "1. Read Transceiver ID's with long read");
        vp = i2c_readl(ISP1301_VENDOR_ID);
        isp1301_private.vendor = vp & 0xffff;
        isp1301_private.product = vp >> 16;
        isp1301_private.revision = i2c_readw(ISP1301_VERSION_ID);
        mode1 = i2c_readb(ISP1301_MODE_CONTROL_1_SET);
        mode2 = i2c_readb(ISP1301_MODE_CONTROL_2_SET);

        TRACE_MSG5(otg->tcd->TAG, "2. OTG Transceiver: vendor: %04x product: %04x revision: %04x mode1: %02x mode2: %02x",
                        isp1301_private.vendor, isp1301_private.product, isp1301_private.revision, mode1, mode2);

#else /* defined(CONFIG_OTG_ISP1301_MX2ADS) */

        isp1301_private.vendor = i2c_readw(ISP1301_VENDOR_ID);
        isp1301_private.product = i2c_readw(ISP1301_PRODUCT_ID);
        isp1301_private.revision = i2c_readw(ISP1301_VERSION_ID);
        mode1 = i2c_readb(ISP1301_MODE_CONTROL_1_SET);
        mode2 = i2c_readb(ISP1301_MODE_CONTROL_2_SET);

        TRACE_MSG5(otg->tcd->TAG, "2. OTG Transceiver: vendor: %04x product: %04x revision: %04x mode1: %02x mode2: %02x",
                        isp1301_private.vendor, isp1301_private.product, isp1301_private.revision, mode1, mode2);

#endif /* defined(CONFIG_OTG_ISP1301_MX2ADS) */


        printk(KERN_INFO"%s: vendor: %04x product: %04x revision: %04x mode1: %02x mode2: %02x\n", __FUNCTION__,
                        isp1301_private.vendor, isp1301_private.product, isp1301_private.revision, mode1, mode2
                        );

        for ( ; map && map->transceiver_type != unknown_transceiver; map++) {
                CONTINUE_UNLESS ((isp1301_private.vendor == map->vendor) && (isp1301_private.product == map->product));
                TRACE_MSG1(otg->tcd->TAG, "Found Transceiver: %s", map->name);
                isp1301_private.transceiver_map = map;
                break;
        }
        UNLESS (isp1301_private.transceiver_map)
                isp1301_private.transceiver_map = isp1301_transceiver_map;

        TRACE_MSG0(otg->tcd->TAG, "3. Disable All Transceiver Control Register 1 ");
        i2c_writeb(ISP1301_OTG_CONTROL_CLR, 0xff);                                    // clear

        TRACE_MSG0(otg->tcd->TAG, "4. Enable D-/D+ Pulldowns in Transceiver Control Register 1 ");

        i2c_writeb(ISP1301_OTG_CONTROL_SET, ISP1301_DM_PULLDOWN | ISP1301_DP_PULLDOWN);
        i2c_writeb(ISP1301_OTG_CONTROL_CLR, (u8) ISP1301_DM_PULLUP | ISP1301_DP_PULLUP);


        TRACE_MSG0(otg->tcd->TAG, "5. Clear latch and enable interrupts");
        i2c_writeb(ISP1301_INTERRUPT_LATCH_CLR, 0xff);
        //i2c_writeb(ISP1301_INTERRUPT_ENABLE_LOW_CLR, 0xeb);
        //i2c_writeb(ISP1301_INTERRUPT_ENABLE_HIGH_CLR, 0xeb);
        isp1301_int_src_set(otg, 0xeb);

        /* The PSW_OE enables the ADR/PSW pin for output driving either high
         * or low depending on the address.
         *
         * ADR          ADR_REG Address         PSW_OE=0        PSW_OE=1
         *
         *      low     0       0x2c            LOW             HIGH
         *
         *      high    1       0x2d            HIGH            LOW
         *
         *
         * The i2c address by tying the ADR/PSW pin either high or low.
         * Setting the PSW_OE drives the ADR/PSW into the opposite of the
         * default wiring.
         *
         * On the MX21ADS the ADR/PSW pin is wired high which enables the
         * charge pump. So enabling the PSW_OE is required to disable Vbus
         * generation on the Charge Pump.
         *
         * The MAX3355E will only enable Vbus when this signal is high AND
         * the ID signal is low. So it may be safe to leave enabled when
         * operating as a B-device (ID floating.)
         */

        //i2c_writeb(ISP1301_MODE_CONTROL_2_SET, ISP1301_PSW_OE);              // PSW_OE - OFFVBUS low


        TRACE_MSG1(otg->tcd->TAG, "6. tx_mode: %02x", tx_mode);
        /*
         * DAT_SE0
         *      0 - VP_VM mode
         *      1 - DAT_SE0 mode
         */
        switch (tx_mode) {
        case vp_vm_unidirectional:
        case vp_vm_bidirectional:
                TRACE_MSG0(otg->tcd->TAG, "6. tx_mode: VP_VM");
                i2c_writeb(ISP1301_MODE_CONTROL_1_CLR, ISP1301_DAT_SE0);
                break;

        case dat_se0_unidirectional:
        case dat_se0_bidirectional:
                TRACE_MSG0(otg->tcd->TAG, "6. tx_mode: DAT_SE0");
                i2c_writeb(ISP1301_MODE_CONTROL_1_SET, ISP1301_DAT_SE0);
                break;
        }
        /*
         * BI_DI
         *      0 - unidirectional
         *      1 - bidirectional
         */
        switch (tx_mode) {
        case vp_vm_unidirectional:
        case dat_se0_unidirectional:
                TRACE_MSG0(otg->tcd->TAG, "6. tx_mode: unidirectional");
                i2c_writeb(ISP1301_MODE_CONTROL_2_CLR, ISP1301_BI_DI);
                break;
        case vp_vm_bidirectional:
        case dat_se0_bidirectional:
                TRACE_MSG0(otg->tcd->TAG, "6. tx_mode: bidirectional");
                i2c_writeb(ISP1301_MODE_CONTROL_2_SET, ISP1301_BI_DI);
                break;
        }
        TRACE_MSG0(otg->tcd->TAG, "6. tx_mode done");

        TRACE_MSG1(otg->tcd->TAG, "7. spd_ctrl: %02x", spd_ctrl);
        switch (spd_ctrl) {
        case spd_susp_pins:
                i2c_writeb(ISP1301_MODE_CONTROL_2_CLR, ISP1301_SPD_SUSP_CTRL);
                break;
        case spd_susp_reg:
                i2c_writeb(ISP1301_MODE_CONTROL_1_SET, ISP1301_SPEED_REG);
                i2c_writeb(ISP1301_MODE_CONTROL_2_SET, ISP1301_SPD_SUSP_CTRL);
                break;
        }

        //i2c_writeb(ISP1301_MODE_CONTROL_2_SET, ISP1301_PSW_OE);              // PSW_OE - OFFVBUS low

        TRACE_MSG0(otg->tcd->TAG, "7. spd_ctrl done");

        TRACE_MSG1(otg->tcd->TAG, "8. transceiver_type: %02x", isp1301_private.transceiver_map);

        TRACE_MSG1(otg->tcd->TAG, "8. transceiver_type: %02x", isp1301_private.transceiver_map->transceiver_type);
        switch (isp1301_private.transceiver_map->transceiver_type) {
        case isp1301:
                break;
        case max3301e:
                i2c_writeb(MAX3301E_SPECIAL_FUNCTION_1_SET, MAX3301E_SESS_END);
                break;
        case unknown_transceiver:
                break;
        }

        TRACE_MSG0(otg->tcd->TAG, "8. transceiver_type done");

        TRACE_MSG0(otg->tcd->TAG, "9. enable interrupts");
        isp1301_int_src(otg, ISP1301_ID_FLOAT | ISP1301_DM_HI | ISP1301_ID_GND | ISP1301_DP_HI | ISP1301_SESS_VLD | ISP1301_VBUS_VLD);

#ifdef CONFIG_OTG_ISP1301_PROCFS
        TRACE_MSG0(otg->tcd->TAG, "3.");
        isp1301_procfs_init ();
#endif /* CONFIG_OTG_ISP1301_PROCFS */

        /* Set the ready flag in isp1301_private to show that the structure is ready for use */
        isp1301_private.ready = TRUE;

#if 0	//For debugging

        printk(KERN_INFO"%s: VENDOR_ID %04x\n", __FUNCTION__, i2c_readb(ISP1301_VENDOR_ID));;
        printk(KERN_INFO"%s: PRODUCT_ID %04x\n", __FUNCTION__, i2c_readb(ISP1301_PRODUCT_ID));;
        printk(KERN_INFO"%s: VERSION_ID %04x\n", __FUNCTION__, i2c_readb(ISP1301_VERSION_ID));;

        printk(KERN_INFO"%s: OTG_CONTROL_SET %02x\n", __FUNCTION__, i2c_readb(ISP1301_OTG_CONTROL_SET));;
        printk(KERN_INFO"%s: INTERRUPT_SOURCE %02x\n", __FUNCTION__, i2c_readb(ISP1301_INTERRUPT_SOURCE));;
        printk(KERN_INFO"%s: INTERRUPT_LATCH_SET %02x\n", __FUNCTION__, i2c_readb(ISP1301_INTERRUPT_LATCH_SET));;
        printk(KERN_INFO"%s: INTERRUPT_ENABLE_LOW %02x\n", __FUNCTION__, i2c_readb(ISP1301_INTERRUPT_ENABLE_LOW));;
        printk(KERN_INFO"%s: INTERRUPT_ENABLE_HIGH %02x\n", __FUNCTION__, i2c_readb(ISP1301_INTERRUPT_ENABLE_HIGH));;
        printk(KERN_INFO"%s: MOD_CONTROL_1 %02x\n", __FUNCTION__, i2c_readb(ISP1301_MODE_CONTROL_1));;
        printk(KERN_INFO"%s: MOD_CONTROL_2 %02x\n", __FUNCTION__, i2c_readb(ISP1301_MODE_CONTROL_2));;
#endif
}

void isp1301_exit(void)
{
        i2c_writeb_direct(ISP1301_OTG_CONTROL_SET, ISP1301_DP_PULLDOWN | ISP1301_DM_PULLDOWN);
        i2c_writeb_direct(ISP1301_OTG_CONTROL_CLR, ISP1301_DP_PULLUP | ISP1301_DM_PULLUP);

}

#endif /* defined(CONFIG_OTG_ISP1301) */
