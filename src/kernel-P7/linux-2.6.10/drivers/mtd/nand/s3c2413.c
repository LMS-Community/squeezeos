/* linux/drivers/mtd/nand/s3c2413.c
 *
 * Samsung S3C2413 NAND driver
 *
 * Changelog:
 *	21-Sep-2004  BJD  Initial version
 *	23-Sep-2004  BJD  Mulitple device support
 *	28-Sep-2004  BJD  Fixed ECC placement for Hardware mode
 *	12-Oct-2004  BJD  Fixed errors in use of platform data
 *
 * $Id: s3c2413.c,v 1.2 2006/02/08 08:48:11 naushad Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/err.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/hardware/clock.h>

#include <asm/arch/bitfield.h>
#include <asm/arch/regs-nand.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-ebi.h> 
#include <asm/arch/nand.h>

#define PFX "s3c2413-nand: "

#define elfin_nand_init_controller()					 \
do{									 \
	unsigned long val;						 \
	val = (readl(info->regs + S3C2413_NFCONF) & NFCONF_KeepMask) | NFCONF_InitSet | (0<<0); \
	writel(val, info->regs + S3C2413_NFCONF);			 \
	writel(NFCONT_InitSet, info->regs + S3C2413_NFCONT);		 \
}while(0)

#define elfin_nand_select() 			            	 \
do{		 						 \
	unsigned long val1, val2;					 \
	val1 = readl(info->regs + S3C2413_NFCONF); \
	if( val1 & (1<<31)) \
		val2 = (readl(info->regs + S3C2413_NFCONT) & ~(1<<1)) ;	 \
	else  \
		val2 = (readl(info->regs + S3C2413_NFCONT) & ~(1<<2)) ;	 \
	writel(val2, info->regs + S3C2413_NFCONT);		 \
}while(0)

#define elfin_nand_deselect() 			            	 \
do{		 						 \
	unsigned long val1, val2;					 \
	val1 = readl(info->regs + S3C2413_NFCONF); \
	if( val1 & (1<<31)) \
		val2 = (readl(info->regs + S3C2413_NFCONT) |  (1<<1)) ;	 \
	else  \
		val2 = (readl(info->regs + S3C2413_NFCONT) |  (1<<2)) ;	 \
	writel(val2, info->regs + S3C2413_NFCONT);		 \
}while(0)

/*
#define elfin_nand_deselect() 			            	 \
do{		 						 \
	unsigned long val;					 \
	val = (readl(info->regs + S3C2413_NFCONT) | (1<<1)) ;	 \
	writel(val, info->regs + S3C2413_NFCONT);		 \
}while(0)
*/
#define elfin_nand_put_cmd(cmd)					\
do{		 						\
	writel(sNFCMMD_B0(cmd), info->regs + S3C2413_NFCMMD);	\
}while(0)

#define elfin_nand_put_addr(addr)					\
do{		 							\
	writel(sNFADDR_B0(addr), info->regs + S3C2413_NFADDR);		\
}while(0)

/* controller and mtd information */

struct s3c2413_nand_info;

struct s3c2413_nand_mtd {
	struct mtd_info			mtd;
	struct nand_chip		chip;
	struct s3c2413_nand_set		*set;
	struct s3c2413_nand_info	*info;
	int				scan_res;
};

/* overview of the s3c2413 nand state */

struct s3c2413_nand_info {
	/* mtd info */
	struct nand_hw_control		controller;
	struct s3c2413_nand_mtd		*mtds;
	struct s3c2413_platform_nand	*platform;

	/* device info */
	struct device			*device;
	struct resource			*area;
	struct clk			*clk;
	void				*regs;
	int				mtd_count;
};

/* conversion functions */

static struct s3c2413_nand_mtd *s3c2413_nand_mtd_toours(struct mtd_info *mtd)
{
	return container_of(mtd, struct s3c2413_nand_mtd, mtd);
}

static struct s3c2413_nand_info *s3c2413_nand_mtd_toinfo(struct mtd_info *mtd)
{
	return s3c2413_nand_mtd_toours(mtd)->info;
}

static struct s3c2413_nand_info *to_nand_info(struct device *dev)
{
	return dev_get_drvdata(dev);
}

static struct s3c2413_platform_nand *to_nand_plat(struct device *dev)
{
	return dev->platform_data;
}

/* timing calculations */

#define NS_IN_KHZ 10000000

#if 0
static int s3c2413_nand_calc_rate(int wanted, unsigned long clk, int max)
{
	int result;

	result = (wanted * NS_IN_KHZ) / clk;
	result++;

	pr_debug("result %d from %ld, %d\n", result, clk, wanted);

	if (result > max) {
		printk("%d ns is too big for current clock rate %ld\n",
		       wanted, clk);
		return -1;
	}

	if (result < 1)
		result = 1;

	return result;
}
#endif

#define to_ns(ticks,clk) (((clk) * (ticks)) / NS_IN_KHZ)

/* controller setup */


static int s3c2413_nand_inithw(struct s3c2413_nand_info *info, 
			       struct device *dev)
{
	unsigned long val;

	// rahul, for 2413
	val = readl(S3C2413_BANK_CFG);
	val |= S3C2413_BANK_CFG_BANK_2_3_NAND; 
	writel(val, S3C2413_BANK_CFG);

	val = readl(S3C2413_GPACON);
	val &= ~(0x1f<<17);
	writel(val, S3C2413_GPACON);
	val = readl(S3C2413_GPACON);
	val |= (0x1f<<17);
	writel(val, S3C2413_GPACON);

	elfin_nand_init_controller();

	val = readl(info->regs + S3C2413_NFCONF);
	val &= ~(1 << 3);
#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
	val |= (1 << 3);
#endif
	writel(val,info->regs +  S3C2413_NFCONF);

	return 0;
}

/* select chip */

static void s3c2413_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);

	if (chip == -1)
		elfin_nand_deselect();
	else
		elfin_nand_select();
}
static u8 s3c2413_nand_getdevid(struct mtd_info *mtd)
{
	u8 id1,id2,i;
	struct nand_chip *this = mtd->priv;
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);

	elfin_nand_select();
       	elfin_nand_put_cmd (NAND_CMD_READID);
        elfin_nand_put_addr(0x0);
        for(i=0; i<30; i++); /* Wait for tWHR(min 60ns) */
	id1 =readb(this->IO_ADDR_R);
	id2 =readb(this->IO_ADDR_R);
	elfin_nand_deselect();
	pr_debug("nand id1%x  devid %x\n",id1,id2);
	return id2;
}

/* command and control functions */

static void s3c2413_nand_hwcontrol(struct mtd_info *mtd, int cmd)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);

	switch (cmd) {
	case NAND_CTL_SETNCE:
		elfin_nand_select();
		break;

	case NAND_CTL_CLRNCE:
		elfin_nand_select();
		break;

		/* we don't need to implement these */
	case NAND_CTL_SETCLE:
	case NAND_CTL_CLRCLE:
	case NAND_CTL_SETALE:
	case NAND_CTL_CLRALE:
		pr_debug(PFX "s3c2413_nand_hwcontrol(%d) unusedn", cmd);
		break;
	}
}

#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
/*
 * function for reading from large size (2k) pages
 */
static void s3c2413_nand_command_lp(struct mtd_info *mtd, unsigned command,
			       int column, int page_addr)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	struct nand_chip *this = mtd->priv;
	int i;

	
	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->oobblock;
		command = NAND_CMD_READ0;
	}
	
        /*
         * Write out the command to the device.
         */
//	printk("column %x page %x\n",column,page_addr);
        elfin_nand_put_cmd (command);
                                                                                                                             
        if (column != -1 || page_addr != -1) {
                /* Serially input address */
		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (this->options & NAND_BUSWIDTH_16)
				column >>= 1;
			elfin_nand_put_addr(column & 0xff);
			elfin_nand_put_addr(column >> 8);
		}	
                if (page_addr != -1) {
                        elfin_nand_put_addr (page_addr & 0xff);
                        elfin_nand_put_addr ((page_addr >> 8) & 0xff);
                        /* One more address cycle for higher density devices */
			if (this->chipsize > (128 << 20))
                                elfin_nand_put_addr ((page_addr >> 16) & 0x0f);
                }
        }
                                                                                                                             
        /*
         * program and erase have their own busy handlers
         * status and sequential in needs no delay
         */
        switch (command) {
                case NAND_CMD_STATUS:
                case NAND_CMD_READID:
        		elfin_nand_put_addr (0x0);
                        for(i=0; i<30; i++); /* Wait for tWHR(min 60ns) */
                        return;
                                                                                                                             
                case NAND_CMD_RESET:
                case NAND_CMD_READ1:
                        for(i=0; i<40; i++); /* Wait for tWB(max 100ns) */
                        while (!this->dev_ready(mtd));
			break;
		case NAND_CMD_READ0:
        		elfin_nand_put_cmd ( NAND_CMD_READSTART);
                        for(i=0; i<40; i++); /* Wait for tWB(max 100ns) */
                        while (!this->dev_ready(mtd));
			break;
        }
                                                                                                                             
        return;
}
#else
/* s3c2413_nand_command
 *
 * This function implements sending commands and the relevant address
 * information to the chip, via the hardware controller. Since the
 * S3C2413 generates the correct ALE/CLE signaling automatically, we
 * do not need to use hwcontrol.
*/
static void s3c2413_nand_command_sp(struct mtd_info *mtd, unsigned command,
			       int column, int page_addr)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	struct nand_chip *this = mtd->priv;
	int i;

	/*
	 * Write out the command to the device.
	 */
	if(command == NAND_CMD_SEQIN) {
		unsigned setptr;

		if(mtd->oobblock == 256 && column >= 256) {
			column -= 256;
			setptr = NAND_CMD_READOOB;
		} else if(mtd->oobblock == 512 && column >= 256) {
			if(column < 512) {
				column -= 256;
				setptr = NAND_CMD_READ1;
			} else {
				column -= 512;
				setptr = NAND_CMD_READOOB;
			}
		} else {
			setptr = NAND_CMD_READ0;
		}

		elfin_nand_put_cmd(setptr);
	}

	elfin_nand_put_cmd(command);

	/* Start address cycle */
	if(column != -1 || page_addr != -1) {
		/* Serially input address */
		if(column != -1) {
			elfin_nand_put_addr(column);
		}
		if(page_addr != -1) {
			elfin_nand_put_addr(page_addr & 0xff);
			elfin_nand_put_addr((page_addr >> 8) & 0xff);
			/* One more address cycle for higher density devices */
			if(mtd->size & 0x0c000000) {
				elfin_nand_put_addr((page_addr >> 16) & 0x0f);
			}
		}
	}

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	 */
	switch (command) {
		case NAND_CMD_STATUS:
		case NAND_CMD_READID:
			for(i = 0; i < 30; i++) {};	/* Wait for tWHR(min 60ns) */
			return;

		case NAND_CMD_RESET:
		case NAND_CMD_READ0:
		case NAND_CMD_READ1:
		case NAND_CMD_READOOB:
			for(i = 0; i < 40; i++) {};	/* Wait for tWB(max 100ns) */
			while(!this->dev_ready(mtd)) {};
	}
	return;
}
#endif

/* s3c2413_nand_devready()
 *
 * returns 0 if the nand is busy, 1 if it is ready
*/

static int s3c2413_nand_devready(struct mtd_info *mtd)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	
	return (readl(info->regs + S3C2413_NFSTAT) & (1<<0)); 
}

#ifdef CONFIG_MTD_NAND_S3C2413_HWECC

#define S3C2413_NFECC		S3C2413_NFMECCD0

/* new oob placement block for use with hardware ecc generation
 */

#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
static struct nand_oobinfo nand_hw_eccoob64 = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 4,
	.eccpos = { 60, 61, 62, 63 },
	.oobfree = { {2, 58} }
};
#else
static struct nand_oobinfo nand_hw_eccoob = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	.eccbytes = 3,
	.eccpos = {0, 1, 2 },
	.oobfree = { {8, 8} }
};
#endif

/* ECC handling functions */

static int s3c2413_nand_correct_data(struct mtd_info *mtd, u_char *dat,
				     u_char *read_ecc, u_char *calc_ecc)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	u32 eccestat0,ecccheck;
	
	pr_debug("s3c2413_nand_correct_data(%p,%p,%p,%p)\n",
		 mtd, dat, read_ecc, calc_ecc);

	pr_debug("eccs: read %02x,%02x,%02x,%02x\n",
		 read_ecc[0], read_ecc[1], read_ecc[2],read_ecc[3]);

	pr_debug(" vs calc %02x,%02x,%02x,%02x\n",
		 calc_ecc[0], calc_ecc[1], calc_ecc[2],calc_ecc[3]);

	ecccheck = read_ecc[0] | (read_ecc[1] << 16);
	writel(ecccheck, info->regs + S3C2413_NFMECCD0);

#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
	ecccheck = read_ecc[2] | ((read_ecc[3] & 0xff) << 16);
#else
	ecccheck = read_ecc[2] | (0xff<<16);
#endif
	writel(ecccheck, info->regs + S3C2413_NFMECCD1);

	eccestat0 = readl(info->regs + S3C2413_NFESTAT0);


	pr_debug("ecc status :%02x \n",eccestat0);

	if((eccestat0 & 0x3) == 0) 
		return 0;

	/* we curently have no method for correcting the error */

	return -1;
}

static void s3c2413_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	unsigned long val;

	val = readl(info->regs + S3C2413_NFCONT);
	val |= NFCONT_INITECC;
	val &= ~NFCONT_LOCKMECC;
	writel(val, info->regs + S3C2413_NFCONT);
	val = readl(info->regs + S3C2413_NFSTAT);
//	val |= (1<<2);
	val |= (1<<4); //rahul, for 2413 RnB_TransDetect
	writel(val, info->regs + S3C2413_NFSTAT);
}

static int s3c2413_nand_calculate_ecc(struct mtd_info *mtd,
				      const u_char *dat, u_char *ecc_code)
{
	unsigned long val;
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);

	val = readl(info->regs + S3C2413_NFCONT);
	val |= NFCONT_LOCKMECC;
	writel(val, info->regs + S3C2413_NFCONT);

	val = readl(info->regs + S3C2413_NFMECC0);

	ecc_code[0] = val & 0xff;
	ecc_code[1] = (val >> 8) & 0xff;
	ecc_code[2] = (val >> 16) & 0xff;
#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
	ecc_code[3] = (((val >> 24)& 0xff));
#endif

	pr_debug("calculate_ecc: returning ecc %02x,%02x,%02x,%02x\n",
		 ecc_code[0], ecc_code[1], ecc_code[2],ecc_code[3]);

	return 0;
}
#endif

/* over-ride the standard functions for a little more speed? */
static void s3c2413_nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
#if 0
	int ind;
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	for(ind=0; ind<len; ind++)
	   buf[ind] = (u_char*)readb(info->regs + S3C2413_NFDATA8);
#else
	struct nand_chip *this = mtd->priv;
	readsb(this->IO_ADDR_R, buf, len);
#endif
}

static void s3c2413_nand_write_buf(struct mtd_info *mtd,
				   const u_char *buf, int len)
{
#if 0
	int ind;
	struct s3c2413_nand_info *info = s3c2413_nand_mtd_toinfo(mtd);
	for(ind=0; ind<len; ind++)
	   writeb(buf[ind], info->regs + S3C2413_NFDATA8);
#else
	struct nand_chip *this = mtd->priv;
	writesb(this->IO_ADDR_W, buf, len);
#endif
}

/* device management functions */

static int s3c2413_nand_remove(struct device *dev)
{
	struct s3c2413_nand_info *info = to_nand_info(dev);

	dev_set_drvdata(dev, NULL);

	if (info == NULL) 
		return 0;

	/* first thing we need to do is release all our mtds
	 * and their partitions, then go through freeing the
	 * resources used 
	 */
	
	if (info->mtds != NULL) {
		struct s3c2413_nand_mtd *ptr = info->mtds;
		int mtdno;

		for (mtdno = 0; mtdno < info->mtd_count; mtdno++, ptr++) {
			pr_debug("releasing mtd %d (%p)\n", mtdno, ptr);
			nand_release(&ptr->mtd);
		}

		kfree(info->mtds);
	}

	/* free the common resources */

	if (info->clk != NULL && !IS_ERR(info->clk)) {
		clk_disable(info->clk);
		clk_unuse(info->clk);
		clk_put(info->clk);
	}

	if (info->regs != NULL) {
		iounmap(info->regs);
		info->regs = NULL;
	}

	if (info->area != NULL) {
		release_resource(info->area);
		kfree(info->area);
		info->area = NULL;
	}

	kfree(info);

	return 0;
}

#ifdef CONFIG_MTD_PARTITIONS
static int s3c2413_nand_add_partition(struct s3c2413_nand_info *info,
				      struct s3c2413_nand_mtd *mtd,
				      struct s3c2413_nand_set *set)
{
	if (set == NULL)
		return add_mtd_device(&mtd->mtd);

	if (set->nr_partitions > 0 && set->partitions != NULL) {
		return add_mtd_partitions(&mtd->mtd,
					  set->partitions,
					  set->nr_partitions);
	}

	return add_mtd_device(&mtd->mtd);
}
#else
static int s3c2413_nand_add_partition(struct s3c2413_nand_info *info,
				      struct s3c2413_nand_mtd *mtd,
				      struct s3c2413_nand_set *set)
{
	return add_mtd_device(&mtd->mtd);
}
#endif

/* s3c2413_nand_init_chip
 *
 * init a single instance of an chip 
*/

static void s3c2413_nand_init_chip(struct s3c2413_nand_info *info,
				   struct s3c2413_nand_mtd *nmtd,
				   struct s3c2413_nand_set *set)
{
	struct nand_chip *chip = &nmtd->chip;

	chip->IO_ADDR_R	   = (char *)info->regs + S3C2413_NFDATA;
	chip->IO_ADDR_W    = (char *)info->regs + S3C2413_NFDATA;
	chip->hwcontrol    = s3c2413_nand_hwcontrol;
	chip->dev_ready    = s3c2413_nand_devready;
#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
	chip->cmdfunc      = s3c2413_nand_command_lp;
#else
	chip->cmdfunc      = s3c2413_nand_command_sp;
#endif
	chip->write_buf    = s3c2413_nand_write_buf;
	chip->read_buf     = s3c2413_nand_read_buf;
	chip->select_chip  = s3c2413_nand_select_chip;
	chip->chip_delay   = 50;
	chip->priv	   = nmtd;
	chip->options	   = 0;
	chip->controller   = &info->controller;

	nmtd->info	   = info;
	nmtd->mtd.priv	   = chip;
	nmtd->set	   = set;

#ifdef CONFIG_MTD_NAND_S3C24XX_DISABLE_ECC
	chip->eccmode	    = NAND_ECC_NONE;
#else
#ifdef CONFIG_MTD_NAND_S3C2413_HWECC
	chip->correct_data  = s3c2413_nand_correct_data;
	chip->enable_hwecc  = s3c2413_nand_enable_hwecc;
	chip->calculate_ecc = s3c2413_nand_calculate_ecc;
#ifdef CONFIG_S3C24XX_LARGEPAGE_NAND
	chip->eccmode	    = NAND_ECC_HW4_2048;
	chip->autooob       = &nand_hw_eccoob64;
#else
	chip->eccmode	    = NAND_ECC_HW3_512;
	chip->autooob       = &nand_hw_eccoob;
#endif  //S3C24XX_LARGEPAGE_NAND
#else   //HWECC
	chip->eccmode	    = NAND_ECC_SOFT;
#endif
#endif
}

/* s3c2413_nand_probe
 *
 * called by device layer when it finds a device matching
 * one our driver can handled. This code checks to see if
 * it can allocate all necessary resources then calls the
 * nand layer to look for devices
*/

static int s3c2413_nand_probe(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct s3c2413_platform_nand *plat = to_nand_plat(dev);
	struct s3c2413_nand_info *info;
	struct s3c2413_nand_mtd *nmtd;
	struct s3c2413_nand_set *sets;
	struct resource *res;
	int err = 0;
	int size;
	int nr_sets;
	int setno;
	u8 devid;

	pr_debug("s3c2413_nand_probe(%p)\n", dev);

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL) {
		printk(KERN_ERR PFX "no memory for flash info\n");
		err = -ENOMEM;
		goto exit_error;
	}

	memzero(info, sizeof(*info));
	dev_set_drvdata(dev, info);

	spin_lock_init(&info->controller.lock);

	/* get the clock source and enable it */

	info->clk = clk_get(dev, "nand");
	if (IS_ERR(info->clk)) {
		printk(KERN_ERR PFX "failed to get clock");
		err = -ENOENT;
		goto exit_error;
	}

	clk_use(info->clk);
	clk_enable(info->clk);

	/* allocate and map the resource */

	res = pdev->resource;  /* assume that the flash has one resource */
	size = res->end - res->start + 1;

	info->area = request_mem_region(res->start, size, pdev->name);

	if (info->area == NULL) {
		printk(KERN_ERR PFX "cannot reserve register region\n");
		err = -ENOENT;
		goto exit_error;
	}

	info->device = dev;
	info->platform = plat;
	info->regs = ioremap(res->start, size);

	if (info->regs == NULL) {
		printk(KERN_ERR PFX "cannot reserve register region\n");
		err = -EIO;
		goto exit_error;
	}		

	printk(KERN_INFO PFX "mapped registers at %p\n", info->regs);

	/* initialise the hardware */

	err = s3c2413_nand_inithw(info, dev);
	if (err != 0)
		goto exit_error;

	sets = (plat != NULL) ? plat->sets : NULL;
	nr_sets = (plat != NULL) ? plat->nr_sets : 1;

	info->mtd_count = nr_sets;

	/* allocate our information */

	//size = nr_sets * sizeof(*info->mtds);
	size = 1 * sizeof(*info->mtds);
	info->mtds = kmalloc(size, GFP_KERNEL);
	if (info->mtds == NULL) {
		printk(KERN_ERR PFX "failed to allocate mtd storage\n");
		err = -ENOMEM;
		goto exit_error;
	}

	memzero(info->mtds, size);

	/* initialise all possible chips */

	nmtd = info->mtds;
	s3c2413_nand_init_chip(info, nmtd, sets);

	nmtd->scan_res = nand_scan(&nmtd->mtd,
				   (sets) ? sets->nr_chips : 1);

	if (nmtd->scan_res == 0) {
		devid = s3c2413_nand_getdevid(&nmtd->mtd);
		for (setno = 0; setno < nr_sets; setno++, sets++) {
			if(devid == sets->devid) {
				nmtd->set = sets;
				s3c2413_nand_add_partition(info, nmtd, sets);
				break;
			}
		}
		if (setno == nr_sets)
			goto exit_error;
	} else
		goto exit_error;


	
	pr_debug("initialised ok\n");
	return 0;

 exit_error:
	s3c2413_nand_remove(dev);

	if (err == 0)
		err = -EINVAL;
	return err;
}

static struct device_driver s3c2413_nand_driver = {
	.name		= "s3c2413-nand",
	.bus		= &platform_bus_type,
	.probe		= s3c2413_nand_probe,
	.remove		= s3c2413_nand_remove,
};

static int __init s3c2413_nand_init(void)
{
	printk("S3C2413 NAND Driver, (c) 2004 Simtec Electronics\n");
	return driver_register(&s3c2413_nand_driver);
}

static void __exit s3c2413_nand_exit(void)
{
	driver_unregister(&s3c2413_nand_driver);
}

module_init(s3c2413_nand_init);
module_exit(s3c2413_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
MODULE_DESCRIPTION("S3C2413 MTD NAND driver");
