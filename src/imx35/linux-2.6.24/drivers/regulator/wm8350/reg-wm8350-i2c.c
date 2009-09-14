#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/leds.h>
#include <linux/fb.h>
#include <linux/regulator/regulator.h>
#include <linux/regulator/wm8350/wm8350-bus.h>

#define WM8350_I2C_ADDR         (0x34 >> 1)

extern void wm8350_free(struct wm8350 *wm8350);
extern int wm8350_init(struct wm8350* wm8350);


static int wm8350_i2c_detach(struct i2c_client *client);

void wm8350_irq_work(struct work_struct *work)
{
	wm8350_irq_worker(work);
}

irqreturn_t wm8350_irq_handler(int irq, void *data)
{
	struct wm8350 *wm8350 = (struct wm8350*)data;
	schedule_work(&wm8350->work);
	return IRQ_HANDLED;
}

/*
 * WM8350 2 wire address
 */
static unsigned short normal_i2c[] = { WM8350_I2C_ADDR, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

extern int wm8350_pmu_init(void);
extern void wm8350_pmu_exit(void);

static int wm8350_pmic_i2c_detect(struct i2c_adapter *adap, int addr, int kind)
{
	struct wm8350* wm8350;
	struct i2c_client *i2c;
	int ret = 0;

	if (addr != WM8350_I2C_ADDR)
		return -ENODEV;

	i2c = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (i2c == NULL)
		return -ENOMEM;

	wm8350 = kzalloc(sizeof(struct wm8350), GFP_KERNEL);
	if (wm8350 == NULL) {
		kfree(i2c);
		return -ENOMEM;
	}

	i2c->addr = addr;
	i2c->adapter = adap;

	mutex_init(&wm8350->work_mutex);
	i2c_set_clientdata(i2c, wm8350);
	wm8350->i2c_client = i2c;
	wm8350_set_io(wm8350, WM8350_IO_I2C, NULL, NULL);

	ret = i2c_attach_client(i2c);
	if (ret < 0) {
		printk(KERN_ERR "wm8350: failed to attach device at addr 0x%x\n", addr);
		goto err;
	}

	ret = wm8350_create_cache(wm8350);
	if (ret < 0) {
		printk(KERN_ERR "wm8350: failed to create register cache\n");
		goto err;
	}

	if (wm8350_reg_read(wm8350, 0) == 0x0)
		printk("wm8350: found Rev C device\n");
	else if (wm8350_reg_read(wm8350, 0) == 0x6143)
		printk("wm8350: found Rev E device\n");
	else {
		printk(KERN_ERR "wm8350: device is not a WM8350\n");
		ret = -ENODEV;
		goto err;
	}

	ret = bus_register(&wm8350_bus_type);
	if (ret < 0)
		goto err;

	wm8350_pmu_init();

	ret = wm8350_init(wm8350);
	if (ret == 0)
		return ret;

err:
	wm8350_i2c_detach(i2c);
	return ret;
}

static int wm8350_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, wm8350_pmic_i2c_detect);
}

static int wm8350_i2c_detach(struct i2c_client *client)
{
	struct wm8350 *wm8350 = i2c_get_clientdata(client);

	wm8350_free(wm8350);
	wm8350_pmu_exit();
	bus_unregister(&wm8350_bus_type);
	i2c_detach_client(client);
	kfree(client);
	if (wm8350->reg_cache)
		kfree(wm8350->reg_cache);
	kfree(wm8350);

	return 0;
}

#ifdef CONFIG_PM
static int wm8350_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	int ret = 0;
	return ret;
}

static int wm8350_i2c_resume(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

#else
#define wm8350_i2c_suspend	NULL
#define wm8350_i2c_resume	NULL
#endif

static struct i2c_driver wm8350_i2c_driver = {
	.driver = {
		.name = "WM8350",
	},
	.attach_adapter = wm8350_i2c_attach,
	.detach_client =  wm8350_i2c_detach,
	.suspend	= wm8350_i2c_suspend,
	.resume		= wm8350_i2c_resume,
	.command =        NULL,
};


static int __init wm8350_pmic_init(void)
{
	return i2c_add_driver(&wm8350_i2c_driver);
}

static void __exit wm8350_pmic_exit(void)
{
	i2c_del_driver(&wm8350_i2c_driver);
}

subsys_initcall(wm8350_pmic_init);
module_exit(wm8350_pmic_exit);

MODULE_AUTHOR("Liam Girdwood");
MODULE_DESCRIPTION("PMIC WM8350 Driver");
MODULE_LICENSE("GPL");

