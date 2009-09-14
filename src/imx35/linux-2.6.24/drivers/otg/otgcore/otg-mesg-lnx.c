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
 * otg/otgcore/otg-mesg-l24.c - OTG state machine Administration
 * @(#) tt@belcarra.com|otg/otgcore/otg-mesg-lnx.c|20070316085010|26362
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies Corp
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 */
/*!
 * @file otg/otgcore/otg-mesg-lnx.c
 * @brief OTG Core Linux Message Interface.
 *
 * Implement linux char device via /proc/otg-mesg.
 *
 * @ingroup OTGMESG
 * @ingroup LINUXOS
 */

#include <otg/otg-compat.h>
#if defined(CONFIG_OTG_LNX) || defined(_OTG_DOXYGEN)

#include <otg/otg-module.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>
#include <otg/otg-trace.h>
#include <otg/otg-api.h>
//#include <otg/otg-task.h>
#include <otg/otg-tcd.h>
#include <otg/otg-hcd.h>
#include <otg/otg-pcd.h>
#include <otg/otg-ocd.h>

#include <linux/poll.h>
#include <linux/sched.h>

#if defined(LINUX24)
#include <asm/div64.h>
#endif


wait_queue_head_t otg_message_queue;


/*!
 *  otg_message_task() -  Bottom half handler to process sent or received urbs.
 *
 * @param data pointer to parameters struct
 */
#if 0
void otg_message_task(otg_task_arg_t data)
{
        struct otg_task * task = (struct otg_task *) data;

        struct otg_instance *otg = (struct otg_instance *)task->data;

        /* signal we have started */
        otg_up_admin(task);

        do {
                /* wait for work */
                TRACE_MSG0(CORE, "SLEEPING");
                otg_down_work(task);
                TRACE_MSG0(CORE, "WORKING");

                wake_up_interruptible(&otg_message_queue);

        } while (!task->terminate);


        /* signal exiting */
        task->terminated = TRUE;
        otg_up_admin(task);
}
#endif
void *otg_message_task(otg_task_arg_t data)
{
        struct otg_instance *otg = (struct otg_instance *)data;
        wake_up_interruptible(&otg_message_queue);
        return NULL;
}


#if 0
/*!
 * otg_message() - queue message data
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
                sprintf(time_buf, " %d uS\n", ticks);

        else if (ticks < 10000000) {
//                do_div(ticks, 1000);
		otg_do_div(ticks, 1000);
                sprintf(time_buf, " %d mS\n", ticks);
        }
        else {
//                do_div(ticks, 1000000);
		otg_do_div(ticks, 1000000);
                sprintf(time_buf, " %d S\n", ticks);
        }

        otg_write_message(otg, time_buf, strlen(time_buf));

        if (otg->message_task)
                otg_up_work(otg->message_task);
}
#endif

/*!
 * otg_message_irq() - queue message data
 * @param otg - otg instance pointer
 * @param buf message to send.
 */
void otg_message_irq(struct otg_instance *otg, char *buf)
{
        otg_message(otg, buf);
}

/*!
 * otg_message_block() - implement block
 */
unsigned int otg_message_block(void)
{
        int count = otg_data_queued();
        if (count) return count;
        interruptible_sleep_on(&otg_message_queue);
        return otg_data_queued();
}

/*!
 * otg_message_poll() - implement poll
 * @param filp file pointer
 * @param wait poll table structure
 */
unsigned int otg_message_poll(struct file *filp, struct poll_table_struct *wait)
{
        //TRACE_MSG0(CORE, "polling ");
        poll_wait(filp, &otg_message_queue, wait);
        return otg_data_queued() ? POLLIN | POLLRDNORM : 0;
}

/*!
 * otg_set_usbd_ops() - connect usbd operations to otg instance
 *
 * @param data - usbd_ops
 * @return 0 on finish
 */
struct usbd_ops *otg_usbd_ops;
int otg_set_usbd_ops(void *data)
{
        struct usbd_ops *usbd_ops = (struct usbd_ops *)data;
        otg_usbd_ops = usbd_ops;
        return 0;
}



/*!
 * otg_message_ioctl_internal() - ioctl call
 * @param cmd ioctl command.
 * @param arg ioctl arguement.
 * @return non-zero for error.
 */
int otg_message_ioctl_internal(unsigned int cmd, unsigned long arg)
{
        int i;
        int len;
        int flag;
        struct otg_admin_command admin_command;
        struct otg_status_update status_update;
        struct otg_firmware_info firmware_info;
        struct otg_state otg_state;
        struct otg_test otg_test;
        struct otg_ioctl_name *otg_ioctl_name;
        static char func_buf[32];
        char *sp, *dp;
        char *name;

        //TRACE_MSG2(CORE, "cmd: %08x %08x", cmd, _IOC_NR(cmd));

        switch (_IOC_DIR(cmd)) {
        case _IOC_NONE:
                switch (_IOC_NR(cmd)) {
                }
                break;

        case _IOC_WRITE:
                switch (_IOC_NR(cmd)) {
                case _IOC_NR(OTGADMIN_SET_FUNCTION):
                        TRACE_MSG0(CORE, "OTGADMIN_SET_FUNCTION");
                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        memset(&admin_command, 0x41, sizeof(struct otg_admin_command));
                        RETURN_EINVAL_IF(copy_from_user(&admin_command, (void *)arg, _IOC_SIZE(cmd)));
                        len = sizeof(mesg_otg_instance->function_name);
                        admin_command.string[len] = '\0';
                        TRACE_MSG1(CORE, "Setting function: \"%s\"", mesg_otg_instance->function_name);
                        strncpy(mesg_otg_instance->function_name, admin_command.string, len);
                        mesg_otg_instance->function_name[len-1] = '\0';
#if defined(LINUX26)
                        {
                                char *cp = mesg_otg_instance->function_name;
                                TRACE_MSG8(CORE, "function_name: %c%c%c%c%c%c%c%c",
                                                cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);

                        }
#endif
                        return 0;

                case _IOC_NR(OTGADMIN_SET_SERIAL):
                        TRACE_MSG0(CORE, "OTGADMIN_SET_SERIAL");
                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        RETURN_EINVAL_IF(copy_from_user(&admin_command, (void *)arg, _IOC_SIZE(cmd)));
                        admin_command.string[sizeof(admin_command.string) - 1] = '\0';
                        for (sp = admin_command.string, dp = mesg_otg_instance->serial_number, i = 0;
                                        *sp && (i < (sizeof(admin_command.string) - 1)); i++, sp++)
                                if (isxdigit(*sp)) *dp++ = toupper(*sp);

                        *sp = '\0';
                        //TRACE_MSG1(CORE, "serial_number: %s", mesg_otg_instance->serial_number);
                        return 0;

                case _IOC_NR(OTGADMIN_SET_INFO):
                        TRACE_MSG0(CORE, "OTGADMIN_SET_INFO");
                        memset(&firmware_info, 0x41, sizeof(firmware_info));
                        RETURN_EINVAL_IF(copy_from_user(&firmware_info, (void *)arg, _IOC_SIZE(cmd)));


                        return otg_mesg_set_firmware_info(&firmware_info);
                        return 0;

                case _IOC_NR(OTGADMIN_SET_STATE):
                        //TRACE_MSG0(CORE, "OTGADMIN_XXX_STATE");
                        RETURN_EINVAL_IF(copy_from_user(&otg_state, (void *)arg, _IOC_SIZE(cmd)));

                        RETURN_EINVAL_UNLESS(otg_firmware_loading);
                        //TRACE_MSG0(CORE, "OTGADMIN_SET_STATE");
                        RETURN_EINVAL_UNLESS (otg_state.state < otg_firmware_loading->number_of_states);
                        memcpy(otg_firmware_loading->otg_states + otg_state.state, &otg_state, sizeof(otg_state));
                        return 0;

                case _IOC_NR(OTGADMIN_SET_TEST):
                        //TRACE_MSG0(CORE, "OTGADMIN_GET/SET");
                        RETURN_EINVAL_IF(copy_from_user(&otg_test, (void *)arg, _IOC_SIZE(cmd)));

                        RETURN_EINVAL_UNLESS(otg_firmware_loading);
                        //TRACE_MSG1(CORE, "OTGADMIN_SET_TEST : %d", otg_test.test);
                        RETURN_EINVAL_UNLESS (otg_test.test < otg_firmware_loading->number_of_tests);
                        memcpy(otg_firmware_loading->otg_tests + otg_test.test, &otg_test, sizeof(otg_test));
                        return 0;
                }
                break;

        case _IOC_READ:
                switch (_IOC_NR(cmd)) {
                case _IOC_NR(OTGADMIN_GET_FUNCTION):
                        TRACE_MSG0(CORE, "OTGADMIN_GET_FUNCTION");
                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        RETURN_EINVAL_IF(copy_from_user(&admin_command, (void *)arg, _IOC_SIZE(cmd)));
                        name = otg_usbd_ops->function_name(admin_command.n);
                        admin_command.string[0] = '\0';
                        if (name)
                                strncat(admin_command.string, name, sizeof(admin_command.string));

                        RETURN_EINVAL_IF(copy_to_user((void *)arg, &admin_command, _IOC_SIZE(cmd)));
                        return 0;

                case _IOC_NR(OTGADMIN_GET_SERIAL):
                        TRACE_MSG0(CORE, "OTGADMIN_GET_SERIAL");
                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        strncpy(admin_command.string, mesg_otg_instance->serial_number, sizeof(admin_command.string));
                        admin_command.string[sizeof(admin_command.string) - 1] = '\0';
                        RETURN_EINVAL_IF(copy_to_user((void *)arg, &admin_command, _IOC_SIZE(cmd)));
                        return 0;

                case _IOC_NR(OTGADMIN_STATUS):
                        //TRACE_MSG0(CORE, "OTGADMIN_STATUS");
                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        memset(&status_update, 0, sizeof(struct otg_status_update));

                        otg_mesg_get_status_update(&status_update);

                        RETURN_EINVAL_IF(copy_to_user((void *)arg, &status_update, _IOC_SIZE(cmd)));
                        return 0;

                case _IOC_NR(OTGADMIN_GET_INFO):
                        TRACE_MSG0(CORE, "OTGADMIN_GET_INFO");
                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);

                        otg_mesg_get_firmware_info(&firmware_info);

                        RETURN_EINVAL_IF(copy_to_user((void *)arg, &firmware_info, _IOC_SIZE(cmd)));
                        //TRACE_MSG0(CORE, "OTGADMIN_GET_INFO: finished");
                        return 0;

                case _IOC_NR(OTGADMIN_GET_STATE):
                        //TRACE_MSG0(CORE, "OTGADMIN_XXX_STATE");
                        RETURN_EINVAL_IF(copy_from_user(&otg_state, (void *)arg, _IOC_SIZE(cmd)));

                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        //TRACE_MSG0(CORE, "OTGADMIN_GET_STATE");
                        RETURN_EINVAL_UNLESS (otg_state.state < otg_firmware_loaded->number_of_states);
                        memcpy(&otg_state, otg_firmware_loaded->otg_states + otg_state.state, sizeof(otg_state));
                        RETURN_EINVAL_IF(copy_to_user((void *)arg, &otg_state, _IOC_SIZE(cmd)));
                        return 0;

                case _IOC_NR(OTGADMIN_GET_TEST):
                        //TRACE_MSG0(CORE, "OTGADMIN_GET/SET");
                        RETURN_EINVAL_IF(copy_from_user(&otg_test, (void *)arg, _IOC_SIZE(cmd)));

                        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
                        //TRACE_MSG1(CORE, "OTGADMIN_GET_TEST: %d", otg_test.test);
                        RETURN_EINVAL_UNLESS (otg_test.test < otg_firmware_loaded->number_of_tests);
                        memcpy(&otg_test, otg_firmware_loaded->otg_tests + otg_test.test, sizeof(otg_test));
                        RETURN_EINVAL_IF(copy_to_user((void *)arg, &otg_test, _IOC_SIZE(cmd)));
                        return 0;
                }
                break;
        }

        TRACE_MSG0(CORE, "OTGADMIN_");
        RETURN_EINVAL_UNLESS(otg_firmware_loaded);
        for (otg_ioctl_name = otg_ioctl_names; otg_ioctl_name && otg_ioctl_name->cmd; otg_ioctl_name++) {
                //TRACE_MSG4(CORE, "lookup: %04x %04x %08x %08x",
                //                _IOC_NR(cmd), _IOC_NR(otg_ioctl_name->cmd), cmd, otg_ioctl_name->cmd);
                BREAK_IF(_IOC_NR(otg_ioctl_name->cmd) == _IOC_NR(cmd));
        }
        TRACE_MSG3(CORE, "checking %d %08x %08x", _IOC_NR(cmd), otg_ioctl_name->cmd, cmd);
        RETURN_EINVAL_UNLESS(otg_ioctl_name->cmd);
        __get_user(flag, (int *)arg);
        TRACE_MSG3(CORE, "%s %08x flag: %d", otg_ioctl_name->name, otg_ioctl_name->set, flag);
        otg_event (mesg_otg_instance, flag ? otg_ioctl_name->set : _NOT(otg_ioctl_name->set), CORE, otg_ioctl_name->name);
        return 0;
}



/*!
 * otg_message_ioctl() - ioctl
 * @param inode inode structure.
 * @param filp file.
 * @param cmd ioctl command.
 * @param arg ioctl argument.
 * @return non-zero for error.
 */
int otg_message_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
        int i;
        int len;
        int flag;
        struct otg_admin_command admin_command;
        struct otg_status_update status_update;
        struct otg_firmware_info firmware_info;
        struct otg_state otg_state;
        struct otg_test otg_test;
        //struct otg_ioctl_map *map = ioctl_map;
        struct otg_ioctl_name *otg_ioctl_name;

        static char func_buf[32];

        //TRACE_MSG6(CORE, "cmd: %08x arg: %08x type: %02d nr: %02d dir: %02d size: %02d",
        //                cmd, arg, _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_DIR(cmd), _IOC_SIZE(cmd));

        RETURN_EINVAL_UNLESS (_IOC_TYPE(cmd) == OTGADMIN_MAGIC);
        RETURN_EINVAL_UNLESS (_IOC_NR(cmd) <= OTGADMIN_MAXNR);

        RETURN_EFAULT_IF((_IOC_DIR(cmd) == _IOC_READ) && !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd)));
        RETURN_EFAULT_IF((_IOC_DIR(cmd) == _IOC_WRITE) && !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd)));

        return otg_message_ioctl_internal(cmd, arg);
}


/*!
 * otg_message_proc_read() - implement proc file system read.
 * Standard proc file system read function.
 * @param file file.
 * @param buf buffer to fill
 * @param count size of buffer
 * @param pos file position
 * @return number of bytes or negative number for error
 */
static ssize_t otg_message_proc_read (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0, avail;
        char *bp,*ep;
        struct list_head *lhd;
        int state;

        // get a page, max 4095 bytes of data...
        RETURN_ENOMEM_UNLESS ((page = GET_KERNEL_PAGE()));

        bp = (char *) page;

        len = bp - (char *) page;

        if (( avail = otg_message_block())) {

                len += otg_read_message(bp, 4095);

                if (len > count)
                        len = -EINVAL;
                else if ((len > 0) && copy_to_user (buf, (char *) page, len))
                        len = -EFAULT;

        }

        free_page (page);
        return len;
}

/*! otg_message_proc_ioctl() -
 * @param inode inode structure.
 * @param filp file.
 * @param cmd ioctl command.
 * @param arg ioctl argument.
 * @return non-zero for error.
 */
int otg_message_proc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
        return otg_message_ioctl(inode, filp, cmd, arg);
}

/*!
 * @param filp file.
 * @param wait buffer to fill
 * @return number of bytes or negative number for error
 */
unsigned int otg_message_proc_poll(struct file *filp, struct poll_table_struct *wait)
{
        return otg_message_poll(filp, wait);
}

/*!
 * @var otg_message_proc_switch_functions
 */
static struct file_operations otg_message_proc_switch_functions = {
        ioctl:otg_message_proc_ioctl,
        poll:otg_message_proc_poll,
        read:otg_message_proc_read,
};


/*!
 * otg_message_init_l24() - initialize
 * @param otg - otg instance pointer
 */
int otg_message_init_l24(struct otg_instance *otg)
{
        struct proc_dir_entry *message = NULL;
        init_waitqueue_head(&otg_message_queue);
        RETURN_EINVAL_UNLESS((otg->message_task = otg_task_init2("otgmessage", otg_message_task, (void *) otg, CORE)));

        //otg->message_task->debug = TRUE;

        otg_task_start(otg->message_task);

        THROW_IF (!(message = create_proc_entry ("otg_message", 0666, 0)), error);
        message->proc_fops = &otg_message_proc_switch_functions;
        CATCH(error) {
                if (message)
                        remove_proc_entry("otg_message", NULL);
                if (otg->message_task) {
                        otg_task_exit(otg->message_task);
                        otg->message_task = NULL;
                }

                return -EINVAL;

        }
        return 0;
}

/*!
 * otg_message_exit_l24() - exit
 * @param otg - otg instance pointer
 */
void otg_message_exit_l24(struct otg_instance *otg)
{
        remove_proc_entry("otg_message", NULL);
        otg_task_exit(otg->message_task);
        otg->message_task = NULL;
}

OTG_EXPORT_SYMBOL(otg_message);

#endif /* defined(CONFIG_OTG_LNX) */
