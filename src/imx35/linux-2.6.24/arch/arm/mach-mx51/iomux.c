/*
 * Copyright 2008 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @defgroup GPIO_MX51 Board GPIO and Muxing Setup
 * @ingroup MSL_MX51
 */
/*!
 * @file mach-mx51/iomux.c
 *
 * @brief I/O Muxing control functions
 *
 * @ingroup GPIO_MX51
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>
#include "iomux.h"

/*!
 * IOMUX register (base) addresses
 */
enum iomux_reg_addr {
	IOMUXGPR0 = IO_ADDRESS(IOMUXC_BASE_ADDR),
	IOMUXGPR1 = IO_ADDRESS(IOMUXC_BASE_ADDR) + 0x004,
	IOMUXSW_MUX_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + MUX_I_START,
	IOMUXSW_MUX_END = IO_ADDRESS(IOMUXC_BASE_ADDR) + MUX_I_END,
	IOMUXSW_PAD_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + PAD_I_START,
	IOMUXSW_PAD_END = IO_ADDRESS(IOMUXC_BASE_ADDR) + PAD_I_END,
	IOMUXSW_INPUT_CTL = IO_ADDRESS(IOMUXC_BASE_ADDR) + INPUT_CTL_START,
	IOMUXSW_INPUT_END = IO_ADDRESS(IOMUXC_BASE_ADDR) + INPUT_CTL_END,
};

#define MUX_PIN_NUM_MAX	(((IOMUXSW_MUX_END - IOMUXSW_MUX_CTL) >> 2) + 1)
#define MUX_INPUT_NUM_MUX	(((IOMUXSW_INPUT_END - IOMUXSW_INPUT_CTL)>> \
				    2) + 1)

static u8 iomux_pin_res_table[MUX_PIN_NUM_MAX];
static DEFINE_SPINLOCK(gpio_mux_lock);

/*!
 * This function is used to configure a pin through the IOMUX module.
 * @param  pin		a pin number as defined in \b #iomux_pin_name_t
 * @param  config	a configuration as defined in \b #iomux_pin_cfg_t
 *
 * @return 		0 if successful; Non-zero otherwise
 */
static int iomux_config_mux(iomux_pin_name_t pin, iomux_pin_cfg_t config)
{
	u32 ret = 0;
	u32 pin_index = PIN_TO_IOMUX_INDEX(pin);
	u32 mux_reg = IOMUXSW_MUX_CTL + PIN_TO_IOMUX_MUX(pin);
	u32 mux_data = 0;
	u8 *rp;

	BUG_ON((mux_reg > IOMUXSW_MUX_END) || (mux_reg < IOMUXSW_MUX_CTL));
	spin_lock(&gpio_mux_lock);

	if (config == IOMUX_CONFIG_GPIO)
		mux_data = PIN_TO_ALT_GPIO(pin);
	else
		mux_data = config;

	__raw_writel(mux_data, mux_reg);

	/*
	 * Log a warning if a pin changes ownership
	 */
	rp = iomux_pin_res_table + pin_index;
	if ((mux_data & *rp) && (*rp != mux_data)) {
		/*
		 * Don't call printk if we're tweaking the console uart or
		 * we'll deadlock.
		 */
		printk(KERN_ERR "iomux_config_mux: Warning: iomux pin"
		       " config changed, pin=%d, "
		       " prev=0x%x new=0x%x\n", mux_reg, *rp, mux_data);
		ret = -EINVAL;
	}
	*rp = mux_data;
	spin_unlock(&gpio_mux_lock);
	return ret;
}

/*!
 * Request ownership for an IO pin. This function has to be the first one
 * being called before that pin is used. The caller has to check the
 * return value to make sure it returns 0.
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  config	a configuration as defined in \b #iomux_pin_cfg_t
 *
 * @return		0 if successful; Non-zero otherwise
 */
int mxc_request_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t config)
{
	int ret = iomux_config_mux(pin, config);
	int gpio_port = GPIO_TO_PORT(IOMUX_TO_GPIO(pin));

	if (!ret && (gpio_port != NON_GPIO_PORT)
	    && ((config == IOMUX_CONFIG_GPIO)
		|| (config == PIN_TO_ALT_GPIO(pin))))
		ret |= mxc_request_gpio(pin);

	return ret;
}
EXPORT_SYMBOL(mxc_request_iomux);

/*!
 * Release ownership for an IO pin
 *
 * @param  pin		a name defined by \b iomux_pin_name_t
 * @param  config	config as defined in \b #iomux_pin_ocfg_t
 */
void mxc_free_iomux(iomux_pin_name_t pin, iomux_pin_cfg_t config)
{
	u32 pin_index = PIN_TO_IOMUX_INDEX(pin);
	u8 *rp = iomux_pin_res_table + pin_index;
	int gpio_port = GPIO_TO_PORT(IOMUX_TO_GPIO(pin));

	BUG_ON(pin_index > MUX_PIN_NUM_MAX);
	*rp = 0;
	if ((gpio_port != NON_GPIO_PORT)
	    && ((config == IOMUX_CONFIG_GPIO)
		|| (config == PIN_TO_ALT_GPIO(pin))))
		mxc_free_gpio(pin);

}
EXPORT_SYMBOL(mxc_free_iomux);

/*!
 * This function configures the pad value for a IOMUX pin.
 *
 * @param  pin          a pin number as defined in \b #iomux_pin_name_t
 * @param  config       the ORed value of elements defined in \b #iomux_pad_config_t
 */
void mxc_iomux_set_pad(iomux_pin_name_t pin, u32 config)
{
	u32 pad_reg = IOMUXSW_PAD_CTL + PIN_TO_IOMUX_PAD(pin);

	BUG_ON((pad_reg > IOMUXSW_PAD_END) || (pad_reg < IOMUXSW_PAD_CTL));
	spin_lock(&gpio_mux_lock);
	__raw_writel(config, pad_reg);
	spin_unlock(&gpio_mux_lock);
}
EXPORT_SYMBOL(mxc_iomux_set_pad);

unsigned int mxc_iomux_get_pad(iomux_pin_name_t pin)
{
	u32 pad_reg = IOMUXSW_PAD_CTL + PIN_TO_IOMUX_PAD(pin);
	u32 val;

	spin_lock(&gpio_mux_lock);
	val = __raw_readl(pad_reg);
	spin_unlock(&gpio_mux_lock);
	return val;
}
EXPORT_SYMBOL(mxc_iomux_get_pad);

/*!
 * This function configures input path.
 *
 * @param  input        index of input select register as defined in \b #iomux_input_select_t
 * @param  config       the binary value of elements defined in \b #iomux_input_config_t
 *      */
void mxc_iomux_set_input(iomux_input_select_t input, u32 config)
{
	u32 reg = IOMUXSW_INPUT_CTL + (input << 2);

	BUG_ON(input >= MUX_INPUT_NUM_MUX);
	__raw_writel(config, reg);
}
EXPORT_SYMBOL(mxc_iomux_set_input);
