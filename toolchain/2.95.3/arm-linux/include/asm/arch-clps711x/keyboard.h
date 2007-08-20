/*
 * linux/include/asm-arm/arch-clps711x/keyboard.h
 *
 * Copyright (C) 1998 Russell King
 */

#define NR_SCANCODES 128

#define kbd_setkeycode(sc,kc)		(-EINVAL)
#define kbd_getkeycode(sc)		(-EINVAL)

/* Prototype: int kbd_pretranslate(scancode, raw_mode)
 * Returns  : 0 to ignore scancode
 */
#define kbd_pretranslate(sc,rm)		(1)

/* Prototype: int kbd_translate(scancode, *keycode, *up_flag, raw_mode)
 * Returns  : 0 to ignore scancode, *keycode set to keycode, *up_flag
 *            set to 0200 if scancode indicates release
 */
#define kbd_sysrq_xlate			((unsigned char *)0)
#define kbd_translate(sc, kcp, rm) 0
#define kbd_unexpected_up(kc)		(0200)
#define kbd_leds(leds)			do { } while (0)
#define kbd_init_hw()			do { } while (0)
#define kbd_disable_irq()		do { } while (0)
#define kbd_enable_irq()		do { } while (0)

#define SYSRQ_KEY			(0)
