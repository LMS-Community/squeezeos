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
 * otg/hardware/mc13783.c -- Freescale mc13783 Connectiviey Transceiver Controller driver
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/mxc/mxc-pmic.c|20070710021518|30741
 *
 *      Copyright (c) 2005 Belcarra Technologies Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>,
 *      Shahrad Payandeh <sp@belcarra.com>
 *
 */
/*!
 * @file otg/hardware/mxc-pmic.c
 * @brief mc13783 Transciever Controller Driver
 *
 * This is a simple transceiver driver for the mc13783 transceiver
 * using the Freescale mc13783 connectivity driver.
 *
 * @ingroup FSOTG
 *
 *
 */

#include <asm/delay.h>

#include <otg/pcd-include.h>
#include <asm/arch/gpio.h>


#include <asm/arch/pmic_external.h>
#include <asm/arch/pmic_convity.h>



//WORK_ITEM pmic_work_bh;
//WORK_ITEM pmic_otg_wq;

struct otg_task *pmic_work_task;
struct otg_task *pmic_otg_task;

int global_flag = 0;
int global_flag_array[20], start_flag, end_flag;
int det_dm_hi, det_dp_hi;

struct otg_task *pmic_task;

#define PUDP_FLAG_SET 1
#define PUDP_FLAG_RESET 2
#define UPD_FLAG_SET 4
#define UPD_FLAG_RESET 8
#define UDM_FLAG_SET 16
#define UDM_FLAG_RESET 32
#define DRV_VBUS_SET 64
#define DRV_VBUS_RESET 128
#define PUDM_FLAG_SET 256
#define PUDM_FLAG_RESET 512
#define DISCHRG_VBUS_SET 1024
#define DISCHRG_VBUS_RESET 2048
#define CHRG_VBUS_SET 4096
#define CHRG_VBUS_RESET 8192

//#define VBUS_TIMER

PMIC_CONVITY_HANDLE pmic_handle = (PMIC_CONVITY_HANDLE) NULL;

/*! pmic_bh- work task bottom handler
 * @param data - otg_instance type pointer
 *
 */
void pmic_bh(void *data)
{
        struct otg_instance *otg = (struct otg_instance *) data;

        do {

//                printk(KERN_INFO"%s: AAAA\n", __FUNCTION__);
                TRACE_MSG0(REMOVE_TCD, "--");
                global_flag = global_flag_array[start_flag];
                if (global_flag & PUDP_FLAG_SET) {	//set DP pullup
                        pmic_convity_usb_set_speed(pmic_handle, USB_FULL_SPEED);
                        pmic_convity_usb_otg_set_config(pmic_handle, USB_PU);
                }
                if (global_flag & PUDP_FLAG_RESET) {    //reset DP pullup
                        pmic_convity_usb_set_speed(pmic_handle, USB_FULL_SPEED);
                        pmic_convity_usb_otg_clear_config(pmic_handle, USB_PU);
                }
                if (global_flag & PUDM_FLAG_SET) {      //set DM pullup
                        pmic_convity_usb_set_speed(pmic_handle, USB_LOW_SPEED);
                        pmic_convity_usb_otg_set_config(pmic_handle, USB_PU);
                }
                if (global_flag & PUDM_FLAG_RESET) {    //reset DM pullup
                        pmic_convity_usb_set_speed(pmic_handle, USB_LOW_SPEED);
                        pmic_convity_usb_otg_clear_config(pmic_handle, USB_PU);
                }
                if (global_flag & UPD_FLAG_SET) {       //set DP pulldown
                        pmic_convity_usb_otg_set_config(pmic_handle, USB_UDP_PD);       //DP pull down switch is on
                }
                if (global_flag & UPD_FLAG_RESET) {     //reset DP pulldown
                        pmic_convity_usb_otg_clear_config(pmic_handle, USB_UDP_PD);     //DP pull down switch is off
                }
                if (global_flag & UDM_FLAG_SET) {       //set DM pulldown
                        pmic_convity_usb_otg_set_config(pmic_handle, USB_UDM_PD);       //DP pull down switch is on
                }
                if (global_flag & UDM_FLAG_RESET) {     //reset DM pulldown
                        pmic_convity_usb_otg_clear_config(pmic_handle, USB_UDM_PD);     //DP pull down switch is off
                }
                if (global_flag & DRV_VBUS_SET) {       //enable vbus voltage
                        pmic_convity_set_output(pmic_handle, TRUE, TRUE);       //enable VBUS
                }
                if (global_flag & DRV_VBUS_RESET) {     //disable vbus voltage
                        pmic_convity_set_output(pmic_handle, TRUE, FALSE);      //disable VBUS
                }
                if (global_flag & CHRG_VBUS_SET) {      //enable vbus
                        pmic_convity_usb_otg_set_config(pmic_handle,
                                        USB_VBUS_CURRENT_LIMIT_LOW_30MS);
                }
                if (global_flag & CHRG_VBUS_RESET) {    //disable vbus
                        pmic_convity_set_output(pmic_handle, TRUE, FALSE);      //disable VBUS
                }
                if (global_flag & DISCHRG_VBUS_SET) {   //discharge vbus
                        pmic_convity_set_output(pmic_handle, TRUE, FALSE);      //disable VBUS
                        pmic_convity_usb_otg_clear_config(pmic_handle,
                                        USB_VBUS_PULLDOWN);
                        pmic_convity_usb_set_power_source(pmic_handle,
                                        USB_POWER_INTERNAL,
                                        USB_POWER_3V3);
                        pmic_convity_set_output(pmic_handle, FALSE, TRUE);      //enable VUSB
                }
                if (global_flag & DISCHRG_VBUS_RESET) { //discharge vbus disable
                        pmic_convity_set_output(pmic_handle, TRUE, FALSE);      //disable VBUS
                        pmic_convity_usb_otg_set_config(pmic_handle, USB_VBUS_PULLDOWN);
                        pmic_convity_set_output(pmic_handle, FALSE, TRUE);      //enable VUSB
                }
#if 1
                TRACE_MSG3(REMOVE_TCD, "gloabl flag %d start_flag %d end_flag %d", global_flag,
                                start_flag, end_flag);
#endif
#if 0
                printk(KERN_INFO "%d %d %d %d %d %d %d %d\n",
                                (global_flag & PU_FLAG_SET), (global_flag & PU_FLAG_RESET),
                                (global_flag & UPD_FLAG_SET), (global_flag & UPD_FLAG_RESET),
                                (global_flag & UDM_FLAG_SET), (global_flag & UDM_FLAG_RESET),
                                (global_flag & VBUSPDENB_RESET),
                                (global_flag & VBUSREGEN_RESET));
#endif
                global_flag = 0;
                global_flag_array[start_flag] = 0;
                if (start_flag++ > 15)
                        start_flag = 0;
                //if (start_flag != end_flag)
                //      SCHEDULE_WORK(pmic_work_bh);

//                printk(KERN_INFO"%s: BBBB start: %d end: %d\n", __FUNCTION__, start_flag, end_flag);
        } while (start_flag != end_flag);

//        printk(KERN_INFO"%s: CCCC\n", __FUNCTION__);
}
/*! pmic_work_proc - pmic tcd task roution
 * @param data
 */
void *pmic_work_proc(otg_task_arg_t data)
{
        pmic_bh(data);
        return NULL;
}

/*! pmic_bh_wakeup - wakeup the pmic bottom half
 *  */
void pmic_bh_wakeup(void)
{
        TRACE_MSG0(REMOVE_TCD, "--");
        //SCHEDULE_WORK(pmic_work_bh);
        otg_up_work(pmic_work_task);
}

/* ********************************************************************************************** */
/*! mxc_pmic_vbus - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 * @param otg - otg instance pointer
 */
int mxc_pmic_vbus(struct otg_instance *otg)
{
#if 1
        t_sensor_bits sense_bits;
        if (pmic_get_sensors(&sense_bits)) {
                printk(KERN_INFO "%s: pmic_get_sensors() failed\n",
                                __FUNCTION__);
                return 0;
        }
        return sense_bits.sense_usb2v0s;
#else
        return pmic_check_sense(sense_usb2v0s);
#endif
}
/*! pmic_otg_event_bh_old -
 * @param arg
 */
void pmic_otg_event_bh_old(void *arg)
{
        otg_event_set_irq(REMOVE_tcd_instance->otg, 1,
                        mxc_pmic_vbus(REMOVE_tcd_instance->otg), B_SESS_VLD, REMOVE_TCD,
                        "B_SESS_VLD");
}

/*! pmic_otg_event_bh - pmic otg event handler
 * @param data - otg instance
 */
void pmic_otg_event_bh(void *data)
{
        struct otg_instance *otg = (struct otg_instance *) data;
        otg_current_t inputs;
        t_sensor_bits sense_bits;
        static BOOL force = TRUE;
        static otg_current_t inputs_saved = 0;

        if (pmic_get_sensors(&sense_bits)) {
                printk(KERN_INFO "%s: pmic_get_sensors() failed\n",
                                __FUNCTION__);
                return;
        }
        TRACE_MSG6(REMOVE_TCD,
                        "usb4v4s%c usb2v0s%c usb0v8s:%c id_gnds%c id_floats%c id_se1s%c",
                        sense_bits.sense_usb4v4s ? ' ' : '/',
                        sense_bits.sense_usb2v0s ? ' ' : '/',
                        sense_bits.sense_usb0v8s ? ' ' : '/',
                        sense_bits.sense_id_gnds ? ' ' : '/',
                        sense_bits.sense_id_floats ? ' ' : '/',
                        sense_bits.sense_se1s ? ' ' : '/');

        inputs = (sense_bits.sense_usb4v4s ? VBUS_VLD : VBUS_VLD_) |
                (sense_bits.
                 sense_usb2v0s ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ |
                         A_SESS_VLD_)) |
                (sense_bits.sense_usb0v8s ? B_SESS_END_ : B_SESS_END) | (sense_bits.
                                sense_id_gnds
                                ? ID_GND :
                                ID_GND_) |
                (sense_bits.sense_id_floats ? ID_FLOAT : ID_FLOAT_) | (sense_bits.
                                sense_se1s ?
                                SE1_DET :
                                SE1_DET_) |
                (det_dp_hi ? DP_HIGH : DP_HIGH_) | (det_dm_hi ? DM_HIGH : DM_HIGH_);

        //      printk(KERN_INFO" inputs: %8X\n", inputs);
        TRACE_MSG4(REMOVE_TCD,
                        "MC13783 EVENT: sense_bits: %8x otg inputs: %8x saved: %x diff: %x",
                        sense_bits.sense_se1s, inputs, inputs_saved,
                        inputs ^ inputs_saved);

        RETURN_UNLESS(force || (inputs ^ inputs_saved));

        inputs_saved = inputs;
        otg_event(REMOVE_tcd_instance->otg, inputs, REMOVE_TCD, "PMIC OTG EVENT");

        //      gpio_config_int_en(2, 17, TRUE);
        //      gpio_config_int_en(2, 16, TRUE);

        //      gpio_clear_int (2, 17);
        //      gpio_clear_int (2, 16);

}
/*! pmic_otg_proc - called to porcess pmic otg event
 * @param data - otg instance type pointer
 */
void *pmic_otg_proc(otg_task_arg_t data)
{
        pmic_otg_event_bh(data);
        return NULL;
}

/*! pmic_otg_wakeup - called to wake up pmic_otg_task
 */
void pmic_otg_wakeup(void)
{
        TRACE_MSG0(REMOVE_TCD, "start");
        //SCHEDULE_WORK(pmic_otg_wq);
        otg_up_work(pmic_otg_task);
        TRACE_MSG0(REMOVE_TCD, "finsih");
}

/*! gpio_c17_int_hndlr - interrupt handler
 * @param irq - interrupt number
 * @param dev_id - interrupt device ip
 * @param regs - cpu registers snapshot
 * @return interrput process result
 */
static irqreturn_t gpio_c17_int_hndlr(int irq, void *dev_id,
                struct pt_regs *regs)
{
        udelay(100);
        if (gpio_get_data(2, 17) == 1) {
                det_dm_hi = 1;
                //      mc13783_otg_wakeup ();
                //      gpio_config_int_en(2, 16, FALSE);
        } else {
                det_dm_hi = 0;
                //      gpio_config_int_en(2, 17, TRUE);
        }
        TRACE_MSG1(REMOVE_TCD, "Changing the state of DM to %d", det_dm_hi);
        pmic_otg_wakeup();
        return IRQ_HANDLED;
}
/*! gpio_c16_int_hndlr - interrupt handler
 * @param irq - interrupt number
 * @param dev_id - interrupt device ip
 * @param regs - cpu registers snapshot
 * @return interrput process result
 */

static irqreturn_t gpio_c16_int_hndlr(int irq, void *dev_id,
                struct pt_regs *regs)
{
        udelay(100);
        if (gpio_get_data(2, 16) == 1) {
                det_dp_hi = 1;
                //      mc13783_otg_wakeup ();
                //      gpio_config_int_en(2, 17, FALSE);
        } else {
                det_dp_hi = 0;
                //      gpio_config_int_en(2, 16, TRUE);
        }
        TRACE_MSG1(REMOVE_TCD, "Changing the state of DP to %d", det_dp_hi);
        pmic_otg_wakeup();
        return IRQ_HANDLED;
}

/*! mxc_pmic_id - Do we have Vbus (cable attached?)
 * Return non-zero if Vbus is detected.
 *
 * @param otg - otg instance
 */
int mxc_pmic_id(struct otg_instance *otg)
{
        struct tcd_instance *tcd = otg->tcd;
#if 1
        t_sensor_bits sense_bits;
        if (pmic_get_sensors(&sense_bits)) {
                printk(KERN_INFO "%s: pmic_get_sensors() failed\n",
                                __FUNCTION__);
                return 0;
        }
        return sense_bits.sense_id_gnds;
#else
        return pmic_check_sense(sense_id_gnds);
#endif
}

/* ********************************************************************************************* */
/*! mxc_pmic_tcd_en() - used to enable/ disable mc13783
 *
 * @param otg - otg instance
 * @param flag - enable/ disable flag
 */
void mxc_pmic_tcd_en(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        switch (flag) {
        case SET:
        case PULSE:
                TRACE_MSG0(tcd->TAG, "SET/PULSE");
                pmic_otg_wakeup();
                //                otg_event_set_irq(tcd_instance->otg, 1, mxc_mc13783_vbus(tcd_instance->otg), B_SESS_VLD, TCD, "B_SESS_VLD");
                break;
        case RESET:
                TRACE_MSG0(tcd->TAG, "RESET");
                break;
        }
}

/* ********************************************************************************************* */
/*! mxc_pmic_tcd_init() - used to enable mc13783
 * @param otg - otg instance pointer
 * @param flag -
 *
 */
void mxc_pmic_tcd_init(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        switch (flag) {
        case SET:
        case PULSE:
                TRACE_MSG0(tcd->TAG, "SET/PULSE");
                break;
        case RESET:
                TRACE_MSG0(tcd->TAG, "RESET");
                break;
        }
        pmic_otg_wakeup();
        otg_event (otg, OCD_OK, otg->tcd->TAG, "MC13783 OK");
}

/*! mxc_pmic_chrg_vbus - used to enable or disable B-device Vbus charging
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_chrg_vbus(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "CHRG_VBUS_SET");
                global_flag_array[end_flag] = CHRG_VBUS_SET;
                pmic_bh_wakeup();
                break;
        case RESET:
                TRACE_MSG0(tcd->TAG, "CHRG_VBUS_RESET");
                global_flag_array[end_flag] = CHRG_VBUS_RESET;
                pmic_bh_wakeup();
                break;
        case PULSE:
                break;
        }
        if (end_flag++ > 15)
                end_flag = 0;

}

/*! mxc_pmic_drv_vbus - used to enable or disable A-device driving Vbus
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_drv_vbus(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "DRV_VBUS_SET");
                global_flag_array[end_flag] = DRV_VBUS_SET;
                pmic_bh_wakeup();
                break;
        case RESET:
                TRACE_MSG0(tcd->TAG, "DRV_VBUS_RESET");
                global_flag_array[end_flag] = DRV_VBUS_RESET;
                pmic_bh_wakeup();
                break;
        }
        if (end_flag++ > 15)
                end_flag = 0;

}

/*! mxc_pmic_dischrg_vbus - used to enable Vbus discharge
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_dischrg_vbus(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "DISCHRG VBUS SET");
                global_flag_array[end_flag] = DISCHRG_VBUS_SET;
                pmic_bh_wakeup();
                break;
        case RESET:
                TRACE_MSG0(tcd->TAG, "DISCHRG VBUS RESET");
                global_flag_array[end_flag] = DISCHRG_VBUS_RESET;
                pmic_bh_wakeup();
                break;
        }
}

/*! mxc_pmic_mx21_vbus_drain - used to enable Vbus discharge
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_mx21_vbus_drain_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "OUTPUT: TCD_DISCHRG_VBUS_SET");
                break;
        case RESET:
                TRACE_MSG0(tcd->TAG, "OUTPUT: TCD_DISCHRG_VBUS_RESET");
                break;
        }
}

/*! mxc_pmic_dp_pullup_func - used to enable or disable peripheral connecting to bus
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
 * @param otg - otg instance
 * @param flag - enable/disable flag

 */
void mxc_pmic_dp_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN SET - Set DP PULLUP");
                //      global_flag |= PU_FLAG_SET;
                global_flag_array[end_flag] = PUDP_FLAG_SET;
                pmic_bh_wakeup();
                //                mc13783_convity_set_pull_down_switch(PD_PU, TRUE);
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN RESET - Clr DP PULLUP");
                //      global_flag |= PU_FLAG_RESET;
                global_flag_array[end_flag] = PUDP_FLAG_RESET;
                pmic_bh_wakeup();
                //                mc13783_convity_set_pull_down_switch(PD_PU, FALSE);
                break;
        }
        if (end_flag++ > 15)
                end_flag = 0;
}

/*! mxc_pmic_dm_pullup_func - used to enable or disable peripheral connecting to bus
 * @param otg - otg instance
 * @param flag - enable/disable flag

 */
void mxc_pmic_dm_pullup_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN SET - Set DM PULLUP");
                //      global_flag |= PU_FLAG_SET;
                global_flag_array[end_flag] = PUDM_FLAG_SET;
                pmic_bh_wakeup();
                //mc13783_convity_set_pull_down_switch(PD_UDB_15, TRUE);
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN RESET - Clr DM PULLUP");
                //              global_flag |= PU_FLAG_RESET;
                global_flag_array[end_flag] = PUDM_FLAG_RESET;
                pmic_bh_wakeup();
                //mc13783_convity_set_pull_down_switch(PD_UDB_15, FALSE);
                break;
        }
        if (end_flag++ > 15)
                end_flag = 0;

}

/*! mxc_pmic_dp_pulldown_func - used to enable or disable peripheral connecting to bus
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
 *
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_dp_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN SET - Set DP PULLDOWN");
                //      global_flag |= UPD_FLAG_SET;
                global_flag_array[end_flag] = UPD_FLAG_SET;
                pmic_bh_wakeup();
                //                mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN RESET - Clr DP PULLDOWN");
                //      global_flag |= UPD_FLAG_RESET;
                global_flag_array[end_flag] = UPD_FLAG_RESET;
                pmic_bh_wakeup();
                //                mc13783_convity_set_pull_down_switch(PD_UPD_15, TRUE);
                break;
        }
        if (end_flag++ > 15)
                end_flag = 0;

}

/*! mxc_pmic_dm_pulldown_func - used to enable or disable peripheral connecting to bus
 *
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_dm_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN SET - Set DM PULLDOWN");
                //      global_flag |= UDM_FLAG_SET;
                global_flag_array[end_flag] = UDM_FLAG_SET;
                pmic_bh_wakeup();
                //                mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "MC13783_LOC_CONN RESET - Clr DM PULLDOWN");
                //      global_flag |= UDM_FLAG_RESET;
                global_flag_array[end_flag] = UDM_FLAG_RESET;
                pmic_bh_wakeup();
                //                mc13783_convity_set_pull_down_switch(PD_UDM_15, TRUE);
                break;
        }
        if (end_flag++ > 15)
                end_flag = 0;

}

/*! mxc_pmic_peripheral_host_func - used to enable or disable peripheral connecting to bus
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
 *
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_peripheral_host_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:       // peripheral
                TRACE_MSG0(tcd->TAG, "SET - CLR DP PULLDOWN");
                break;

        case RESET:     // host
                TRACE_MSG0(tcd->TAG, "RESET - SET DM PULLDOWN");
                break;
        }
}

/*! mxc_pmic_dm_det_func - used to enable or disable D- detect
 *
 * @param otg - otg instance
 * @param flag - enable/disable flag

 */
void mxc_pmic_dm_det_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting DM_HI detect");
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting DM_HI detect");
                break;
        }
}

/*! mxc_pmic_dp_det_func - used to enable or disable D+ detect
 *
 * @param otg - otg instance
 * @param flag - enable/disable flag
 */
void mxc_pmic_dp_det_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting DP_HI detect");
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting DP_HI detect");
                break;
        }
}

/*! mxc_pmic_cr_det_func - used to enable or disable D+ detect
 * @param otg - otg instance
 * @param flag - enable/disable flag
 *
 */
void mxc_pmic_cr_det_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting CR_INT detect");
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting CR_INT detect");
                break;
        }
}

/*! mxc_pmic_bdis_acon_func - used to enable or disable auto a-connect
 * @param otg - otg instance
 * @param flag - enable/disable flag
 *
 */
void mxc_pmic_bdis_acon_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting BDIS ACON");
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting BDIS ACON");
                break;
        }
}

/*! mxc_pmic_id_pulldown_func - used to enable or disable ID pulldown
 * @param otg - otg instance
 * @param flag - enable/disable flag
 *
 */
void mxc_pmic_id_pulldown_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting ID PULLDOWN");
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting ID PULLDOWN");
                break;
        }
}

/*! mxc_pmic_audio_func - used to enable or disable Carkit Interrupt
 * @param otg - otg instance
 * @param flag - enable/disable flag
 *
 */
void mxc_pmic_audio_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "SET AUDIO_EN");
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "RESET AUDIO_EN");
                break;
        }
}

/*! mxc_pmic_uart_func - used to enable or disable transparent uart mode
 * @param otg - otg instance
 * @param flag - enable/disable flag
 *
 */
void mxc_pmic_uart_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting UART_EN");
                /* XXX enable uart */
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting UART_EN");
                /* XXX disable uart */
                break;
        }
}

/*! mxc_pmic_mono_func - used to enable or disable mono audio connection
 * @param otg - otg instance
 * @param flag - enable/disable flag
 *
 */
void mxc_pmic_mono_func(struct otg_instance *otg, u8 flag)
{
        struct tcd_instance *tcd = (struct tcd_instance *)otg->tcd;
        //TRACE_MSG0(tcd->TAG, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(tcd->TAG, "setting MONO");
                /* XXX enable mono output */
                break;

        case RESET:
                TRACE_MSG0(tcd->TAG, "reseting MONO");
                /* XXX disable mono output */
                break;
        }
}

/* ********************************************************************************************* */
/*!
 * mxc_pmic_usbi_handler() - event handler
 *
 *
 *
 */
void pmic_usbi_handler(void)
{

        TRACE_MSG0(REMOVE_TCD, "--");
        pmic_otg_wakeup();

#if 0
#if 1
        t_sensor_bits sense_bits;
        if (pmic_get_sensors(&sense_bits)) {
                printk(KERN_INFO "%s: pmic_get_sensors() failed\n",
                                __FUNCTION__);
                return;
        }

        TRACE_MSG3(REMOVE_TCD, "MC13783 EVENT: 4V4S: %d 2V0S: %d 0V8S: %d",
                        sense_bits.sense_usb4v4s,
                        sense_bits.sense_usb2v0s, sense_bits.sense_usb0v8s);

        otg_event(tcd_instance->otg,
                        (sense_bits.sense_usb4v4s ? VBUS_VLD : VBUS_VLD_) |
                        (sense_bits.
                         sense_usb2v0s ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ |
                                 A_SESS_VLD_)) |
                        (sense_bits.sense_usb0v8s ? B_SESS_END : B_SESS_END_), REMOVE_TCD,
                        "MC13783 USBI");
#else
        otg_event(tcd_instance->otg,
                        (pmic_check_sense(sense_usb4v4s) ? VBUS_VLD : VBUS_VLD_) |
                        (pmic_check_sense(sense_usb4v4s)
                         ? (B_SESS_VLD | A_SESS_VLD) : (B_SESS_VLD_ | A_SESS_VLD_)) |
                        (pmic_check_sense(sense_usb0v8s) ? B_SESS_END :
                         B_SESS_END_), REMOVE_TCD, "MC13783 USBI");
#endif
#endif
}

void pmic_idi_handler(void)
{

        TRACE_MSG0(REMOVE_TCD, "--");
        pmic_otg_wakeup();

#if 0
#if 1
        t_sensor_bits sense_bits;
        if (pmic_get_sensors(&sense_bits)) {
                printk(KERN_INFO "%s: pmic_get_sensors() failed\n",
                                __FUNCTION__);
                return;
        }

        TRACE_MSG2(REMOVE_TCD, "MC13783 EVENT: IDGNDS: %d IDFLOATS: %d",
                        sense_bits.sense_id_gnds, sense_bits.sense_id_floats);

        otg_event(tcd_instance->otg,
                        (sense_bits.sense_id_gnds ? ID_GND : ID_GND_) |
                        (sense_bits.sense_id_floats ? ID_FLOAT : ID_FLOAT_),
                        REMOVE_TCD, "MC13783 IDI");
#else
        otg_event(tcd_instance->otg,
                        (pmic_check_sense(sense_id_gnds) ? ID_GND : ID_GND_) |
                        (pmic_check_sense(sense_id_floats) ? ID_FLOAT : ID_FLOAT_),
                        REMOVE_TCD, "MC13783 IDI");
#endif
#endif
}

void pmic_se1i_handler(void)
{

        TRACE_MSG0(REMOVE_TCD, "--");
        pmic_otg_wakeup();

#if 0
#if 1
        t_sensor_bits sense_bits;
        if (pmic_get_sensors(&sense_bits)) {
                printk(KERN_INFO "%s: pmic_get_sensors() failed\n",
                                __FUNCTION__);
                return;
        }
        TRACE_MSG1(REMOVE_TCD, "MC13783 EVENT: se1: %d", sense_bits.sense_se1s);
        otg_event(tcd_instance->otg,
                        (sense_bits.sense_se1s ? SE1_DET : SE1_DET_),
                        REMOVE_TCD, "MC13783 SE1");
#else
        otg_event(tcd_instance->otg,
                        (pmic_check_sense(sense_se1s) ? SE1_DET : SE1_DET_),
                        REMOVE_TCD, "MC13783 SE1");
#endif
#endif
}




void pmic_detect_event(const PMIC_CONVITY_EVENTS event)
{
        unsigned int flags = 0;

        switch (event) {
        case USB_DETECT_4V4_RISE:
                flags &= ~(VBUS_VLD_);
                flags |= VBUS_VLD;
                break;
        case USB_DETECT_4V4_FALL:
                flags &= ~(VBUS_VLD);
                flags |= VBUS_VLD_;
                break;
        case USB_DETECT_2V0_RISE:
                flags &= ~(B_SESS_VLD_ | A_SESS_VLD_);
                flags |= (B_SESS_VLD | A_SESS_VLD);
                break;
        case USB_DETECT_2V0_FALL:
                flags &= ~(B_SESS_VLD | A_SESS_VLD);
                flags |= (B_SESS_VLD_ | A_SESS_VLD_);
                break;
        case USB_DETECT_0V8_RISE:
                flags &= ~(B_SESS_END_);
                flags |= B_SESS_END;
                break;
        case USB_DM_HI:
                flags &= ~(DM_HIGH);
                flags |= DM_HIGH;
                break;
        case USB_DP_HI:
                flags &= ~(DP_HIGH);
                flags |= DP_HIGH;
                break;
        case USB_DETECT_0V8_FALL:
                flags &= ~(B_SESS_END);
                flags |= B_SESS_END_;
                break;
        case USB_DETECT_MINI_A:
                flags &= ~(ID_GND);
                flags |= ID_GND;
                break;
        case USB_DETECT_MINI_B:
                flags &= ~(ID_FLOAT);
                flags |= ID_FLOAT;
                break;
        case USB_DETECT_NON_USB_ACCESSORY:
                flags &= (ID_FLOAT | ID_GND);
                flags |= (ID_FLOAT_ | ID_GND_);
                break;
        case USB_DETECT_FACTORY_MODE:
                flags &= (ID_FLOAT | ID_GND);
                flags |= (ID_FLOAT | ID_GND);
                break;
        }
#if 0
        flags = (USB_DETECT_4V4_RISE ? VBUS_VLD : VBUS_VLD_) |
                (USB_DETECT_2V0_RISE ? (B_SESS_VLD | A_SESS_VLD)
                 : (B_SESS_VLD_ | A_SESS_VLD_)) | (USB_DETECT_0V8_RISE ? B_SESS_END
                 : B_SESS_END_);
#endif
        pmic_otg_wakeup();
}



void mxc_pmic_mod_exit(struct otg_instance *otg)
{

        PMIC_STATUS rc;
        if (pmic_work_task) {
                otg_task_exit(pmic_work_task);
                pmic_work_task = NULL;
        }
        if (pmic_otg_task) {
                otg_task_exit(pmic_otg_task);
                pmic_otg_task = NULL;
        }
        //while (PENDING_WORK_ITEM(pmic_work_bh)) {
        //      printk(KERN_ERR "%s: waiting for mc13783_work_bh\n",
        //      __FUNCTION__);
        //      schedule_timeout(10 * HZ);
        //}
        //while (PENDING_WORK_ITEM(pmic_otg_wq)) {
        //      printk(KERN_ERR "%s: waiting for mc13783_otg_wq\n",
        //      __FUNCTION__);
        //      schedule_timeout(10 * HZ);
        //}


        pmic_convity_set_mode(pmic_handle, RS232_1);
        pmic_convity_usb_otg_set_config(pmic_handle, USB_PULL_OVERRIDE);
        pmic_convity_usb_otg_set_config(pmic_handle, USB_OTG_SE0CONN);
        pmic_convity_usb_otg_clear_config(pmic_handle, USB_USBCNTRL);

        pmic_convity_usb_otg_clear_config(pmic_handle, USB_PU);
        pmic_convity_usb_otg_clear_config(pmic_handle, USB_UDP_PD);
        pmic_convity_usb_otg_clear_config(pmic_handle, USB_UDM_PD);
        pmic_convity_clear_callback(pmic_handle);

        rc = pmic_convity_close(pmic_handle);
        if (rc != PMIC_SUCCESS) {
                pr_debug("pmic_convity_close() returned error %d", rc);
        }


}

/* ********************************************************************************************* */

int mxc_pmic_mod_init(struct otg_instance *otg)
{


        PMIC_CONVITY_USB_TRANSCEIVER_MODE transceiver;
        PMIC_STATUS rc = PMIC_ERROR;

        rc = pmic_convity_open(&pmic_handle, USB);
        if (rc != PMIC_SUCCESS) {
                printk(KERN_INFO "Failed to connect to the transceiver\n");
        }


        TRACE_MSG0(REMOVE_TCD, "Setup the work item");
        //PREPARE_WORK_ITEM(pmic_work_bh, &pmic_bh, NULL);
        //PREPARE_WORK_ITEM(pmic_otg_wq, &pmic_otg_event_bh, NULL);

        THROW_UNLESS((pmic_work_task = otg_task_init2("tcdwork", pmic_work_proc, otg, otg->tcd->TAG)), error);
        THROW_UNLESS((pmic_otg_task = otg_task_init2("tcdotg", pmic_otg_proc, otg, otg->tcd->TAG)), error);

        otg_task_start(pmic_work_task);
        otg_task_start(pmic_otg_task);



        if (pmic_convity_usb_get_xcvr(pmic_handle, &transceiver))
                printk(KERN_INFO
                                "%s: pmic_convity_get_usb_transciver failed\n",
                                __FUNCTION__);

        printk(KERN_INFO "%s: tw: %02x\n", __FUNCTION__, transceiver);


        pmic_convity_set_callback(pmic_handle, pmic_detect_event,
                        USB_DETECT_4V4_RISE | USB_DETECT_4V4_FALL |
                        USB_DETECT_2V0_RISE | USB_DETECT_2V0_FALL |
                        USB_DETECT_0V8_RISE | USB_DETECT_0V8_FALL |
                        USB_DETECT_MINI_A | USB_DETECT_MINI_B);
        if (rc != PMIC_SUCCESS) {
        }

        // XXX it may be more appropriate to do this in the enable function
        //         // and reset to something when disaabled
        pmic_convity_usb_otg_set_config(pmic_handle, USB_PULL_OVERRIDE);
        pmic_convity_usb_otg_set_config(pmic_handle, USB_USBCNTRL);

        start_flag = end_flag = 0;

        CATCH(error) {

                if (pmic_work_task) {
                        otg_task_exit(pmic_work_task);
                        pmic_work_task = NULL;
                }
                if (pmic_otg_task) {
                        otg_task_exit(pmic_otg_task);
                        pmic_otg_task = NULL;
                }
                return -EINVAL;
        }
        return 0;


}
