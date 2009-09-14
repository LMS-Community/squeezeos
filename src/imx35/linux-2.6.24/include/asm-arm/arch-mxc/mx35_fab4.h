/*
 * Copyright 2008 Logitech. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct mxc_fab4_gpio_pins {
	int gpio;
	char *name;
	struct device_attribute *attr;
};

struct mxc_fab4_gpio_platform_data {
	int nr_pins;
	struct mxc_fab4_gpio_pins *pins;
};
