/*
 * linux/include/asm-arm/arch-ixp4xx/io.h
 *
 * Author: Deepak Saxena <dsaxena@plexity.net>
 *
 * Copyright (C) 2002-2004  MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include <asm/hardware.h>

#define IO_SPACE_LIMIT 0xffff0000

#define	BIT(x)	((1)<<(x))


extern int (*ixp4xx_pci_read)(__u32 addr, __u32 cmd, __u32* data);
extern int ixp4xx_pci_write(__u32 addr, __u32 cmd, __u32 data);


/*
 * IXP4xx provides two methods of accessing PCI memory space:
 *
 * 1) A direct mapped window from 0x48000000 to 0x4bffffff (64MB).
 *    To access PCI via this space, we simply ioremap() the BAR
 *    into the kernel and we can use the standard read[bwl]/write[bwl]
 *    macros. This is the preffered method due to speed but it
 *    limits the system to just 64MB of PCI memory. This can be 
 *    problamatic if using video cards and other memory-heavy
 *    targets.
 *
 * 2) If > 64MB of memory space is required, the IXP4xx can be configured
 *    to use indirect registers to access PCI (as we do below for I/O
 *    transactions). This allows for up to 128MB (0x48000000 to 0x4fffffff)
 *    of memory on the bus. The disadvantadge of this is that every 
 *    PCI access requires three local register accesses plus a spinlock,
 *    but in some cases the performance hit is acceptable. In addition,
 *    you cannot mmap() PCI devices in this case.
 *
 */
#ifndef	CONFIG_IXP4XX_INDIRECT_PCI

#define __mem_pci(a)		(a)

#else

#include <linux/mm.h>

/*
 * In the case of using indirect PCI, we simply return the actual PCI
 * address and our read/write implementation use that to drive the 
 * access registers. If something outside of PCI is ioremap'd, we
 * fallback to the default.
 */
static inline void *
__ixp4xx_ioremap(unsigned long addr, size_t size, unsigned long flags, unsigned long align)
{
	extern void * __ioremap(unsigned long, size_t, unsigned long, unsigned long);
	if((addr < 0x48000000) || (addr > 0x4fffffff))
		return __ioremap(addr, size, flags, align);

	return (void *)addr;
}

static inline void
__ixp4xx_iounmap(void *addr)
{
	extern void __iounmap(void *addr);

	if ((__u32)addr >= VMALLOC_START)
		__iounmap(addr);
}

#define __arch_ioremap(a, s, f, x)	__ixp4xx_ioremap(a, s, f, x)
#define	__arch_iounmap(a)		__ixp4xx_iounmap(a)

#define	writeb(p, v)			__ixp4xx_writeb(p, v)
#define	writew(p, v)			__ixp4xx_writew(p, v)
#define	writel(p, v)			__ixp4xx_writel(p, v)

#define	writesb(p, v, l)		__ixp4xx_writesb(p, v, l)
#define	writesw(p, v, l)		__ixp4xx_writesw(p, v, l)
#define	writesl(p, v, l)		__ixp4xx_writesl(p, v, l)
	
#define	readb(p)			__ixp4xx_readb(p)
#define	readw(p)			__ixp4xx_readw(p)
#define	readl(p)			__ixp4xx_readl(p)
	
#define	readsb(p, v, l)			__ixp4xx_readsb(p, v, l)
#define	readsw(p, v, l)			__ixp4xx_readsw(p, v, l)
#define	readsl(p, v, l)			__ixp4xx_readsl(p, v, l)

static inline void 
__ixp4xx_writeb(__u8 value, __u32 addr)
{
	__u32 n, byte_enables, data;

	if (addr >= VMALLOC_START) {
		__raw_writeb(value, addr);
		return;
	}

	n = addr % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP4XX_PCI_NP_CBE_BESL;
	data = value << (8*n);
	ixp4xx_pci_write(addr, byte_enables | NP_CMD_MEMWRITE, data);
}

static inline void
__ixp4xx_writesb(__u32 bus_addr, __u8 *vaddr, int count)
{
	while (count--)
		writeb(*vaddr++, bus_addr);
}

static inline void 
__ixp4xx_writew(__u16 value, __u32 addr)
{
	__u32 n, byte_enables, data;

	if (addr >= VMALLOC_START) {
		__raw_writew(value, addr);
		return;
	}

	n = addr % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP4XX_PCI_NP_CBE_BESL;
	data = value << (8*n);
	ixp4xx_pci_write(addr, byte_enables | NP_CMD_MEMWRITE, data);
}

static inline void
__ixp4xx_writesw(__u32 bus_addr, __u16 *vaddr, int count)
{
	while (count--)
		writew(*vaddr++, bus_addr);
}

static inline void 
__ixp4xx_writel(__u32 value, __u32 addr)
{
	if (addr >= VMALLOC_START) {
		__raw_writel(value, addr);
		return;
	}

	ixp4xx_pci_write(addr, NP_CMD_MEMWRITE, value);
}

static inline void
__ixp4xx_writesl(__u32 bus_addr, __u32 *vaddr, int count)
{
	while (count--)
		writel(*vaddr++, bus_addr);
}

static inline unsigned char 
__ixp4xx_readb(__u32 addr)
{
	__u32 n, byte_enables, data;

	if (addr >= VMALLOC_START)
		return __raw_readb(addr);

	n = addr % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP4XX_PCI_NP_CBE_BESL;
	if (ixp4xx_pci_read(addr, byte_enables | NP_CMD_MEMREAD, &data))
		return 0xff;

	return data >> (8*n);
}

static inline void
__ixp4xx_readsb(__u32 bus_addr, __u8 *vaddr, __u32 count)
{
	while (count--)
		*vaddr++ = readb(bus_addr);
}

static inline unsigned short 
__ixp4xx_readw(__u32 addr)
{
	__u32 n, byte_enables, data;

	if (addr >= VMALLOC_START)
		return __raw_readw(addr);

	n = addr % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP4XX_PCI_NP_CBE_BESL;
	if (ixp4xx_pci_read(addr, byte_enables | NP_CMD_MEMREAD, &data))
		return 0xffff;

	return data>>(8*n);
}

static inline void 
__ixp4xx_readsw(__u32 bus_addr, __u16 *vaddr, __u32 count)
{
	while (count--)
		*vaddr++ = readw(bus_addr);
}

static inline unsigned long 
__ixp4xx_readl(__u32 addr)
{
	__u32 data;

	if (addr >= VMALLOC_START)
		return __raw_readl(addr);

	if (ixp4xx_pci_read(addr, NP_CMD_MEMREAD, &data))
		return 0xffffffff;

	return data;
}

static inline void 
__ixp4xx_readsl(__u32 bus_addr, __u32 *vaddr, __u32 count)
{
	while (count--)
		*vaddr++ = readl(bus_addr);
}


/*
 * We can use the built-in functions b/c they end up calling writeb/readb
 */
#define memset_io(c,v,l)		_memset_io((c),(v),(l))
#define memcpy_fromio(a,c,l)		_memcpy_fromio((a),(c),(l))
#define memcpy_toio(c,a,l)		_memcpy_toio((c),(a),(l))

#define eth_io_copy_and_sum(s,c,l,b) \
				eth_copy_and_sum((s),__mem_pci(c),(l),(b))

static inline int
check_signature(unsigned long bus_addr, const unsigned char *signature,
		int length)
{
	int retval = 0;
	do {
		if (readb(bus_addr) != *signature)
			goto out;
		bus_addr++;
		signature++;
		length--;
	} while (length);
	retval = 1;
out:
	return retval;
}

#endif

/*
 * IXP4xx does not have a transparent cpu -> PCI I/O translation
 * window.  Instead, it has a set of registers that must be tweaked
 * with the proper byte lanes, command types, and address for the
 * transaction.  This means that we need to override the default
 * I/O functions.
 */
#define	outb(p, v)			__ixp4xx_outb(p, v)
#define	outw(p, v)			__ixp4xx_outw(p, v)
#define	outl(p, v)			__ixp4xx_outl(p, v)
	
#define	outsb(p, v, l)			__ixp4xx_outsb(p, v, l)
#define	outsw(p, v, l)			__ixp4xx_outsw(p, v, l)
#define	outsl(p, v, l)			__ixp4xx_outsl(p, v, l)

#define	inb(p)				__ixp4xx_inb(p)
#define	inw(p)				__ixp4xx_inw(p)
#define	inl(p)				__ixp4xx_inl(p)

#define	insb(p, v, l)			__ixp4xx_insb(p, v, l)
#define	insw(p, v, l)			__ixp4xx_insw(p, v, l)
#define	insl(p, v, l)			__ixp4xx_insl(p, v, l)


static inline void 
__ixp4xx_outb(__u8 value, __u32 addr)
{
	__u32 n, byte_enables, data;
	n = addr % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP4XX_PCI_NP_CBE_BESL;
	data = value << (8*n);
	ixp4xx_pci_write(addr, byte_enables | NP_CMD_IOWRITE, data);
}

static inline void 
__ixp4xx_outsb(__u32 io_addr, const __u8 *vaddr, __u32 count)
{
	while (count--)
		outb(*vaddr++, io_addr);
}

static inline void 
__ixp4xx_outw(__u16 value, __u32 addr)
{
	__u32 n, byte_enables, data;
	n = addr % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP4XX_PCI_NP_CBE_BESL;
	data = value << (8*n);
	ixp4xx_pci_write(addr, byte_enables | NP_CMD_IOWRITE, data);
}

static inline void 
__ixp4xx_outsw(__u32 io_addr, const __u16 *vaddr, __u32 count)
{
	while (count--)
		outw(cpu_to_le16(*vaddr++), io_addr);
}

static inline void 
__ixp4xx_outl(__u32 value, __u32 addr)
{
	ixp4xx_pci_write(addr, NP_CMD_IOWRITE, value);
}

static inline void 
__ixp4xx_outsl(__u32 io_addr, const __u32 *vaddr, __u32 count)
{
	while (count--)
		outl(*vaddr++, io_addr);
}

static inline __u8 
__ixp4xx_inb(__u32 addr)
{
	__u32 n, byte_enables, data;
	n = addr % 4;
	byte_enables = (0xf & ~BIT(n)) << IXP4XX_PCI_NP_CBE_BESL;
	if (ixp4xx_pci_read(addr, byte_enables | NP_CMD_IOREAD, &data))
		return 0xff;

	return data >> (8*n);
}

static inline void 
__ixp4xx_insb(__u32 io_addr, __u8 *vaddr, __u32 count)
{
	while (count--)
		*vaddr++ = inb(io_addr);
}

static inline __u16 
__ixp4xx_inw(__u32 addr)
{
	__u32 n, byte_enables, data;
	n = addr % 4;
	byte_enables = (0xf & ~(BIT(n) | BIT(n+1))) << IXP4XX_PCI_NP_CBE_BESL;
	if (ixp4xx_pci_read(addr, byte_enables | NP_CMD_IOREAD, &data))
		return 0xffff;

	return data>>(8*n);
}

static inline void 
__ixp4xx_insw(__u32 io_addr, __u16 *vaddr, __u32 count)
{
	while (count--)
		*vaddr++ = le16_to_cpu(inw(io_addr));
}

static inline __u32 
__ixp4xx_inl(__u32 addr)
{
	__u32 data;
	if (ixp4xx_pci_read(addr, NP_CMD_IOREAD, &data))
		return 0xffffffff;

	return data;
}

static inline void 
__ixp4xx_insl(__u32 io_addr, __u32 *vaddr, __u32 count)
{
	while (count--)
		*vaddr++ = inl(io_addr);
}


#endif	//  __ASM_ARM_ARCH_IO_H

