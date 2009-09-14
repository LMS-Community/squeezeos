/*
 * regulator.h -- SoC Regulator support.
 *
 * Copyright (C) 2007 Wolfson Microelectronics PLC.
 *
 * Author: Liam Girdwood <lg@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Regulator Client Interface.
 *
 * A Power Management Regulator framework for SoC based devices.
 * Features:-
 *   o Voltage and current level control.
 *   o Operating mode control.
 *   o Regulator status.
 *   o sysfs entries for showing client devices and status
 *
 * EXPERIMENTAL FEATURES:
 *   Dynamic Regulator operating Mode Switching (DRMS) - allows regulators
 *   to use most efficient operating mode depending upon voltage and load and
 *   is transparent to client drivers.
 */


#ifndef __LINUX_REGULATOR_H_
#define __LINUX_REGULATOR_H_

/*
 * Regulator operating modes.
 *
 * Regulators can run in a variety of different operating modes depending
 * output load. This allows further power saving though regulator efficiency.
 *
 * Most drivers will only care about NORMAL. The modes below are generic and
 * will probably not match the naming convention of your regulator data sheet
 * but should match the use cases in the datasheet.
 *
 * In order of power efficiency (least efficient at top).
 *
 *  Mode       Description
 *  FAST       Regulator can handle fast changes in it's load.
 *             e.g. usefull in CPU voltage & frequency scaling where
 *             load can quickly increase with freqency increases.
 *
 *  NORMAL     Normal regulator power supply mode. Most drivers will
 *             use this mode.
 *
 *  IDLE       Regulator runs in a more efficient mode for light
 *             loads. Can be used for devices that have a low power
 *             requirement during periods of inactivity. This mode
 *             may be more noisy than NORMAL and may not be able
 *             to handle fast load switching.
 *
 *  STANDBY    Regulator runs in most efficient mode for very
 *             light loads. Can be used by devices when they are
 *             in a sleep/standby state. This mode may be more noisy
 *             than NORMAL and may not be able to handle fast load
 *             switching.
 *
 * NOTE: Most regulators will only support a subset of these modes. Some
 * will only just support NORMAL.
 */

#define REGULATOR_MODE_FAST		0x1
#define REGULATOR_MODE_NORMAL		0x2
#define REGULATOR_MODE_IDLE		0x4
#define REGULATOR_MODE_STANDBY		0x8

/*
 * Regulator notifier events.
 *
 * @UNDER_VOLTAGE:  Regulator output is undervoltage.
 * @OVER_CURRENT:   Regulator output current is too high.
 * @POWER_ON:       Regulator power ON event.
 * @POWER_OFF:      Regulator power OFF event.
 * @REGULATION_OUT: Regulator output is out of regulation.
 * @FAIL:           Regulator output has failed.
 * @OVER_TEMP:      Regulator over temp.
 */

#define REGULATOR_EVENT_UNDER_VOLTAGE		0x1
#define REGULATOR_EVENT_OVER_CURRENT		0x2
#define REGULATOR_EVENT_POWER_ON		0x4
#define REGULATOR_EVENT_POWER_OFF		0x8
#define REGULATOR_EVENT_REGULATION_OUT		0x10
#define REGULATOR_EVENT_FAIL			0x20
#define REGULATOR_EVENT_OVER_TEMP		0x40

/*
 * Convenience conversion.
 * Here atm, maybe there is somewhere better for this.
 */
#define mV_to_uV(mV) (mV * 1000)
#define uV_to_mV(uV) (uV / 1000)
#define V_to_uV(V) (mV_to_uV(V * 1000))
#define uV_to_V(uV) (uV_to_mV(uV) / 1000)

struct regulator;

#if defined(CONFIG_REGULATOR_API)

/**
 * regulator_get - lookup and obtain a reference to a regulator.
 * @dev: device for regulator "consumer"
 * @id: regulator ID
 *
 * Returns a struct regulator corresponding to the regulator producer, or
 * valid IS_ERR() condition containing errno.
 *
 * Drivers must assume that the clock source is not enabled.
 */
struct regulator *regulator_get(struct device *dev, const char *id);

/**
 * regulator_put - "free" the regulator source
 * @regulator: regulator source
 * @dev: device
 *
 * Note: drivers must ensure that all regulator_enable calls made on this
 * regulator source are balanced by regulator_disable calls prior to calling
 * this function.
 */
void regulator_put(struct regulator *regulator, struct device *dev);

/**
 * regulator_enable - enable regulator output
 * @regulator: regulator source
 *
 * Enable the regulator output at the predefined voltage or current value.
 * Note: the output value can be set by other drivers, bootloader or may be
 * hardwired in the regulator.
 */
int regulator_enable(struct regulator *regulator);

/**
 * regulator_disable - disable regulator output
 * @regulator: regulator source
 *
 * Disable the regulator output voltage or current.
 */
int regulator_disable(struct regulator *regulator);

/**
 * regulator_is_enabled - is the regulator output enabled
 * @regulator: regulator source
 *
 * Returns zero for disabled otherwise return number of enable requests.
 */
int regulator_is_enabled(struct regulator *regulator);

/**
 * regulator_set_voltage - set regulator output voltage
 * @regulator: regulator source
 * @uV: voltage in uV
 *
 * Sets a voltage regulator to the desired output voltage. This can be set
 * during any regulator state. IOW, regulator can be disabled or enabled.
 *
 * If the regulator is enabled then the voltage will change to the new value
 * immediately otherwise if the regulator is disabled the regulator will
 * output at the new voltage when enabled.
 */
int regulator_set_voltage(struct regulator *regulator, int uV);

/**
 * regulator_get_voltage - get regulator output voltage
 * @regulator: regulator source
 *
 * This returns the regulator voltage in uV.
 *
 * Note: If the regulator is disabled it will return the voltage value. This
 * function should not be used to determine regulator state.
 */
int regulator_get_voltage(struct regulator *regulator);

/**
 * regulator_set_current - set regulator output current
 * @regulator: regulator source
 * @uV: voltage in uA
 *
 * Sets current regulator to the desired output current. This can be set during
 * any regulator state. IOW, regulator can be disabled or enabled.
 *
 * If the regulator is enabled then the current will change to the new value
 * immediately otherwise if the regulator is disabled the regulator will
 * output at the new current when enabled.
 */
int regulator_set_current(struct regulator *regulator, int uA);

/**
 * regulator_get_current - get regulator output current
 * @regulator: regulator source
 *
 * This returns the regulator current in uA.
 *
 * Note: If the regulator is disabled it will return the current value. This
 * function should not be used to determine regulator state.
 */
int regulator_get_current(struct regulator *regulator);

/**
 * regulator_set_mode - set regulator operating mode
 * @regulator: regulator source
 * @mode: operating mode
 *
 * Set regulator operating mode to increase regulator efficiency or improve
 * regulation performance.
 */
int regulator_set_mode(struct regulator *regulator, unsigned int mode);

/**
 * regulator_get_mode - set regulator operating mode
 * @regulator: regulator source
 *
 * Get the current regulator operating mode.
 */
unsigned int regulator_get_mode(struct regulator *regulator);

/**
 * regulator_get_optimum_mode - get regulator optimum operating mode
 * @regulator: regulator source
 * @input_uV: input voltage
 * @output_uV: output voltage
 * @load_uV: load current
 *
 * Get the most efficient regulator operating mode for the given input
 * and output voltages at a specific load..
 */
unsigned int regulator_get_optimum_mode(struct regulator *regulator,
	int input_uV, int output_uV, int load_uA);

/**
 * regulator_register_client - register regulator event notifier
 * @regulator: regulator source
 * @notifier_block: notifier block
 *
 * Register notifier block to receive regulator events.
 */
int regulator_register_client(struct regulator *regulator,
	struct notifier_block *nb);

/**
 * regulator_unregister_client - unregister regulator event notifier
 * @regulator: regulator source
 * @notifier_block: notifier block
 *
 * Unregister regulator event notifier block.
 */
int regulator_unregister_client(struct regulator *regulator,
	struct notifier_block *nb);

/**
 * regulator_notify_load - notify regulator of device max load
 * @regulator: regulator
 * @dev: device
 * @uA: load
 *
 * Notifies the regulator of new max device load. Can be used by DRMS to select
 * the most efficient regulator operating mode.
 *
 * Client devices notify their supply regulator of the maximum power
 * they will require when they change operational status and hence power
 * state. Examples of operational state changes that can affect power
 * consumption are :-
 *
 *    o Device is opened / closed.
 *    o Device IO is about to begin or has just finished.
 *    o Device is idling in between work.
 *
 * The power tables in device datasheets would be used by the driver for
 * the power consumption in each operational state. This information is
 * also exported via sysfs to userspace.
 *
 * DRMS would then sum the total requested load on the regulator and change
 * to the most efficient operating mode if platform constraints allow.
 */
void regulator_drms_notify_load(struct regulator *regulator,
	struct device *dev, int uA);

/**
 * regulator_get_drvdata - get regulator driver data
 * @regulator: regulator
 */
void *regulator_get_drvdata(struct regulator *regulator);

/**
 * regulator_set_drvdata - set regulator driver data
 * @regulator: regulator
 * @void: data
 */
void regulator_set_drvdata(struct regulator *regulator, void *data);

#else

/*
 * Make sure client drivers will still build on systems with no software
 * controllable voltage or current regulators.
 */
#define regulator_get(dev, id)	({ (void)(dev); (void)(id); NULL; })
#define regulator_put(regulator, dev) \
	do { (void)(regulator); (void)(dev); } while (0)
#define regulator_enable(regulator) ({ (void)(regulator); 0; })
#define regulator_disable(regulator) ({ (void)(regulator); 0; })
#define regulator_is_enabled(regulator) ({ (void)(regulator); 1; })
#define regulator_set_voltage(regulator, uV) \
	({ (void)(regulator); (void)(uV); 0; })
#define regulator_get_voltage(regulator) ({ (void)(regulator); 0; })
#define regulator_set_current(regulator, uA) \
	({ (void)(regulator); (void)(uA); 0; })
#define regulator_get_current(regulator) ({ (void)(regulator); 0; })
#define regulator_set_mode(regulator, mode) \
	({ (void)(regulator); (void)(mode); 0; })
#define regulator_get_mode(regulator) ({ (void)(regulator); 0; })
#define regulator_get_optimum_mode(regulator, input_uV, output_uV, load_uA) \
	({ (void)(regulator); (void)(input_uV); \
	   (void)(output_uV); (void)(load_uA); 0; })
#define regulator_register_client(regulator, nb) \
	({ (void)(regulator); (void)(nb); 0; })
#define regulator_unregister_client(regulator, nb) \
	do { (void)(regulator); (void)(nb); } while (0)
#define regulator_notify_load(regulator, dev, uA) \
	do { (void)(regulator); (void)(dev); (void)(uA); } while (0)
#define regulator_get_drvdata(regulator) ({ (void)(regulator); NULL; })
#define regulator_set_drvdata(regulator, data) \
	do { (void)(regulator); (void)(data); } while (0)

#endif

#endif
