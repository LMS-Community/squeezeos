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
 * otg/otgcore/otg-mesg.c - OTG state machine Administration
 * @(#) sl@belcarra.com/whiskey.enposte.net|otg/otgcore/otg-mesg.c|20061220065259|48878
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@lbelcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/otgcore/otg-mesg.c
 * @brief OTG Core Message Facility.
 *
 * The OTG Core Message Facility has two functions:
 *
 *      1. implement a message queue for messages from the otg state machine to the otg
 *         management application
 *
 *      2. implement an ioctl control mechanism allowing the otg managment application
 *         to pass data and commands to the otg state machine
 *
 * @ingroup OTGMESG
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>

//#if defined(OTG_LINUX)
//#include <linux/poll.h>
//#include <linux/sched.h>
//#endif /* defined(OTG_LINUX) */

//#define OTG_MESSAGE_BUF_SIZE    512
#define OTG_MESSAGE_BUF_SIZE    2048

struct otg_instance * mesg_otg_instance;

char *otg_message_buf;          // buffer
char *otg_message_head;         // next available empty space
char *otg_message_tail;         // next available data if not equal to head

/*!
 * otg_data_avail() - amount of data in buffer
 *                                      space                   data
 *           tp     hp                   20 - (10 - 5) = 15      (10 - 5)
 *     0......5.data.10......20
 *
 *          hp     tp                   (10 - 5) = 5            20 - (10 - 5)
 *     0.data.5......10.data.20
 *
 * @return Bytes available in message ring buffer.
 */
int otg_data_avail(void)
{
        int avail = (otg_message_head >= otg_message_tail) ?
                (otg_message_head - otg_message_tail) :
                OTG_MESSAGE_BUF_SIZE - (otg_message_tail - otg_message_head) ;

        //TRACE_MSG4(CORE, "buf: %p head: %p tail: %p  data avail: %d", otg_message_buf, otg_message_head, otg_message_tail, avail);
        return avail;
}

/*!
 * otg_space_avail() - amount of space in buffer
 *
 * @return Space available in message ring buffer.
 */
int otg_space_avail(void)
{
        int avail = (otg_message_head >= otg_message_tail) ?
                OTG_MESSAGE_BUF_SIZE - (otg_message_head - otg_message_tail) :
                (otg_message_tail - otg_message_head) ;

        //TRACE_MSG4(CORE, "buf: %p head: %p tail: %p space avail: %d", otg_message_buf, otg_message_head, otg_message_tail, avail);
        return avail;
}

/*!
 * otg_get_byte() - return next byte
 *
 * @return Next byte in message ring buffer
 */
char otg_get_byte(void)
{
        char c = *otg_message_tail;

        //TRACE_MSG3(CORE, "buf: %p head: %p tail: %p", otg_message_buf, otg_message_head, otg_message_tail);
        if ((otg_message_tail - otg_message_buf) == OTG_MESSAGE_BUF_SIZE)
                otg_message_tail = otg_message_buf;
        else
                otg_message_tail++;

        return c;
}

/*!
 * otg_put_byte() - put byte into buffer
 *
 * Put byte into message ring buffer
 *
 * @param byte put into message ring.
 */
void otg_put_byte(char byte)
{
        *otg_message_head = byte;
        if ((otg_message_head - otg_message_buf) == OTG_MESSAGE_BUF_SIZE)
                otg_message_head = otg_message_buf;
        else
                otg_message_head++;

}

/*!
 * otg_write_message_irq() - queue message data (interrupt version)
 *
 * Attempt to write message data into ring buffer, return amount
 * of data actually written.
 * @param otg - otg instance pointer
 * @param buf - pointer to data
 * @param size - amount of available data
 * @return number of bytes that where not written.
 */
static int otg_write_message_irq (struct otg_instance *otg, char *buf, int size)
{
        struct pcd_instance *pcd = NULL;
        struct usbd_bus_instance *bus = NULL;

        pcd = (struct pcd_instance *)mesg_otg_instance->pcd;
        if (pcd)
                bus = pcd->bus;

        if (size >= (OTG_MESSAGE_BUF_SIZE - 2))
                return size;

        while (otg_space_avail() < (size + 1))
                otg_get_byte();


        /* copy, note that we don't have to test for space as we have
         * already ensured that there is enough room for all of the data
         */
        while (size--)
                otg_put_byte(*buf++);

        //otg_put_byte('\n');

        return size;
}

/*!
 * otg_write_message() - queue message data
 *
 * Attempt to write message data into ring buffer, return amount
 * of data actually written.
 * @param otg - otg instance pointer
 * @param buf - pointer to data
 * @param size - amount of available data
 * @return number of bytes that where not written.
 */
int otg_write_message (struct otg_instance *otg, char *buf, int size)
{
        int rc;
        otg_pthread_mutex_lock(&mesg_otg_instance->mutex);         /* lock mutex */
        rc = otg_write_message_irq(otg, buf, size);
        otg_pthread_mutex_unlock(&mesg_otg_instance->mutex);      /* unlock mutex */
        return size;
}

/*!
 * otg_message() - queue message data
 * @param otg pointer to otg instance
 * @param buf message to send.
 */
void otg_message (struct otg_instance *otg, char *buf)
{
        char time_buf[64];
        static otg_tick_t save_ticks;
        otg_tick_t new_ticks;
        otg_tick_t ticks;

        new_ticks = otg_tmr_ticks();

        otg_write_message(otg, buf, strlen(buf));

        ticks = otg_tmr_elapsed(&new_ticks, &save_ticks);
        save_ticks = new_ticks;

        if (ticks < 10000)
                sprintf(time_buf, " %ld uS\n", ticks);

        else if (ticks < 10000000) {
                //ticks = otg_do_div(ticks, (otg_tick_t)1000);
                sprintf(time_buf, " %ld mS\n", ticks / 1000);
        }
        else {
                //ticks = otg_do_div(ticks, (otg_tick_t)1000000);
                sprintf(time_buf, " %ld S\n", ticks / 1000000);
        }

        //TRACE_MSG3(CORE, "ticks: new_ticks: %d - save_ticks: %d = ticks: %d ", new_ticks, save_ticks, ticks);
        //TRACE_STRING(CORE, "time_buf: %s", time_buf);

        otg_write_message(otg, time_buf, strlen(time_buf));

        if (otg->message_task)
                otg_up_work(otg->message_task);
}


/*!
 * otg_read_message() - get queued message data
 *
 * @param buf - pointer to data
 * @param size - amount of available data
 * @return the number of bytes read.
 */
int otg_read_message(char *buf, int size)
{
        int count = 0;
        otg_pthread_mutex_lock(&mesg_otg_instance->mutex);         /* lock mutex */

        /* Copy bytes to the buffer while there is data available and space
         * to put it in the buffer
         */
        for ( ; otg_data_avail() && size--; count++)
                BREAK_IF((*buf++ = otg_get_byte()) == '\n');

        otg_pthread_mutex_unlock(&mesg_otg_instance->mutex);      /* unlock mutex */
        return count;
}

/*!
 * otg_data_queued() - return amount of data currently queued
 *
 * @return Amount of data currently queued in mesage ring buffer.
 */
int otg_data_queued(void)
{
        int count;
        otg_pthread_mutex_lock(&mesg_otg_instance->mutex);         /* lock mutex */
        count = otg_data_avail();
        otg_pthread_mutex_unlock(&mesg_otg_instance->mutex);      /* unlock mutex */
        return count;
}


/*!
 * otg_message_init() - initialize
 * @param otg OTG instance
 */
int otg_message_init(struct otg_instance *otg)
{
        //struct proc_dir_entry *message = NULL;
        otg_message_buf = otg_message_head = otg_message_tail = NULL;
        THROW_UNLESS((otg_message_buf = CKMALLOC(OTG_MESSAGE_BUF_SIZE + 16)), error);
        otg_message_head = otg_message_tail = otg_message_buf ;

        mesg_otg_instance = otg;
        CATCH(error) {
                #if defined(OTG_LINUX)
                #endif /* defined(OTG_LINUX) */
                #if defined(OTG_WINCE)
                DEBUGMSG(ZONE_INIT, (_T("otg_message_init: creating //proc//otg_message failed\n")) );
                #endif /* defined(OTG_LINUX) */
                return -EINVAL;
        }
        return 0;
}

/*!
 * otg_message_exit() - exit
 */
void otg_message_exit(void)
{
        mesg_otg_instance = NULL;
        if (otg_message_buf)
                LKFREE(otg_message_buf);
        otg_message_buf = otg_message_head = otg_message_tail = NULL;
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */

/*!
 * otg_mesg_get_status_update() - get status update
 * @param status_update
 */
void otg_mesg_get_status_update(struct otg_status_update *status_update)
{

        //TRACE_MSG0(CORE, "--");
        status_update->state = mesg_otg_instance->state;
        status_update->meta = mesg_otg_instance->current_outputs->meta;
        status_update->inputs = mesg_otg_instance->current_inputs;
        status_update->outputs = mesg_otg_instance->outputs;
        status_update->capabilities = 0;

        strncpy(status_update->fw_name, otg_firmware_loaded->fw_name, OTGADMIN_MAXSTR);
        strncpy(status_update->state_name, otg_get_state_name(mesg_otg_instance->state), OTGADMIN_MAXSTR);

        if (mesg_otg_instance->function_name)
                strncpy(status_update->function_name, mesg_otg_instance->function_name, OTGADMIN_MAXSTR);

        TRACE_MSG5(CORE, "state: %02x meta: %02x inputs: %08x outputs: %04x",
                        status_update->state, status_update->meta, status_update->inputs,
                        status_update->outputs, status_update->capabilities);
}


/*!
 * otg_mesg_get_firmware_info() - get firmware info
 * @param firmware_info
 */
void otg_mesg_get_firmware_info(struct otg_firmware_info *firmware_info)
{
        memset(firmware_info, 0, sizeof(firmware_info));
        // XXX irq lock

        firmware_info->number_of_states = otg_firmware_loaded->number_of_states;
        firmware_info->number_of_tests = otg_firmware_loaded->number_of_tests;
        memcpy(&firmware_info->fw_name, otg_firmware_loaded->fw_name, sizeof(firmware_info->fw_name));

}

/*!
 * otg_mesg_set_firmware_info() - set firmware info
 * @param firmware_info
 */
int otg_mesg_set_firmware_info(struct otg_firmware_info *firmware_info)
{

        if (otg_firmware_loading) {

                if ( (otg_firmware_loading->number_of_states == firmware_info->number_of_states) &&
                                (otg_firmware_loading->number_of_tests == firmware_info->number_of_tests)
                   ) {
                        TRACE_MSG0(CORE, "OTGADMIN_SET_INFO: loaded");
                        otg_firmware_loaded = otg_firmware_loading;
                        otg_firmware_loading = NULL;
                        return 0;
                }
                else {
                        TRACE_MSG0(CORE, "OTGADMIN_SET_INFO: abandoned");
                        LKFREE(otg_firmware_loading->otg_states);
                        LKFREE(otg_firmware_loading->otg_tests);
                        LKFREE(otg_firmware_loading);
                        return -EINVAL;
                }
        }

        TRACE_MSG2(CORE, "OTGADMIN_SET_INFO: otg_firmware_loaded: %x otg_firmware_orig: %x",
                        otg_firmware_loaded, otg_firmware_orig);
        if (otg_firmware_loaded && (otg_firmware_loaded != otg_firmware_orig)) {
                LKFREE(otg_firmware_loaded->otg_states);
                LKFREE(otg_firmware_loaded->otg_tests);
                LKFREE(otg_firmware_loaded);
                otg_firmware_loaded = otg_firmware_orig;
                TRACE_MSG0(CORE, "OTGADMIN_SET_INFO: unloaded");
        }
        if (firmware_info->number_of_states && firmware_info->number_of_tests) {
                int size;
                otg_firmware_loaded = NULL;
                TRACE_MSG0(CORE, "OTGADMIN_SET_INFO: setup");
                THROW_UNLESS((otg_firmware_loading = CKMALLOC(sizeof(struct otg_firmware))), error);
                size = sizeof(struct otg_state) * firmware_info->number_of_states;
                THROW_UNLESS((otg_firmware_loading->otg_states = CKMALLOC(size)), error);
                size = sizeof(struct otg_test) * firmware_info->number_of_tests;
                THROW_UNLESS((otg_firmware_loading->otg_tests = CKMALLOC(size)), error);

                otg_firmware_loading->number_of_states = firmware_info->number_of_states;
                otg_firmware_loading->number_of_tests = firmware_info->number_of_tests;
                memcpy(otg_firmware_loading->fw_name, &firmware_info->fw_name, sizeof(firmware_info->fw_name));

                CATCH(error) {
                        TRACE_MSG0(CORE, "OTGADMIN_SET_INFO: setup error");
                        if (otg_firmware_loading) {
                                if (otg_firmware_loading->otg_states) LKFREE(otg_firmware_loading->otg_states);
                                if (otg_firmware_loading->otg_tests) LKFREE(otg_firmware_loading->otg_tests);
                                LKFREE(otg_firmware_loading);
                                return -EINVAL;
                        }
                }

        }
        return 0;
}


/* ********************************************************************************************* */
/* ********************************************************************************************* */
