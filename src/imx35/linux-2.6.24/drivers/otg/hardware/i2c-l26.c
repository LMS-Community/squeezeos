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
 * otg/hardware/i2c-l26.c -- Linux 2.6 I2C access
 * @(#) balden@belcarra.com/seth2.rillanon.org|otg/platform/otglib/i2c-l26.c|20070614183950|36713
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
 * @file otg/hardware/i2c-l26.c
 * @brief Linux I2C I/O via generic i2c device.
 *
 * Writes are queued and performed in a bottom half handler.
 *
 * @ingroup ISP1301TCD
 * @ingroup LINUXOS
 */

#include <otg/otg-compat.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/i2c.h>

#include <otg/pcd-include.h>
#include <linux/pci.h>
#include "isp1301-hardware.h"
#include "isp1301.h"

/* ********************************************************************************************* */

/*
 * N.B. i2c functions must not be called from interrupt handlers
 */

static struct file *i2c_file;
static struct i2c_client *i2c_client;
static int initstate_i2c;
static int initstate_region;
#define MAX_I2C 16

/*! i2c_configure
 * Attempt to find and open generic i2c device
 * @param name - i2c device name
 * @param addr - address used to configure i2c
 * @return - non-zero for failure
 */
int  i2c_configure(char *name, int addr)
{
        char filename[20];
        struct i2c_adapter *ad;
        int tmp;

        RETURN_ZERO_IF(initstate_i2c);

#if 1
        for (tmp=0 ; tmp<MAX_I2C; tmp++){
                ad = i2c_get_adapter(tmp);
                if (ad){
//	                printk(KERN_INFO"SHP - for tmp = %d name = %s\n", tmp, ad->name);
                        if (!strncmp(ad->name, name, strlen(name)))
                                break;
                }// valid driver
        }
        if (tmp == MAX_I2C) {                           // Nothing found
                printk(KERN_ERR"%s: cannot find I2C driver", __FUNCTION__);
                return -ENODEV;
        }
//	printk(KERN_ERR"%s: kmalloc(sizeof(*i2c_client) ", __FUNCTION__);
        i2c_client = kmalloc(sizeof(*i2c_client), GFP_KERNEL);
        i2c_client->adapter = (struct i2c_adapter *) ad;
        //        printk(KERN_INFO"i2c_client name = %s\n", i2c_client->adapter->name);
#endif

#if 0
        /*find the I2C driver we need
         */
        for (tmp = 0; tmp < MAX_I2C; tmp++) {

                sprintf(filename, "/dev/i2c/%d", tmp);

                printk(KERN_INFO"%s: %s\n", __FUNCTION__, filename);

                UNLESS (IS_ERR(i2c_file = filp_open(filename, O_RDWR, 0))) {

                        //printk(KERN_INFO"%s: %s found\n", __FUNCTION__, filename);

                        /*found some driver */
                        i2c_client = (struct i2c_client *)i2c_file->private_data;

                        printk(KERN_INFO"%s: \"%s\" found \"%s\"\n", __FUNCTION__, name, i2c_client->adapter->name);
                        if (strlen(i2c_client->adapter->name) >= 8) {
                                if (!strncmp(i2c_client->adapter->name, name, strlen(name)))
                                        break;  /*we found our driver! */
                        }
                        i2c_client = NULL;
                        filp_close(i2c_file, NULL);
                }
                printk(KERN_INFO"%s: %s %d\n", __FUNCTION__, filename, i2c_file);
        }
        if (tmp == MAX_I2C) {                           // Nothing found
                printk(KERN_ERR"%s: cannot find I2C driver", __FUNCTION__);
                return -ENODEV;
        }
#endif

        i2c_client->addr = addr;
        initstate_i2c = 1;
        return 0;
}



/*! i2c_close
 * Close i2c device fd.
 */
void  i2c_close(void)
{
#if 1
        printk(KERN_ERR"%s: Free i2c_client \n", __FUNCTION__);
        kfree(i2c_client);
#endif
#if 0
        if (initstate_i2c)
                filp_close(i2c_file, NULL);
#endif
        initstate_i2c = 0;
}

/*! i2c_readb
 * Read byte from i2c device
 * @param subaddr - address to read value
 * @return read byte value
 */
u8 i2c_readb(u8 subaddr)
{
        u8 buf = 0;
        i2c_master_send(i2c_client, &subaddr, 1);
        i2c_master_recv(i2c_client, &buf, 1);
        //TRACE_MSG2(TCD, "addr: %02x buf:  %02x", subaddr, buf);
        return buf;
}

/*! i2c_readw -
 * Read word from i2c device
 * @param subaddr - address to fetch value
 * @return fetched word value
 */
u16 i2c_readw(u8 subaddr)
{
        u16 buf = 0;
        i2c_master_send(i2c_client, &subaddr, 1);
        i2c_master_recv(i2c_client, (u8 *)&buf, 2);
        //TRACE_MSG2(TCD, "addr: %02x buf:  %04x", subaddr, buf);
        return buf;
}

/*! i2c_readl -
 * Read long from i2c device
 * @param subaddr - address to fetch value
 * @return fetched value
 */
u32 i2c_readl(u8 subaddr)
{
        u32 buf = 0;
        i2c_master_send(i2c_client, &subaddr, 1);
        i2c_master_recv(i2c_client, (u8 *)&buf, 4);
        //TRACE_MSG2(TCD, "addr: %02x buf:  %08x", subaddr, buf);
        return buf;
}



/* ********************************************************************************************* */
/*! @var struct otg_task i2c_io_task
 * @brief - an otg_task intance to process i2c_io operations
 */
struct otg_task *i2c_io_task;

#define I2C_MAX_WRITE           500

int i2c_Write_Queued;
u8 i2c_Write_Data[I2C_MAX_WRITE];
u8 i2c_Write_Addr[I2C_MAX_WRITE];

/*! i2c_writeb_direct() - internal
 * Writw byte to i2c device
 * @param subaddr
 * @param buf
 */
void i2c_writeb_direct(u8 subaddr, u8 buf)
{
        char tmpbuf[2];

        tmpbuf[0] = subaddr;    /*register number */
        tmpbuf[1] = buf;        /*register data */
        i2c_master_send(i2c_client, &tmpbuf[0], 2);
}

/*!
 * i2c_io_bh() - bottom half handler to use i2c_write on queued data
 * i2c_write operations are queued so that they can be done in this bottom
 * half handler.
 *
 * XXX the memcpy's could be eliminated with head/tail pointers.
 *
 * @param data
 */
void *i2c_io_bh(otg_task_arg_t data)
{
        while (i2c_Write_Queued > 0) {
                u8 write = i2c_Write_Data[0];
                u8 port = i2c_Write_Addr[0];
                unsigned long flags;                                    // atomic update of counter and saved values
                local_irq_save (flags);
                //printk(KERN_INFO"%s: i2c_Write_Queued: %d\n", __FUNCTION__, i2c_Write_Queued);
                i2c_Write_Queued--;
                memcpy(i2c_Write_Data, i2c_Write_Data + 1, sizeof(i2c_Write_Data) - 1);
                memcpy(i2c_Write_Addr, i2c_Write_Addr + 1, sizeof(i2c_Write_Addr) - 4);
                local_irq_restore (flags);

                i2c_writeb_direct(port, write);                                 // perform write

                //TRACE_MSG3(TCD,"port: %02x data: %02x result: %02x", port, write, i2c_readb(port & 0xfe));
        }
        return NULL;
}


/*!
 * i2c_writeb() - queue write operation
 * @param port
 * @param data byte to write
 */
void i2c_writeb(u8 port, u8 data)
{
        unsigned long flags;                                    // atomic update of counter and saved values
        RETURN_IF(i2c_Write_Queued < 0);                                // check if terminating
        if ((i2c_Write_Queued == I2C_MAX_WRITE)) {

                printk(KERN_ERR"TOO MANY QUEUED CANNOT WRITE port: %02x data: %02x active: %d\n", port, data, i2c_Write_Queued);
                //TRACE_MSG3(TCD, "TOO MANY QUEUED CANNOT WRITE port: %02x data: %02x active: %d", port, data, i2c_Write_Queued);
                return;
        }
        local_irq_save (flags);
        i2c_Write_Addr[i2c_Write_Queued] = port;
        i2c_Write_Data[i2c_Write_Queued] = data;
        i2c_Write_Queued++;
        //printk(KERN_INFO"%s: i2c_Write_Queued: %d\n", __FUNCTION__, i2c_Write_Queued);
        local_irq_restore (flags);
        //TRACE_MSG3(TCD, "port: %02x data: %02x active: %d", port, data, i2c_Write_Queued);
        //SCHEDULE_WORK(i2c_io_work_struct);
        if (i2c_io_task) otg_up_work(i2c_io_task);
}


int i2c_mod_init(struct otg_instance *otg)
{
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        //PREPARE_WORK_ITEM(i2c_io_work_struct, &i2c_io_bh, NULL);
        TRACE_MSG0(otg->tcd->TAG, "INIT");
        RETURN_EINVAL_UNLESS((i2c_io_task = otg_task_init2("otgi2c", i2c_io_bh, NULL, otg->tcd->TAG)));
        //i2c_io_task->debug = TRUE;
        otg_task_start(i2c_io_task);
        return 0;
}

void i2c_mod_exit(struct otg_instance *otg)
{
        otg_task_exit(i2c_io_task);
        i2c_io_task = NULL;

        TRACE_MSG0(otg->tcd->TAG, "EXIT");
        //while (PENDING_WORK_ITEM(i2c_io_work_struct) /*|| PENDING_WORK_ITEM(i2c_xcvr_work_struct)*/ ) {
        //        printk(KERN_ERR"%s: waiting for bh\n", __FUNCTION__);
        //        schedule_timeout(10 * HZ);
        //}
}
