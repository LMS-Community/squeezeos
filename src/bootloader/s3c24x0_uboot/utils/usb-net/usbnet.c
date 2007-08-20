/*
 * USB Host-to-Host Links
 * Copyright (C) 2000-2002 by David Brownell <dbrownell@users.sourceforge.net>
 */

/*
 * This is used for "USB networking", connecting USB hosts as peers.
 *
 * It can be used with USB "network cables", for IP-over-USB communications;
 * Ethernet speeds without the Ethernet.  USB devices (including some PDAs)
 * can support such links directly, replacing device-specific protocols
 * with Internet standard ones.
 *
 * The links can be bridged using the Ethernet bridging (net/bridge)
 * support as appropriate.  Devices currently supported include:
 *
 *	- "Linux Devices" (like iPaq and similar SA-1100 based PDAs)
 *
 * USB devices can implement their side of this protocol at the cost
 * of two bulk endpoints; it's not restricted to "cable" applications.
 * See the LINUXDEV or EPSON device/client support.
 *
 *
 * 06-apr-2002	Added ethtool support, based on a patch from Brad Hards.
 *		Level of diagnostics is more configurable; they use device
 *		location (usb_device->devpath) instead of address (2.5).
 *		For tx_fixup, memflags can't be NOIO.
 * 07-may-2002	Generalize/cleanup keventd support, handling rx stalls (mostly
 *		for USB 2.0 TTs) and memory shortages (potential) too. (db)
 *		Use "locally assigned" IEEE802 address space. (Brad Hards)
 *
 *-------------------------------------------------------------------------*/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/ethtool.h>
#include <linux/tqueue.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#define	DEBUG			// error path messages, extra info
#define	VERBOSE			// more; success messages
// #define	REALLY_QUEUE

#if !defined (DEBUG) && defined (CONFIG_USB_DEBUG)
#   define DEBUG
#endif
#include <linux/usb.h>

/* in 2.5 these standard usb ops take mem_flags */
#define ALLOC_URB(n,flags)	usb_alloc_urb(n)
#define SUBMIT_URB(u,flags)	usb_submit_urb(u)

/* and these got renamed (may move to usb.h) */
#define usb_get_dev		usb_inc_dev_use
#define usb_put_dev		usb_dec_dev_use

#define	CONFIG_USB_LINUXDEV  

#define DRIVER_VERSION		"17-Jul-2002"

/*-------------------------------------------------------------------------*/

/*
 * Nineteen USB 1.1 max size bulk transactions per frame (ms), max.
 * Several dozen bytes of IPv4 data can fit in two such transactions.
 * One maximum size Ethernet packet takes twenty four of them.
 */
#ifdef REALLY_QUEUE
#define	RX_QLEN		4
#define	TX_QLEN		4
#else
#define	RX_QLEN		1
#define	TX_QLEN		1
#endif

// packets are always ethernet inside
// ... except they can be bigger (limit of 64K with NetChip framing)
#define MIN_PACKET	sizeof(struct ethhdr)
#define MAX_PACKET	32768

// reawaken network queue this soon after stopping; else watchdog barks
#define TX_TIMEOUT_JIFFIES	(5*HZ)

// for vendor-specific control operations
#define	CONTROL_TIMEOUT_MS	(500)			/* msec */
#define CONTROL_TIMEOUT_JIFFIES ((CONTROL_TIMEOUT_MS * HZ)/1000)

// between wakeups
#define UNLINK_TIMEOUT_JIFFIES ((3  /*ms*/ * HZ)/1000)

/*-------------------------------------------------------------------------*/

// list of all devices we manage
static DECLARE_MUTEX (usbnet_mutex);
static LIST_HEAD (usbnet_list);

// randomly generated ethernet address
static u8	node_id [ETH_ALEN];

// state we keep for each device we handle
struct usbnet {
	// housekeeping
	struct usb_device	*udev;
	struct driver_info	*driver_info;
	struct semaphore	mutex;
	struct list_head	dev_list;
	wait_queue_head_t	*wait;

	// protocol/interface state
	struct net_device	net;
	struct net_device_stats	stats;
	int			msg_level;

	// various kinds of pending driver work
	struct sk_buff_head	rxq;
	struct sk_buff_head	txq;
	struct sk_buff_head	done;
	struct tasklet_struct	bh;

	struct tq_struct	kevent;
	unsigned long		flags;
#		define EVENT_TX_HALT	0
#		define EVENT_RX_HALT	1
#		define EVENT_RX_MEMORY	2
};

// device-specific info used by the driver
struct driver_info {
	char		*description;

	int		flags;
#define FLAG_FRAMING_NC	0x0001		/* guard against device dropouts */ 
#define FLAG_FRAMING_GL	0x0002		/* genelink batches packets */
#define FLAG_NO_SETINT	0x0010		/* device can't set_interface() */

	/* reset device ... can sleep */
	int	(*reset)(struct usbnet *);

	/* see if peer is connected ... can sleep */
	int	(*check_connect)(struct usbnet *);

	/* fixup rx packet (strip framing) */
	int	(*rx_fixup)(struct usbnet *dev, struct sk_buff *skb);

	/* fixup tx packet (add framing) */
	struct sk_buff	*(*tx_fixup)(struct usbnet *dev,
				struct sk_buff *skb, int flags);

	// FIXME -- also an interrupt mechanism
	// useful for at least PL2301/2302 and GL620USB-A

	/* framework currently "knows" bulk EPs talk packets */
	int		in;		/* rx endpoint */
	int		out;		/* tx endpoint */
	int		epsize;
};

#define EP_SIZE(usbnet)	((usbnet)->driver_info->epsize)

// we record the state for each of our queued skbs
enum skb_state {
	illegal = 0,
	tx_start, tx_done,
	rx_start, rx_done, rx_cleanup
};

struct skb_data {	// skb->cb is one of these
	struct urb		*urb;
	struct usbnet		*dev;
	enum skb_state		state;
	size_t			length;
};

static const char driver_name [] = "usbnet";

/* use ethtool to change the level for any given device */
static int msg_level = 1;
MODULE_PARM (msg_level, "i");
MODULE_PARM_DESC (msg_level, "Initial message level (default = 1)");


#define	mutex_lock(x)	down(x)
#define	mutex_unlock(x)	up(x)

#define	RUN_CONTEXT (in_irq () ? "in_irq" \
			: (in_interrupt () ? "in_interrupt" : "can sleep"))

/*-------------------------------------------------------------------------*/

#ifdef DEBUG
#define devdbg(usbnet, fmt, arg...) \
	printk(KERN_DEBUG "%s: " fmt "\n" , (usbnet)->net.name, ## arg)
#else
#define devdbg(usbnet, fmt, arg...) do {} while(0)
#endif

#define devinfo(usbnet, fmt, arg...) \
	do { if ((usbnet)->msg_level >= 1) \
	printk(KERN_INFO "%s: " fmt "\n" , (usbnet)->net.name, ## arg); \
	} while (0)


#ifdef	CONFIG_USB_LINUXDEV

/*-------------------------------------------------------------------------
 *
 * This could talk to a device that uses Linux, such as a PDA or
 * an embedded system, or in fact to any "smart" device using this
 * particular mapping of USB and Ethernet.
 *
 * Such a Linux host would need a "USB Device Controller" hardware
 * (not "USB Host Controller"), and a network driver talking to that
 * hardware.
 *
 * One example is Intel's SA-1100 chip, which integrates basic USB
 * support (arch/arm/sa1100/usb-eth.c); it's used in the iPaq PDA.
 * And others too, like the Yopy.
 *
 *-------------------------------------------------------------------------*/

static const struct driver_info	linuxdev_info = {
	.description =	"Linux Device",

	.in = 2, .out = 1,
	.epsize = 64,
};

#endif	/* CONFIG_USB_LINUXDEV */



/*-------------------------------------------------------------------------
 *
 * Network Device Driver (peer link to "Host Device", from USB host)
 *
 *-------------------------------------------------------------------------*/

static int usbnet_change_mtu (struct net_device *net, int new_mtu)
{
	struct usbnet	*dev = (struct usbnet *) net->priv;

	if (new_mtu <= MIN_PACKET || new_mtu > MAX_PACKET)
		return -EINVAL;
	// no second zero-length packet read wanted after mtu-sized packets
	if (((new_mtu + sizeof (struct ethhdr)) % EP_SIZE (dev)) == 0)
		return -EDOM;
	net->mtu = new_mtu;
	return 0;
}

/*-------------------------------------------------------------------------*/

static struct net_device_stats *usbnet_get_stats (struct net_device *net)
{
	return &((struct usbnet *) net->priv)->stats;
}

/*-------------------------------------------------------------------------*/

/* urb completions are currently in_irq; avoid doing real work then. */

static void defer_bh (struct usbnet *dev, struct sk_buff *skb)
{
	struct sk_buff_head	*list = skb->list;
	unsigned long		flags;

	spin_lock_irqsave (&list->lock, flags);
	__skb_unlink (skb, list);
	spin_unlock (&list->lock);
	spin_lock (&dev->done.lock);
	__skb_queue_tail (&dev->done, skb);
	if (dev->done.qlen == 1)
		tasklet_schedule (&dev->bh);
	spin_unlock_irqrestore (&dev->done.lock, flags);
}

/* some work can't be done in tasklets, so we use keventd
 *
 * NOTE:  annoying asymmetry:  if it's active, schedule_task() fails,
 * but tasklet_schedule() doesn't.  hope the failure is rare.
 */
static void defer_kevent (struct usbnet *dev, int work)
{
	set_bit (work, &dev->flags);
	if (!schedule_task (&dev->kevent))
		err ("%s: kevent %d may have been dropped",
			dev->net.name, work);
	else
		dbg ("%s: kevent %d scheduled", dev->net.name, work);
}

/*-------------------------------------------------------------------------*/

static void rx_complete (struct urb *urb);

static void rx_submit (struct usbnet *dev, struct urb *urb, int flags)
{
	struct sk_buff		*skb;
	struct skb_data		*entry;
	int			retval = 0;
	unsigned long		lockflags;
	size_t			size;

	size = (sizeof (struct ethhdr) + dev->net.mtu);

	if ((skb = alloc_skb (size, flags)) == 0) {
		dbg ("no rx skb");
		defer_kevent (dev, EVENT_RX_MEMORY);
		usb_free_urb (urb);
		return;
	}

	entry = (struct skb_data *) skb->cb;
	entry->urb = urb;
	entry->dev = dev;
	entry->state = rx_start;
	entry->length = 0;

	FILL_BULK_URB (urb, dev->udev,
		usb_rcvbulkpipe (dev->udev, dev->driver_info->in),
		skb->data, size, rx_complete, skb);
	urb->transfer_flags |= USB_ASYNC_UNLINK;
#if 0
	// Idle-but-posted reads with UHCI really chew up
	// PCI bandwidth unless FSBR is disabled
	urb->transfer_flags |= USB_NO_FSBR;
#endif

	spin_lock_irqsave (&dev->rxq.lock, lockflags);

	if (netif_running (&dev->net)
			&& !test_bit (EVENT_RX_HALT, &dev->flags)) {
		switch (retval = SUBMIT_URB (urb, GFP_ATOMIC)){ 
		case -EPIPE:
			defer_kevent (dev, EVENT_RX_HALT);
			break;
		case -ENOMEM:
			defer_kevent (dev, EVENT_RX_MEMORY);
			break;
		default:
			dbg ("%s rx submit, %d", dev->net.name, retval);
			tasklet_schedule (&dev->bh);
			break;
		case 0:
			__skb_queue_tail (&dev->rxq, skb);
		}
	} else {
		dbg ("rx: stopped");
		retval = -ENOLINK;
	}
	spin_unlock_irqrestore (&dev->rxq.lock, lockflags);
	if (retval) {
		dev_kfree_skb_any (skb);
		usb_free_urb (urb);
	}
}


/*-------------------------------------------------------------------------*/

static inline void rx_process (struct usbnet *dev, struct sk_buff *skb)
{
	if (dev->driver_info->rx_fixup
			&& !dev->driver_info->rx_fixup (dev, skb))
		goto error;
	// else network stack removes extra byte if we forced a short packet

	if (skb->len) {
		int	status;

// FIXME: eth_copy_and_csum "small" packets to new SKB (small < ~200 bytes) ?

		skb->dev = &dev->net;
		skb->protocol = eth_type_trans (skb, &dev->net);
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += skb->len;

#ifdef	VERBOSE
		devdbg (dev, "< rx, len %d, type 0x%x",
			skb->len + sizeof (struct ethhdr), skb->protocol);
#endif
		memset (skb->cb, 0, sizeof (struct skb_data));
		status = netif_rx (skb);
		if (status != NET_RX_SUCCESS)
			devdbg (dev, "netif_rx status %d", status);
	} else {
		dbg ("drop");
error:
		dev->stats.rx_errors++;
		skb_queue_tail (&dev->done, skb);
	}
}

/*-------------------------------------------------------------------------*/

static void rx_complete (struct urb *urb)
{
	struct sk_buff		*skb = (struct sk_buff *) urb->context;
	struct skb_data		*entry = (struct skb_data *) skb->cb;
	struct usbnet		*dev = entry->dev;
	int			urb_status = urb->status;

	skb_put (skb, urb->actual_length);
	entry->state = rx_done;
	entry->urb = 0;

	switch (urb_status) {
	    // success
	    case 0:
		if (MIN_PACKET > skb->len || skb->len > MAX_PACKET) {
			entry->state = rx_cleanup;
			dev->stats.rx_errors++;
			dev->stats.rx_length_errors++;
			dbg ("rx length %d", skb->len);
		}
		break;

	    // stalls need manual reset. this is rare ... except that
	    // when going through USB 2.0 TTs, unplug appears this way.
	    // we avoid the highspeed version of the ETIMEOUT/EILSEQ
	    // storm, recovering as needed.
	    case -EPIPE:
		defer_kevent (dev, EVENT_RX_HALT);
		// FALLTHROUGH

	    // software-driven interface shutdown
	    case -ECONNRESET:		// according to API spec
	    case -ECONNABORTED:		// some (now fixed?) UHCI bugs
		dbg ("%s rx shutdown, code %d", dev->net.name, urb_status);
		entry->state = rx_cleanup;
		// do urb frees only in the tasklet (UHCI has oopsed ...)
		entry->urb = urb;
		urb = 0;
		break;

	    // data overrun ... flush fifo?
	    case -EOVERFLOW:
		dev->stats.rx_over_errors++;
		// FALLTHROUGH
	    
	    default:
		// on unplug we get ETIMEDOUT (ohci) or EILSEQ (uhci)
		// until khubd sees its interrupt and disconnects us.
		// that can easily be hundreds of passes through here.
		entry->state = rx_cleanup;
		dev->stats.rx_errors++;
		dbg ("%s rx: status %d", dev->net.name, urb_status);
		break;
	}

	defer_bh (dev, skb);

	if (urb) {
		if (netif_running (&dev->net)
				&& !test_bit (EVENT_RX_HALT, &dev->flags)) {
			rx_submit (dev, urb, GFP_ATOMIC);
			return;
		}
		usb_free_urb (urb);
	}
#ifdef	VERBOSE
	dbg ("no read resubmitted");
#endif /* VERBOSE */
}

/*-------------------------------------------------------------------------*/

// unlink pending rx/tx; completion handlers do all other cleanup

static int unlink_urbs (struct sk_buff_head *q)
{
	unsigned long		flags;
	struct sk_buff		*skb, *skbnext;
	int			count = 0;

	spin_lock_irqsave (&q->lock, flags);
	for (skb = q->next; skb != (struct sk_buff *) q; skb = skbnext) {
		struct skb_data		*entry;
		struct urb		*urb;
		int			retval;

		entry = (struct skb_data *) skb->cb;
		urb = entry->urb;
		skbnext = skb->next;

		// during some PM-driven resume scenarios,
		// these (async) unlinks complete immediately
		retval = usb_unlink_urb (urb);
		if (retval != -EINPROGRESS && retval != 0)
			dbg ("unlink urb err, %d", retval);
		else
			count++;
	}
	spin_unlock_irqrestore (&q->lock, flags);
	return count;
}


/*-------------------------------------------------------------------------*/

// precondition: never called in_interrupt

static int usbnet_stop (struct net_device *net)
{
	struct usbnet		*dev = (struct usbnet *) net->priv;
	int			temp;
	DECLARE_WAIT_QUEUE_HEAD (unlink_wakeup); 
	DECLARE_WAITQUEUE (wait, current);

	mutex_lock (&dev->mutex);
	netif_stop_queue (net);

	if (dev->msg_level >= 2)
		devinfo (dev, "stop stats: rx/tx %ld/%ld, errs %ld/%ld",
			dev->stats.rx_packets, dev->stats.tx_packets, 
			dev->stats.rx_errors, dev->stats.tx_errors
			);

	// ensure there are no more active urbs
	add_wait_queue (&unlink_wakeup, &wait);
	dev->wait = &unlink_wakeup;
	temp = unlink_urbs (&dev->txq) + unlink_urbs (&dev->rxq);

	// maybe wait for deletions to finish.
	while (skb_queue_len (&dev->rxq)
			&& skb_queue_len (&dev->txq)
			&& skb_queue_len (&dev->done)) {
		set_current_state (TASK_UNINTERRUPTIBLE);
		schedule_timeout (UNLINK_TIMEOUT_JIFFIES);
		dbg ("waited for %d urb completions", temp);
	}
	dev->wait = 0;
	remove_wait_queue (&unlink_wakeup, &wait); 

	mutex_unlock (&dev->mutex);
	return 0;
}

/*-------------------------------------------------------------------------*/

// posts reads, and enables write queing

// precondition: never called in_interrupt

static int usbnet_open (struct net_device *net)
{
	struct usbnet		*dev = (struct usbnet *) net->priv;
	int			retval = 0;
	struct driver_info	*info = dev->driver_info;

	mutex_lock (&dev->mutex);

	// put into "known safe" state
	if (info->reset && (retval = info->reset (dev)) < 0) {
		devinfo (dev, "open reset fail (%d) usbnet usb-%s-%s, %s",
			retval,
			dev->udev->bus->bus_name, dev->udev->devpath,
			info->description);
		goto done;
	}

	// insist peer be connected
	if (info->check_connect && (retval = info->check_connect (dev)) < 0) {
		devdbg (dev, "can't open; %d", retval);
		goto done;
	}

	netif_start_queue (net);
	if (dev->msg_level >= 2)
		devinfo (dev, "open: enable queueing "
				"(rx %d, tx %d) mtu %d %s framing",
			RX_QLEN, TX_QLEN, dev->net.mtu,
			(info->flags & (FLAG_FRAMING_NC | FLAG_FRAMING_GL))
			    ? ((info->flags & FLAG_FRAMING_NC)
				? "NetChip"
				: "GeneSys")
			    : "raw"
			);

	// delay posting reads until we're fully open
	tasklet_schedule (&dev->bh);
done:
	mutex_unlock (&dev->mutex);
	return retval;
}

/*-------------------------------------------------------------------------*/

static int usbnet_ethtool_ioctl (struct net_device *net, void *useraddr)
{
	struct usbnet	*dev = (struct usbnet *) net->priv;
	u32		cmd;

	if (get_user (cmd, (u32 *)useraddr))
		return -EFAULT;
	switch (cmd) {

	case ETHTOOL_GDRVINFO: {	/* get driver info */
		struct ethtool_drvinfo		info;

		memset (&info, 0, sizeof info);
		info.cmd = ETHTOOL_GDRVINFO;
		strncpy (info.driver, driver_name, sizeof info.driver);
		strncpy (info.version, DRIVER_VERSION, sizeof info.version);
		strncpy (info.fw_version, dev->driver_info->description,
			sizeof info.fw_version);
		usb_make_path (dev->udev, info.bus_info, sizeof info.bus_info);
		if (copy_to_user (useraddr, &info, sizeof (info)))
			return -EFAULT;
		return 0;
		}

	case ETHTOOL_GLINK: 		/* get link status */
		if (dev->driver_info->check_connect) {
			struct ethtool_value	edata = { ETHTOOL_GLINK };

			edata.data = dev->driver_info->check_connect (dev) == 0;
			if (copy_to_user (useraddr, &edata, sizeof (edata)))
				return -EFAULT;
			return 0;
		}
		break;

	case ETHTOOL_GMSGLVL: {		/* get message-level */
		struct ethtool_value	edata = {ETHTOOL_GMSGLVL};

		edata.data = dev->msg_level;
		if (copy_to_user (useraddr, &edata, sizeof (edata)))
			return -EFAULT;
		return 0;
		}

	case ETHTOOL_SMSGLVL: {		/* set message-level */
		struct ethtool_value	edata;

		if (copy_from_user (&edata, useraddr, sizeof (edata)))
			return -EFAULT;
		dev->msg_level = edata.data;
		return 0;
		}
	
	/* could also map RINGPARAM to RX/TX QLEN */

	}
        /* Note that the ethtool user space code requires EOPNOTSUPP */
	return -EOPNOTSUPP;
}

static int usbnet_ioctl (struct net_device *net, struct ifreq *rq, int cmd)
{
	switch (cmd) {
	case SIOCETHTOOL:
		return usbnet_ethtool_ioctl (net, (void *)rq->ifr_data);
	default:
		return -EOPNOTSUPP;
	}
}

/*-------------------------------------------------------------------------*/

/* work that cannot be done in interrupt context uses keventd.
 *
 * NOTE:  "uhci" and "usb-uhci" may have trouble with this since they don't
 * queue control transfers to individual devices, and other threads could
 * trigger control requests concurrently.  hope that's rare.
 */
static void
kevent (void *data)
{
	struct usbnet		*dev = data;
	int			status;

	/* usb_clear_halt() needs a thread context */
	if (test_bit (EVENT_TX_HALT, &dev->flags)) {
		unlink_urbs (&dev->txq);
		status = usb_clear_halt (dev->udev,
			usb_sndbulkpipe (dev->udev, dev->driver_info->out));
		if (status < 0)
			err ("%s: can't clear tx halt, status %d",
				dev->net.name, status);
		else {
			clear_bit (EVENT_TX_HALT, &dev->flags);
			netif_wake_queue (&dev->net);
		}
	}
	if (test_bit (EVENT_RX_HALT, &dev->flags)) {
		unlink_urbs (&dev->rxq);
		status = usb_clear_halt (dev->udev,
			usb_rcvbulkpipe (dev->udev, dev->driver_info->in));
		if (status < 0)
			err ("%s: can't clear rx halt, status %d",
				dev->net.name, status);
		else {
			clear_bit (EVENT_RX_HALT, &dev->flags);
			tasklet_schedule (&dev->bh);
		}
	}

	/* tasklet could resubmit itself forever if memory is tight */
	if (test_bit (EVENT_RX_MEMORY, &dev->flags)) {
		struct urb	*urb = 0;

		if (netif_running (&dev->net))
			urb = ALLOC_URB (0, GFP_KERNEL);
		else
			clear_bit (EVENT_RX_MEMORY, &dev->flags);
		if (urb != 0) {
			clear_bit (EVENT_RX_MEMORY, &dev->flags);
			rx_submit (dev, urb, GFP_KERNEL);
			tasklet_schedule (&dev->bh);
		}
	}

	if (dev->flags)
		dbg ("%s: kevent done, flags = 0x%lx",
			dev->net.name, dev->flags);
}

/*-------------------------------------------------------------------------*/

static void tx_complete (struct urb *urb)
{
	struct sk_buff		*skb = (struct sk_buff *) urb->context;
	struct skb_data		*entry = (struct skb_data *) skb->cb;
	struct usbnet		*dev = entry->dev;

	if (urb->status == -EPIPE)
		defer_kevent (dev, EVENT_TX_HALT);
	urb->dev = 0;
	entry->state = tx_done;
	defer_bh (dev, skb);
}

/*-------------------------------------------------------------------------*/

static void usbnet_tx_timeout (struct net_device *net)
{
	struct usbnet		*dev = (struct usbnet *) net->priv;

	unlink_urbs (&dev->txq);
	tasklet_schedule (&dev->bh);

	// FIXME: device recovery -- reset?
}

/*-------------------------------------------------------------------------*/

static int usbnet_start_xmit (struct sk_buff *skb, struct net_device *net)
{
	struct usbnet		*dev = (struct usbnet *) net->priv;
	int			length = skb->len;
	int			retval = NET_XMIT_SUCCESS;
	struct urb		*urb = 0;
	struct skb_data		*entry;
	struct driver_info	*info = dev->driver_info;
	unsigned long		flags;

	// some devices want funky USB-level framing, for
	// win32 driver (usually) and/or hardware quirks
	if (info->tx_fixup) {
		skb = info->tx_fixup (dev, skb, GFP_ATOMIC);
		if (!skb) {
			dbg ("can't tx_fixup skb");
			goto drop;
		}
	}

	if (!(urb = ALLOC_URB (0, GFP_ATOMIC))) {
		dbg ("no urb");
		goto drop;
	}

	entry = (struct skb_data *) skb->cb;
	entry->urb = urb;
	entry->dev = dev;
	entry->state = tx_start;
	entry->length = length;

	// FIXME: reorganize a bit, so that fixup() fills out NetChip
	// framing too. (Packet ID update needs the spinlock...)


	/* don't assume the hardware handles USB_ZERO_PACKET */
	if ((length % EP_SIZE (dev)) == 0)
		skb->len++;

	FILL_BULK_URB (urb, dev->udev,
			usb_sndbulkpipe (dev->udev, info->out),
			skb->data, skb->len, tx_complete, skb);
	urb->transfer_flags |= USB_ASYNC_UNLINK;
	// FIXME urb->timeout = ... jiffies ... ;

	spin_lock_irqsave (&dev->txq.lock, flags);

	switch ((retval = SUBMIT_URB (urb, GFP_ATOMIC))) {
	case -EPIPE:
		netif_stop_queue (net);
		defer_kevent (dev, EVENT_TX_HALT);
		break;
	default:
		dbg ("%s tx: submit urb err %d", net->name, retval);
		break;
	case 0:
		net->trans_start = jiffies;
		__skb_queue_tail (&dev->txq, skb);
		if (dev->txq.qlen >= TX_QLEN)
			netif_stop_queue (net);
	}
	spin_unlock_irqrestore (&dev->txq.lock, flags);

	if (retval) {
		devdbg (dev, "drop, code %d", retval);
drop:
		retval = NET_XMIT_DROP;
		dev->stats.tx_dropped++;
		if (skb)
			dev_kfree_skb_any (skb);
		usb_free_urb (urb);
#ifdef	VERBOSE
	} else {
		devdbg (dev, "> tx, len %d, type 0x%x",
			length, skb->protocol);
#endif
	}
	return retval;
}


/*-------------------------------------------------------------------------*/

// tasklet ... work that avoided running in_irq()

static void usbnet_bh (unsigned long param)
{
	struct usbnet		*dev = (struct usbnet *) param;
	struct sk_buff		*skb;
	struct skb_data		*entry;

	while ((skb = skb_dequeue (&dev->done))) {
		entry = (struct skb_data *) skb->cb;
		switch (entry->state) {
		    case rx_done:
			entry->state = rx_cleanup;
			rx_process (dev, skb);
			continue;
		    case tx_done:
			if (entry->urb->status) {
				// can this statistic become more specific?
				dev->stats.tx_errors++;
				dbg ("%s tx: err %d", dev->net.name,
					entry->urb->status);
			} else {
				dev->stats.tx_packets++;
				dev->stats.tx_bytes += entry->length;
			}
			// FALLTHROUGH:
		    case rx_cleanup:
			usb_free_urb (entry->urb);
			dev_kfree_skb (skb);
			continue;
		    default:
			dbg ("%s: bogus skb state %d",
				dev->net.name, entry->state);
		}
	}

	// waiting for all pending urbs to complete?
	if (dev->wait) {
		if ((dev->txq.qlen + dev->rxq.qlen + dev->done.qlen) == 0) {
			wake_up (dev->wait);
		}

	// or are we maybe short a few urbs?
	} else if (netif_running (&dev->net)
			&& !test_bit (EVENT_RX_HALT, &dev->flags)) {
		int	temp = dev->rxq.qlen;

		if (temp < RX_QLEN) {
			struct urb	*urb;
			int		i;
			for (i = 0; i < 3 && dev->rxq.qlen < RX_QLEN; i++) {
				if ((urb = ALLOC_URB (0, GFP_ATOMIC)) != 0)
					rx_submit (dev, urb, GFP_ATOMIC);
			}
			if (temp != dev->rxq.qlen)
				devdbg (dev, "rxqlen %d --> %d",
						temp, dev->rxq.qlen);
			if (dev->rxq.qlen < RX_QLEN)
				tasklet_schedule (&dev->bh);
		}
		if (dev->txq.qlen < TX_QLEN)
			netif_wake_queue (&dev->net);
	}
}



/*-------------------------------------------------------------------------
 *
 * USB Device Driver support
 *
 *-------------------------------------------------------------------------*/
 
// precondition: never called in_interrupt

static void usbnet_disconnect (struct usb_device *udev, void *ptr)
{
	struct usbnet	*dev = (struct usbnet *) ptr;

	devinfo (dev, "unregister usbnet usb-%s-%s, %s",
		udev->bus->bus_name, udev->devpath,
		dev->driver_info->description);
	
	unregister_netdev (&dev->net);

	mutex_lock (&usbnet_mutex);
	mutex_lock (&dev->mutex);
	list_del (&dev->dev_list);
	mutex_unlock (&usbnet_mutex);

	// assuming we used keventd, it must quiesce too
	flush_scheduled_tasks ();

	kfree (dev);
	usb_put_dev (udev);
}


/*-------------------------------------------------------------------------*/

// precondition: never called in_interrupt

static void *
usbnet_probe (struct usb_device *udev, unsigned ifnum,
			const struct usb_device_id *prod)
{
	struct usbnet			*dev;
	struct net_device 		*net;
	struct usb_interface_descriptor	*interface;
	struct driver_info		*info;
	int				altnum = 0;

	info = (struct driver_info *) prod->driver_info;

	// sanity check; expect dedicated interface/devices for now.
	interface = &udev->actconfig->interface [ifnum].altsetting [altnum];
	if (udev->descriptor.bNumConfigurations != 1
			|| udev->config[0].bNumInterfaces != 1
//			|| interface->bInterfaceClass != USB_CLASS_VENDOR_SPEC
			) {
		dbg ("Bogus config info");
		return 0;
	}

	// more sanity (unless the device is broken)
	if (!(info->flags & FLAG_NO_SETINT)) {
		if (usb_set_interface (udev, ifnum, altnum) < 0) {
			err ("set_interface failed");
			return 0;
		}
	}

	// set up our own records
	if (!(dev = kmalloc (sizeof *dev, GFP_KERNEL))) {
		dbg ("can't kmalloc dev");
		return 0;
	}
	memset (dev, 0, sizeof *dev);

	init_MUTEX_LOCKED (&dev->mutex);
	usb_get_dev (udev);
	dev->udev = udev;
	dev->driver_info = info;
	dev->msg_level = msg_level;
	INIT_LIST_HEAD (&dev->dev_list);
	skb_queue_head_init (&dev->rxq);
	skb_queue_head_init (&dev->txq);
	skb_queue_head_init (&dev->done);
	dev->bh.func = usbnet_bh;
	dev->bh.data = (unsigned long) dev;
	INIT_TQUEUE (&dev->kevent, kevent, dev);

	// set up network interface records
	net = &dev->net;
	SET_MODULE_OWNER (net);
	net->priv = dev;
	strcpy (net->name, "usb%d");
	memcpy (net->dev_addr, node_id, sizeof node_id);

	// point-to-point link ... we always use Ethernet headers 
	// supports win32 interop and the bridge driver.
	ether_setup (net);

	net->change_mtu = usbnet_change_mtu;
	net->get_stats = usbnet_get_stats;
	net->hard_start_xmit = usbnet_start_xmit;
	net->open = usbnet_open;
	net->stop = usbnet_stop;
	net->watchdog_timeo = TX_TIMEOUT_JIFFIES;
	net->tx_timeout = usbnet_tx_timeout;
	net->do_ioctl = usbnet_ioctl;

	register_netdev (&dev->net);
	devinfo (dev, "register usbnet usb-%s-%s, %s",
		udev->bus->bus_name, udev->devpath,
		dev->driver_info->description);

	// ok, it's ready to go.
	mutex_lock (&usbnet_mutex);
	list_add (&dev->dev_list, &usbnet_list);
	mutex_unlock (&dev->mutex);

	// start as if the link is up
	netif_device_attach (&dev->net);

	mutex_unlock (&usbnet_mutex);
	return dev;
}


/*-------------------------------------------------------------------------*/

/*
 * chip vendor names won't normally be on the cables, and
 * may not be on the device.
 */

static const struct usb_device_id	products [] = {

#ifdef	CONFIG_USB_LINUXDEV
/*
 * for example, this can be a host side talk-to-PDA driver.
 * this driver is NOT what runs _inside_ a Linux device !!
 */
{
	// 1183 = 0x049F, both used as hex values?
	USB_DEVICE (0x049F, 0x505A),	// Compaq "Itsy"
	.driver_info =	(unsigned long) &linuxdev_info,
}, {
	USB_DEVICE (0x0E7E, 0x1001),	// G.Mate "Yopy"
	.driver_info =	(unsigned long) &linuxdev_info,
},
	// NOTE:  the Sharp Zaurus uses a modified version of
	// this driver, which is not interoperable with this.
#endif

	{ },		// END
};
MODULE_DEVICE_TABLE (usb, products);

static struct usb_driver usbnet_driver = {
	.name =		driver_name,
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
};

/*-------------------------------------------------------------------------*/

static int __init usbnet_init (void)
{
	// compiler should optimize this out
	if (sizeof (((struct sk_buff *)0)->cb) < sizeof (struct skb_data))
		BUG ();

	get_random_bytes (node_id, sizeof node_id);
	node_id [0] &= 0xfe;	// clear multicast bit
	node_id [0] |= 0x02;    // set local assignment bit (IEEE802)

 	if (usb_register (&usbnet_driver) < 0)
 		return -1;

	return 0;
}
module_init (usbnet_init);

static void __exit usbnet_exit (void)
{
 	usb_deregister (&usbnet_driver);
}
module_exit (usbnet_exit);

EXPORT_NO_SYMBOLS;
MODULE_AUTHOR ("David Brownell <dbrownell@users.sourceforge.net>");
MODULE_DESCRIPTION ("USB Host-to-Host Link Drivers (numerous vendors)");
MODULE_LICENSE ("GPL");
