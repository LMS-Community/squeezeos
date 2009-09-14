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

/*!
 * @file plat-mxc/dsp_bringup.c
 *
 * @brief This files contains functions for DSP operations.
 *
 * @ingroup MSL_MXC91321
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/setup.h>
#include <asm/arch/hardware.h>

#include <asm/arch/dsp_bringup.h>

#define MCU_MTR0_REGISTER       IO_ADDRESS(MU_BASE_ADDR + 0x0)
#define MCU_MTR1_REGISTER       IO_ADDRESS(MU_BASE_ADDR + 0x4)
#define MCU_MTR2_REGISTER       IO_ADDRESS(MU_BASE_ADDR + 0x8)
#define MCU_MRR0_REGISTER       IO_ADDRESS(MU_BASE_ADDR + 0x10)
#define MCU_MSR_REGISTER        IO_ADDRESS(MU_BASE_ADDR + 0x20)
#define MCU_MCR_REGISTER        IO_ADDRESS(MU_BASE_ADDR + 0x24)

/*
 * Control register to reset and to disable security mode on DSP core.
 */
#define REG_SPBA_INIT_IIM        IO_ADDRESS(SPBA_CTRL_BASE_ADDR + 0x1C)
#define REG_DSP_HAB_TYPE         IO_ADDRESS(IIM_BASE_ADDR + 0x814)

/*
 * Definition of parameter passed to kernel to decide
 * if an image should be started on StarCore
 */
#define DSP_BOOT                 "dspboot"
#define DSP_BOOT_ON              "on"
#define DSP_BOOT_OFF             "off"

#define WRITE_REQUEST            1
#define WRITE_RESPONSE           1

#define READ_REQUEST             2
#define READ_RESPONSE            2

#define MEM_BLOCK_REQUEST        3
#define MEM_BLOCK_RESPONSE       3

#define STARTAPP_REQUEST         4
#define STARTAPP_RESPONSE        4

#define BAD_OPCODE_RESPONSE      5

/*
 * This variable holds address of MSR register. We need this register to
 * issue a DSP reset and to disable security mode.
 */
volatile unsigned long *mcu_msr_register =
    (volatile unsigned long *)MCU_MSR_REGISTER;
volatile unsigned long *mcu_mcr_register =
    (volatile unsigned long *)MCU_MCR_REGISTER;
volatile unsigned long *mcu_mtr0_register =
    (volatile unsigned long *)MCU_MTR0_REGISTER;
volatile unsigned long *mcu_mtr1_register =
    (volatile unsigned long *)MCU_MTR1_REGISTER;
volatile unsigned long *mcu_mtr2_register =
    (volatile unsigned long *)MCU_MTR2_REGISTER;
volatile unsigned long *mcu_mrr0_register =
    (volatile unsigned long *)MCU_MRR0_REGISTER;

volatile unsigned long *spba_iim_register =
    (volatile unsigned long *)REG_SPBA_INIT_IIM;
volatile unsigned long *iim_hab_register =
    (volatile unsigned long *)REG_DSP_HAB_TYPE;

/*
 * This variable holds whether we have received a "dspboot=on" or not
 */
static int dsp_boot_on = 0;

 /*!
  * This function issues a DSP hardware reset. It executes
  * three steps:
  *
  *  1) Sets DSP Hardware Reset bit into MU module - MCR register
  *  2) De-asserts DSP Hardware Reset bit
  *  3) Executes an active wait until DSP is out of reset state.
  *
  * @return          No return value
  */
void reset_dsp(void)
{
	*mcu_mcr_register |= 0x10;
	*mcu_mcr_register &= 0xFFFFFFEF;

	while (*mcu_msr_register & 0x80) ;
}

/*!
 *  Disables security mode for DSP
 *
 *  @return   No return value
 */
void disable_security_mode(void)
{
	/*Disable DSP_HAB_TYPE security mode */
	*spba_iim_register = 0x00000007;
	*iim_hab_register = 0x00000004;
}

/*!
 * parse_args system function passes kernel's parsed arguments
 * to do_dsp_params in order to verify whether dspboot parameter
 * has been passed or not.
 *
 * @return     0 if parameter is not "dspboot"
 *             0 if "dspboot" parameter has been found and
 *             its value is equal to "on"
 *             0 if "dspboot" parameter has been found and
 *             its value is equal to "off"
 */
static int __init do_dsp_params(char *param, char *val)
{

	if (strcmp(param, DSP_BOOT) == 0) {
		printk
		    ("STARCORE: dspboot parameter present. Go to try to start DSP!\n");
		if (strcmp(val, DSP_BOOT_ON) == 0)
			dsp_boot_on = 1;
		else
			printk
			    ("STARCORE: dspboot parameter present, but option is set to \"off\"\n");
	}

	return 0;
}

/*!
 * This function search for 'dspboot' in kernel's command line
 * options and checks its value. if value is 'on', a startapp_request
 * will be sent to DSP core in order to start an image. Otherwise
 * none request is sent
 *
 * @return      Returns 1 if dspboot option = 'on'
 *              Returns 0 if dspboot option = 'off'
 *              Returns 0 if dspboot option does not exist
 */
int __init dsp_parse_cmdline(const char *cmdline)
{
	char tmp_cmdline[COMMAND_LINE_SIZE];

	dsp_boot_on = 0;

	strlcpy(tmp_cmdline, cmdline, COMMAND_LINE_SIZE);
	parse_args("DSP bringup arguments", tmp_cmdline, NULL, 0,
		   do_dsp_params);
	return dsp_boot_on;
}

/*!
 * this function sends a jump vector to DSP
 * in order to resume execution of a previously
 * downloaded executable image.
 *
 * @return      Returns -1 if it is not possible to allocate
 *              channel 0 or channel 1, both required by boot
 *              protocol to issue a start_application request.
 *              Returns request's result otherwise.
 */
void dsp_startapp_request(void)
{
	unsigned long *virtual_addr = NULL;
	unsigned long physical_addr = 0;

	/* disable security mode */
	disable_security_mode();

	/*execute a dsp hardware reset */
	reset_dsp();

	/* Get a chunk of virtual memory to hold DSP image start address */
	virtual_addr = (unsigned long *)kmalloc(sizeof(unsigned long),
						GFP_KERNEL);
	*virtual_addr = IMAGE_START_ADDRESS;

	physical_addr = __pa((volatile void *)virtual_addr);

	/* Send a start application request to the DSP */
	*mcu_mtr0_register = STARTAPP_REQUEST;

	/* Send the start physical address to the DSP */
	*mcu_mtr1_register = physical_addr;

	/* Wait for the DSP response */
	while (!(*mcu_msr_register & 0x08000000)) ;

	/*check if DSP has acknowledged our request */
	if ((*mcu_mrr0_register & 0x000000FF) != STARTAPP_RESPONSE)
		printk
		    ("STARCORE: DSP core DOES NOT acknowledged STARTAPP request!!\n");
	else
		printk("STARCORE: DSP core acknowledged STARTAPP request\n");

	kfree(virtual_addr);
}

int dsp_memwrite_request(unsigned long addr, unsigned long word)
{
	int res = 0;

	/* Send a write application request to the DSP */
	*mcu_mtr0_register = 0x00010001;

	/* Send the write address to the DSP */
	*mcu_mtr1_register = addr;

	/* Send data to write to StarCore */
	*mcu_mtr2_register = word;

	/* Wait for the DSP response */
	while (!(*mcu_msr_register & 0x08000000)) ;

	/*return response of DSP */
	res = (int)(*mcu_mrr0_register & 0x000000FF);
	return res;
}
