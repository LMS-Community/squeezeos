
/*
 *  S3C24x0 USB controller core driver.
 *  2004 (C) Samsung Electronics
 *          SW.LEE : Added "ud" interface to Linux (S3C2440,S3C24A0)
 *          Sree  : Added DNW interface
 *
 *  This program is derived from Extenex Corporation's SA-1100 usb
 *  controller core driver by MIZI.  Seungbum Lim <shawn@mizi.com>
 *
 */

/*
 * ep0 - register
 * ep2~4 - dual port async. RAM (interrupt or DMA)
 *
 * config:
 *  ep0.
 *  ep2 : input - DMA_CH0 ?
 *  ep1 : output - DMA_CH3 ?
 */

#include <common.h>
#include <command.h>
#include <asm/errno.h>
#include "usb_ctl.h"
#include <s3c24x0_usb.h>
//#if defined(CONFIG_S3C2460x)
//#include <asm/arch/s3c2460.h>
//#else
//#include <asm/arch/s3c24a0.h>
//#endif
#include <asm/arch/irqs.h>

#include <asm-arm/arch-arm926ejs/s3c2413.h>


#define EINVAL          22	/* Invalid argument */

//////////////////////////////////////////////////////////////////////////////
// Prototypes
//////////////////////////////////////////////////////////////////////////////
int usbctl_next_state_on_event(int event);
//static void udc_int_hndlr(int, void *, struct pt_regs *);
void udc_int_hndlr(void);
static void initialize_descriptors(void);
void ChangeUPllValue(int mdiv, int pdiv, int sdiv);
void reset_usbd(void);
void reconfig_usbd(void);



unsigned int usbd_dn_cnt;
unsigned int usbd_dn_addr;
int DNW;


//#define USB_DEBUG 1

#ifdef USB_DEBUG
#define LOG(arg...) printf(__FILE__":"__FUNCTION__"(): " ##arg)
#else
#define LOG(arg...) (void)(0)
#endif

#if CONFIG_PROC_FS
#define PROC_NODE_NAME "usb"
static int usbctl_read_proc(char *page, char **start, off_t off,
			    int count, int *eof, void *data);
#endif

//////////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////////
static const char pszMe[] = "usbctl: ";
struct usb_info_t usbd_info;	/* global to ep0, usb_recv, usb_send */
//static int cnt=0;


/* device descriptors */
static desc_t desc;

#define MAX_STRING_DESC 8
static string_desc_t *string_desc_array[MAX_STRING_DESC];
static string_desc_t sd_zero;	/* special sd_zero holds language codes */

// called when configured
static usb_notify_t configured_callback = NULL;

enum { kStateZombie = 0, kStateZombieSuspend = 1,
	kStateDefault = 2, kStateDefaultSuspend = 3,
	kStateAddr = 4, kStateAddrSuspend = 5,
	kStateConfig = 6, kStateConfigSuspend = 7
};

#if 0
static int device_state_machine[8][6] = {
//                suspend               reset          resume     adddr config deconfig
/* zombie */ {kStateZombieSuspend, kStateDefault, kError, kError,
		      kError, kError},
/* zom sus */ {kError, kStateDefault, kStateZombie, kError, kError,
		       kError},
/* default */ {kStateDefaultSuspend, kError, kStateDefault,
		       kStateAddr, kError, kError},
/* def sus */ {kError, kStateDefault, kStateDefault, kError,
		       kError, kError},
/* addr */ {kStateAddrSuspend, kStateDefault, kError, kError,
		    kStateConfig, kError},
/* addr sus */ {kError, kStateDefault, kStateAddr, kError, kError,
			kError},
/* config */ {kStateConfigSuspend, kStateDefault, kError, kError,
		      kError, kStateAddr},
/* cfg sus */ {kError, kStateDefault, kStateConfig, kError, kError,
		       kError}
};

/* "device state" is the usb device framework state, as opposed to the
   "state machine state" which is whatever the driver needs and is much
   more fine grained
*/
static int sm_state_to_device_state[8] =
//  zombie           zom suspend          default            default sus
{ USB_STATE_POWERED, USB_STATE_SUSPENDED, USB_STATE_DEFAULT,
	    USB_STATE_SUSPENDED,
// addr              addr sus             config                config sus
	USB_STATE_ADDRESS, USB_STATE_SUSPENDED, USB_STATE_CONFIGURED,
	    USB_STATE_SUSPENDED
};

static char *state_names[8] =
    { "zombie", "zombie suspended", "default", "default suspended",
	"address", "address suspended", "configured", "config suspended"
};

static char *event_names[6] = { "suspend", "reset", "resume",
	"address assigned", "configure", "de-configure"
};

static char *device_state_names[] =
    { "not attached", "attached", "powered", "default",
	"address", "configured", "suspended"
};

static int sm_state = kStateZombie;
#endif

//////////////////////////////////////////////////////////////////////////////
// Reset Fucntions
//////////////////////////////////////////////////////////////////////////////

void reset_usbd(void)
{
	int i;

	UD_PWR = UD_PWR_DEFAULT | UD_PWR_RESET;	/* UD_PWR default value, MCU_RESET */
	UD_PWR;
	UD_PWR = UD_PWR_DEFAULT;

	LOG("UD_PWR = 0x%08x\n", UD_PWR);

	for (i = 0; i < 0x100; i++);
}


void reconfig_usbd(void)
{
	LOG("\n");

	/* sec like, shawn */
	ep0_state = EP0_STATE_IDLE;
	set_configuration = 1;
	set_interface = 1;
	device_status = 0;
	ep0_status = 0;
	ep_bulk_in_status = 0;
	ep_bulk_out_status = 0;

	UD_PWR = UD_PWR_DEFAULT;
	/* EP0 */
	UD_INDEX = UD_INDEX_EP0;
	UD_MAXP = UD_MAXP_8;	// 8 byte
	UD_INDEX = UD_INDEX_EP0;
	UD_ICSR1 = EP0_CSR_SOPKTRDY | EP0_CSR_SSE;

	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {		/* EP2 */
		UD_INDEX = UD_INDEX_EP2;
	}

	UD_MAXP = UD_MAXP_64;	// 64 byte
	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}
	UD_ICSR1 = UD_ICSR1_FFLUSH | UD_ICSR1_CLRDT;	// fifo flush, data toggle

	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;
	}

	if (DNW) {
		UD_ICSR2 = UD_ICSR2_DMAIEN;
	} else {
		UD_ICSR2 = UD_ICSR2_MODEIN | UD_ICSR2_DMAIEN;	/* input mode, IN_PKT_RDY dis */
	}
#ifdef USE_USBD_DMA
	UD_ICSR2 &= ~UD_ICSR2_DMAIEN;
#endif

	/* EP1 */
	UD_INDEX = UD_INDEX_EP1;
	UD_MAXP = UD_MAXP_64;	// 64 byte
	UD_INDEX = UD_INDEX_EP1;
	UD_ICSR1 = UD_ICSR1_FFLUSH | UD_ICSR1_CLRDT;	// fifo flush, data toggle
	UD_INDEX = UD_INDEX_EP1;
	UD_ICSR2 = 0x0;		// output mode
	UD_INDEX = UD_INDEX_EP1;
	UD_OCSR1 = UD_OCSR1_FFLUSH | UD_OCSR1_CLRDT;	// fifo flush
	UD_INDEX = UD_INDEX_EP1;
	UD_OCSR2 = UD_OCSR2_DMAIEN;	// OUT_PKT_RDY interrupt disable
#ifdef USE_USBD_DMA
	UD_OCSR2 &= ~UD_OCSR2_DMAIEN;	// OUT_PKT_RDY interrupt disable
#endif

	if (DNW) {
		UD_INTE = UD_INTE_EP0 | UD_INTE_EP3 | UD_INTE_EP1;
	} else {
		UD_INTE = UD_INTE_EP0 | UD_INTE_EP2 | UD_INTE_EP1;
	}
	UD_USBINTE = UD_USBINTE_RESET | UD_USBINTE_SUSPND;

	initialize_descriptors();
#if defined(CONFIG_S3C2460x)
	INTMSK &= ~(BIT_USBD_2460);
#else
	INTMSK &= ~(BIT_USBD);
#endif
}


void udc_int_hndlr(void)
{
	__u8 saveIdx = UD_INDEX;
	__u8 usb_status = UD_USBINT;
	__u8 usbd_status = UD_INT;

	LOG("usb_status = 0x%02x, usbd_status = 0x%02x\n", usb_status,
	    usbd_status);
	if (usb_status & UD_USBINT_RESET) {

		LOG("RESET interrupt\n");
		if (usbctl_next_state_on_event(kEvReset) != kError) {
			LOG("%s Resetting\n", pszMe);

			ep0_reset();
			ep1_reset();	/* output */
			ep2_reset();	/* input */

		}
		// reset_usbd();
		reconfig_usbd();
		UD_USBINT = UD_USBINT_RESET;	//RESET_INT should be cleared after reconfig_usbd().- by samsung src
		ep0_state = EP0_STATE_IDLE;

	}


	/* RESume Interrupt Request */
	if (usb_status & UD_USBINT_RESUM) {
		LOG("RESUME interrupt\n");
		UD_USBINT = UD_USBINT_RESUM;	/* clear */
		usbctl_next_state_on_event(kEvResume);

	}


	/* SUSpend Interrupt Request */
	if (usb_status & UD_USBINT_SUSPND) {
		LOG("SUSPEND interrupt\n");
		UD_USBINT = UD_USBINT_SUSPND;	/* clear */
		usbctl_next_state_on_event(kEvSuspend);

	}

	if (usbd_status & UD_INT_EP0) {
		LOG("EP0 interrupt\n");
		UD_INT = UD_INT_EP0;	/* clear */
		ep0_int_hndlr();

	}

	/* output */
	if (usbd_status & UD_INT_EP1) {
		LOG("EP1 interrupt\n");
		UD_INT = UD_INT_EP1;	/* clear */
		if (DNW) {
			/* tell me reason ? */
			ep2_int_hndlr(usbd_status);
		} else {
			ep1_int_hndlr(usbd_status);
		}

	}

	if (DNW) {
		/* input */
		if (usbd_status & UD_INT_EP3) {
			LOG("EP2 interrupt\n");
			UD_INT = UD_INT_EP3;	/* clear */
			ep1_int_hndlr(usbd_status);

		}
	} else {
		/* input */
		if (usbd_status & UD_INT_EP2) {
			LOG("EP2 interrupt\n");
			UD_INT = UD_INT_EP2;	/* clear */
			ep2_int_hndlr(usbd_status);
		}
	}

	if (usbd_status & UD_INT_EP3)
		UD_INT = UD_INT_EP3;
	if (usbd_status & UD_INT_EP4)
		UD_INT = UD_INT_EP4;

	//Clear_pending(IRQ_USBD);
	UD_INDEX = saveIdx;
	LOG("end of usb irq \n");
}



//////////////////////////////////////////////////////////////////////////////
// Public Interface
//////////////////////////////////////////////////////////////////////////////

/* Open S3C24x0 usb core on behalf of a client, but don't start running */

int s3c24x0_usb_open(const char *client)
{

	printf("\n");

	if (usbd_info.client_name != NULL)
		return -EBUSY;

	usbd_info.client_name = (char *) client;
	memset(&usbd_info.stats, 0, sizeof(struct usb_stats_t));
	memset(string_desc_array, 0, sizeof(string_desc_array));

	/* hack to start in zombie suspended state */
#if 0
	sm_state = kStateZombieSuspend;
	usbd_info.state = USB_STATE_SUSPENDED;
#endif

	/* create descriptors for enumeration */
	initialize_descriptors();

	//printf("%sOpened for %s\n", pszMe, client);
	return 0;
}

/* Start running. Must have called usb_open (above) first */
int s3c24x0_usb_start(void)
{
	unsigned long tmp;

	printf("\n");

	if (usbd_info.client_name == NULL) {
		printf("%s%s - no client registered\n",
		       pszMe, __FUNCTION__);
		return -EPERM;
	}

	/* start UDC internal machinery running */
	udelay(100);

	/* clear stall - receiver seems to start stalled? */
	if (DNW) {
		UD_INDEX = UD_INDEX_EP3;
	} else {
		UD_INDEX = UD_INDEX_EP2;	// EP2 input 
	}
	tmp = UD_ICSR1;
	tmp &= ~(UD_ICSR1_SENTSTL | UD_ICSR1_FFLUSH | UD_ICSR1_UNDRUN);
	tmp &= ~(UD_ICSR1_PKTRDY | UD_ICSR1_SENDSTL);
	UD_ICSR1 = tmp;

	UD_INDEX = UD_INDEX_EP1;	// EP1 output
	tmp = UD_OCSR1;
	tmp &= ~(UD_OCSR1_SENTSTL | UD_OCSR1_FFLUSH | UD_OCSR1_OVRRUN);
	tmp &= ~(UD_OCSR1_PKTRDY | UD_OCSR1_SENDSTL);
	UD_OCSR1 = tmp;


	/* flush DMA and fire through some -EAGAINs */
	// ep2_init( usbd_info.dmach_tx );
	// ep1_init( usbd_info.dmach_rx );


	/* clear all top-level sources */
	if (DNW) {
		UD_INT = UD_INT_EP0 | UD_INT_EP1 | UD_INT_EP3;
	} else {
		UD_INT = UD_INT_EP0 | UD_INT_EP1 | UD_INT_EP2;
	}
	UD_USBINT = UD_USBINT_RESET | UD_USBINT_RESUM | UD_USBINT_SUSPND;

	//printf("%sStarted for %s\n", pszMe, usbd_info.client_name);
	usbd_dn_cnt = 0;
	//printf("USB Device started for download, counter=%d.\n",
	//       usbd_dn_cnt);
	return 0;
}

/* Stop USB core from running */
int s3c24x0_usb_stop(void)
{
	printf("name=%s\n",
	    usbd_info.client_name ? usbd_info.client_name : "NULL");
	if (usbd_info.client_name == NULL) {
		printf("%s%s - no client registered\n",
		       pszMe, __FUNCTION__);
		return -EPERM;
	}
#if 0
	/* It may be default value of S3C24x0 USBD and makes only RESET be enalble */
	UD_INTM = 0x13f;
#endif
	ep1_reset();
	ep2_reset();
	printf("%sStopped \n", pszMe);
	return 0;
}

/* Tell S3C24x0 core client is through using it */
int s3c24x0_usb_close(void)
{
	if (usbd_info.client_name == NULL) {
		printf("%s%s - no client registered\n",
		       pszMe, __FUNCTION__);
		return -EPERM;
	}
	usbd_info.client_name = NULL;
	return 0;
}

/* set a proc to be called when device is configured */
usb_notify_t s3c24x0_set_configured_callback(usb_notify_t func)
{
	usb_notify_t retval = configured_callback;

	printf("\n");

	configured_callback = func;
	return retval;
}

/*====================================================
 * Descriptor Manipulation.
 * Use these between open() and start() above to setup
 * the descriptors for your device.
 *
 */

/* get pointer to static default descriptor */
desc_t *s3c24x0_usb_get_descriptor_ptr(void)
{
	return &desc;
}

/* optional: set a string descriptor */
int s3c24x0_usb_set_string_descriptor(int i, string_desc_t * p)
{
	int retval;

	printf("\n");

	if (i < MAX_STRING_DESC) {
		string_desc_array[i] = p;
		retval = 0;
	} else {
		retval = -EINVAL;
	}
	return retval;
}

/* optional: get a previously set string descriptor */
string_desc_t *s3c24x0_usb_get_string_descriptor(int i)
{
	printf("\n");

	return (i < MAX_STRING_DESC)
	    ? string_desc_array[i]
	    : NULL;
}


/* optional: kmalloc and unicode up a string descriptor */
string_desc_t *s3c24x0_usb_kmalloc_string_descriptor(const char *p)
{
	string_desc_t *pResult = NULL;

	printf("\n");

	if (p) {
		int len = strlen(p);
		int uni_len = len * sizeof(__u16);
		// pResult = (string_desc_t*) kmalloc( uni_len + 2, GFP_KERNEL ); /* ugh! */
		if (pResult != NULL) {
			int i;
			pResult->bLength = uni_len + 2;
			pResult->bDescriptorType = USB_DESC_STRING;
			for (i = 0; i < len; i++) {
				pResult->bString[i] =
				    make_word((__u16) p[i]);
			}
		}
	}
	return pResult;
}

//////////////////////////////////////////////////////////////////////////////
// Exports to rest of driver
//////////////////////////////////////////////////////////////////////////////

/* called by the int handler here and the two endpoint files when interesting
   .."events" happen */

int usbctl_next_state_on_event(int event)
{
	return 1;
#if 0
	int next_state = device_state_machine[sm_state][event];

	LOG("\n");

	if (next_state != kError) {
		int next_device_state =
		    sm_state_to_device_state[next_state];
		printf("%s%s --> [%s] --> %s. Device in %s state.\n",
		       pszMe, state_names[sm_state], event_names[event],
		       state_names[next_state],
		       device_state_names[next_device_state]);

		sm_state = next_state;
		if (usbd_info.state != next_device_state) {
			if (configured_callback != NULL
			    &&
			    next_device_state == USB_STATE_CONFIGURED
			    && usbd_info.state != USB_STATE_SUSPENDED) {
				configured_callback();
			}
			usbd_info.state = next_device_state;
		}
	}
#if 0
	else
		printf("%s%s --> [%s] --> ??? is an error.\n",
		       pszMe, state_names[sm_state], event_names[event]);
#endif
	return next_state;
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Private Helpers
//////////////////////////////////////////////////////////////////////////////

//* setup default descriptors */

static void initialize_descriptors(void)
{
	//printf("\n initialize_descriptors ");

	desc.dev.bLength = sizeof(device_desc_t);
	desc.dev.bDescriptorType = USB_DESC_DEVICE;
	desc.dev.bcdUSB = 0x100;	/* 1.1 */
	desc.dev.bDeviceClass = 0xFF;	/* vendor specific */
	desc.dev.bDeviceSubClass = 0x0;
	desc.dev.bDeviceProtocol = 0x0;
	desc.dev.bMaxPacketSize0 = 0x8;	/* ep0 max fifo size in s3c24x0 */
	//desc.dev.idVendor = 0x49f;	/* vendor ID undefined */
	//desc.dev.idProduct = 0x505a;	/* product */
	desc.dev.idVendor = 0x5345;	/* vendor ID undefined */
	desc.dev.idProduct = 0x1234;	/* product */

	desc.dev.bcdDevice = 0x01;	/* vendor assigned device release num */
	desc.dev.iManufacturer = 0;	/* index of manufacturer string */
	desc.dev.iProduct = 0;	/* index of product description string */
	desc.dev.iSerialNumber = 0;	/* index of string holding product s/n */
	desc.dev.bNumConfigurations = 0x1;

	desc.b.cfg.bLength = sizeof(config_desc_t);
	desc.b.cfg.bDescriptorType = USB_DESC_CONFIG;
	desc.b.cfg.wTotalLength = make_word_c(sizeof(struct cdb));
	desc.b.cfg.bNumInterfaces = 1;
	desc.b.cfg.bConfigurationValue = 1;
	desc.b.cfg.iConfiguration = 0;
	desc.b.cfg.bmAttributes = USB_CONFIG_BUSPOWERED;
	desc.b.cfg.MaxPower = USB_POWER(500);

	desc.b.intf.bLength = sizeof(intf_desc_t);
	desc.b.intf.bDescriptorType = USB_DESC_INTERFACE;
	desc.b.intf.bInterfaceNumber = 0x0;	/* unique intf index */
	desc.b.intf.bAlternateSetting = 0x0;
	desc.b.intf.bNumEndpoints = 2;	/* endpoint number excluding ep0 */
	desc.b.intf.bInterfaceClass = 0xFF;	/* vendor specific */
	desc.b.intf.bInterfaceSubClass = 0x0;
	desc.b.intf.bInterfaceProtocol = 0x0;
	desc.b.intf.iInterface = 0x0;

	desc.b.ep1.bLength = sizeof(ep_desc_t);
	desc.b.ep1.bDescriptorType = USB_DESC_ENDPOINT;
	if (DNW) {
		desc.b.ep1.bEndpointAddress = USB_EP_ADDRESS(1, USB_IN);
	} else {
		desc.b.ep1.bEndpointAddress = USB_EP_ADDRESS(1, USB_OUT);
	}


	desc.b.ep1.bmAttributes = USB_EP_BULK;
	desc.b.ep1.wMaxPacketSize = make_word_c(64);
	desc.b.ep1.bInterval = 0x0;

	desc.b.ep2.bLength = sizeof(ep_desc_t);
	desc.b.ep2.bDescriptorType = USB_DESC_ENDPOINT;
	if (DNW) {
		desc.b.ep2.bEndpointAddress = USB_EP_ADDRESS(3, USB_OUT);
	} else {
		desc.b.ep2.bEndpointAddress = USB_EP_ADDRESS(2, USB_IN);
	}
	desc.b.ep2.bmAttributes = USB_EP_BULK;
	desc.b.ep2.wMaxPacketSize = make_word_c(64);
	desc.b.ep2.bInterval = 0x0;

	/* set language */
	/* See: http://www.usb.org/developers/data/USB_LANGIDs.pdf */
	sd_zero.bDescriptorType = USB_DESC_STRING;
	sd_zero.bLength = sizeof(string_desc_t);
	sd_zero.bString[0] = make_word_c(0x409);	/* American English */
	s3c24x0_usb_set_string_descriptor(0, &sd_zero);
}

#if defined(CONFIG_S3C2460x)
void Usb_On(void)
{
	rGPJDAT |= (1 << 2);
	rGPJCON = rGPJCON & ~(3 << 4) | (1 << 4);
}

void Usb_Off(void)
{
	rGPJDAT &= ~(1 << 2);
	rGPJCON = rGPJCON & ~(3 << 4) | (1 << 4);
}
#endif


//////////////////////////////////////////////////////////////////////////////
// Module Initialization and Shutdown
//////////////////////////////////////////////////////////////////////////////
/*
 * usbctl_init()
 * Module load time. Allocate dma and interrupt resources. Setup /proc fs
 * entry. Leave UDC disabled.
 */
int usbctl_init(void)
{

	memset(&usbd_info, 0, sizeof(usbd_info));

#if defined(CONFIG_S3C2460x)
	Usb_Off();
	CLKCON |= ENABLE_SeIUPLL;	/* for 2460 */
	CLKDIV_1 |= (ENABLE_UPLLCLK_DIV);	/* for 2460 */
	UPLLCON = FInsrt(72, PLL_MDIV) | FInsrt(3, PLL_PDIV)
	    | FInsrt(1, PLL_SDIV);
	INTMSK &= ~(BIT_USBD_2460);
	INTSUBMSK &= ~(BIT_USBD_SUB);
	Usb_On();
#else

	/* MISC. register */
	//MISCCR &= ~MISCCR_USBPAD; /* S3C24x0, S3C2440a */
	/* CLKCON */
	CLKCON |= CLKCON_USBD;
	/* UPLLCON */
	/* S3C24A0A UPLL is 96Mhz, Internally set to 48Mhz */
	UPLLCON = FInsrt(56, fPLL_MDIV) | FInsrt(2, fPLL_PDIV)
	    | FInsrt(1, fPLL_SDIV);

	/*  interrupt enable */
	INTMSK &= ~(BIT_USBD);
#endif
	//printf("S3C24x0 USB Controller Core Initialized");
	printf("\n");
	usbd_dn_cnt = 0;
	return 0;

}
