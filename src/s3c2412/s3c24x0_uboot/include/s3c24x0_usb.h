/*
 * Public interface to the s3c24x0 USB core. For use by client modules
 * like usb-eth and usb-char.
 *  2004 (C) Samsung Elecronics 
 *              SW.LEE <hitchcar@samsung.com> 
                For S3C2440A and S3C24A0A
 *
 */

#ifndef _S3C24X0_USB_H
#define _S3C24X0_USB_H
#include <asm/byteorder.h>
#include <asm/arch/hardware.h>
#include <asm/arch/bitfield.h>


#undef USE_USBD_DMA

typedef void (*usb_callback_t)(int flag, int size);

/* in usb_ctl.c (see also descriptor methods at bottom of file) */

// Open the USB client for client and initialize data structures
// to default values, but _do not_ start UDC.
int s3c24x0_usb_open( const char * client_name );

// Start UDC running
int s3c24x0_usb_start( void );

// Immediately stop udc, fire off completion routines w/-EINTR
int s3c24x0_usb_stop( void ) ;

// Disconnect client from usb core
int s3c24x0_usb_close( void ) ;

// set notify callback for when core reaches configured state
// return previous pointer (if any)
typedef void (*usb_notify_t)(void);
usb_notify_t s3c24x0_set_configured_callback( usb_notify_t callback );

/* in usb_send.c */
int s3c24x0_usb_xmitter_avail( void );
int s3c24x0_usb_send(char *buf, int len, usb_callback_t callback);
void s3c24x0_usb_send_reset(void);

/* in usb_recev.c */
int s3c24x0_usb_recv(char *buf, int len, usb_callback_t callback);
void s3c24x0_usb_recv_reset(void);

//////////////////////////////////////////////////////////////////////////////
// Descriptor Management
//////////////////////////////////////////////////////////////////////////////

#define DescriptorHeader \
	__u8 bLength;        \
	__u8 bDescriptorType


// --- Device Descriptor -------------------

typedef struct {
	 DescriptorHeader;
	 __u16 bcdUSB;		   	/* USB specification revision number in BCD */
	 __u8  bDeviceClass;	/* USB class for entire device */
	 __u8  bDeviceSubClass; /* USB subclass information for entire device */
	 __u8  bDeviceProtocol; /* USB protocol information for entire device */
	 __u8  bMaxPacketSize0; /* Max packet size for endpoint zero */
	 __u16 idVendor;        /* USB vendor ID */
	 __u16 idProduct;       /* USB product ID */
	 __u16 bcdDevice;       /* vendor assigned device release number */
	 __u8  iManufacturer;	/* index of manufacturer string */
	 __u8  iProduct;        /* index of string that describes product */
	 __u8  iSerialNumber;	/* index of string containing device serial number */
	 __u8  bNumConfigurations; /* number fo configurations */
} __attribute__ ((packed)) device_desc_t;

// --- Configuration Descriptor ------------

typedef struct {
	 DescriptorHeader;
	 __u16 wTotalLength;	    /* total # of bytes returned in the cfg buf 4 this cfg */
	 __u8  bNumInterfaces;      /* number of interfaces in this cfg */
	 __u8  bConfigurationValue; /* used to uniquely ID this cfg */
	 __u8  iConfiguration;      /* index of string describing configuration */
	 __u8  bmAttributes;        /* bitmap of attributes for ths cfg */
	 __u8  MaxPower;		    /* power draw in 2ma units */
} __attribute__ ((packed)) config_desc_t;

// bmAttributes:
enum { USB_CONFIG_REMOTEWAKE=0x20, USB_CONFIG_SELFPOWERED=0x40,
	   USB_CONFIG_BUSPOWERED=0x80 };
// MaxPower:
#define USB_POWER( x)  ((x)>>1) /* convert mA to descriptor units of A for MaxPower */

// --- Interface Descriptor ---------------

typedef struct {
	 DescriptorHeader;
	 __u8  bInterfaceNumber;   /* Index uniquely identfying this interface */
	 __u8  bAlternateSetting;  /* ids an alternate setting for this interface */
	 __u8  bNumEndpoints;      /* number of endpoints in this interface */
	 __u8  bInterfaceClass;    /* USB class info applying to this interface */
	 __u8  bInterfaceSubClass; /* USB subclass info applying to this interface */
	 __u8  bInterfaceProtocol; /* USB protocol info applying to this interface */
	 __u8  iInterface;         /* index of string describing interface */
} __attribute__ ((packed)) intf_desc_t;

// --- Endpoint  Descriptor ---------------

typedef struct {
	 DescriptorHeader;
	 __u8  bEndpointAddress;  /* 0..3 ep num, bit 7: 0 = 0ut 1= in */
	 __u8  bmAttributes;      /* 0..1 = 0: ctrl, 1: isoc, 2: bulk 3: intr */
	 __u16 wMaxPacketSize;    /* data payload size for this ep in this cfg */
	 __u8  bInterval;         /* polling interval for this ep in this cfg */
} __attribute__ ((packed)) ep_desc_t;

// bEndpointAddress:
/* 
 *  bEndpointAddress[0:7]
 *  	[0:3] - The endpoint number
 *	[4:6] - Reserved, reset to zero
 *	[7]   - Direction, ignored for contol endpoint
 *		0 = OUT endpoint
 *		1 = IN endpoint
 *   shawn
 */ 
enum { USB_OUT=0, USB_IN=1 }; 


   
#define USB_EP_ADDRESS(a,d) (((a)&0xf) | ((d) << 7))
// bmAttributes:
enum { USB_EP_CNTRL=0, USB_EP_BULK=2, USB_EP_INT=3 };

// --- String Descriptor -------------------

typedef struct {
	 DescriptorHeader;
	 __u16 bString[1];		  /* unicode string .. actaully 'n' __u16s */
} __attribute__ ((packed)) string_desc_t;

/*=======================================================
 * Handy helpers when working with above
 *
 */
// these are x86-style 16 bit "words" ...
#define make_word_c( w ) __constant_cpu_to_le16(w)
#define make_word( w )   __cpu_to_le16(w)

// descriptor types
enum { USB_DESC_DEVICE=1, USB_DESC_CONFIG=2, USB_DESC_STRING=3,
	   USB_DESC_INTERFACE=4, USB_DESC_ENDPOINT=5 };


/*=======================================================
 * Default descriptor layout for S3C2440 UDC
 */

/* S3C2440 USB Device 64Byte
 * S3C2440 USB Device 128 Byte
 */
/* "config descriptor buffer" - that is, one config,
   ..one interface and 2 endpoints */
struct cdb {
	 config_desc_t cfg;
	 intf_desc_t   intf;
	 ep_desc_t     ep1; // DOWN stream(OUT)
	 ep_desc_t     ep2; // UP stream(IN)
} __attribute__ ((packed));


/* all S3C2440 device descriptors */
typedef struct {
	 device_desc_t dev;   /* device descriptor */
	 struct cdb b;        /* bundle of descriptors for this cfg */
} __attribute__ ((packed)) desc_t;


/*=======================================================
 * Descriptor API
 */

/* Get the address of the statically allocated desc_t structure
   in the usb core driver. Clients can modify this between
   the time they call s3c24x0_usb_open() and s3c2440_usb_start()
*/
desc_t *
s3c24x0_usb_get_descriptor_ptr( void );


/* Set a pointer to the string descriptor at "index". The driver
 ..has room for 8 string indicies internally. Index zero holds
 ..a LANGID code and is set to US English by default. Inidices
 ..1-7 are available for use in the config descriptors as client's
 ..see fit. This pointer is assumed to be good as long as the
 ..S3C2440 usb core is open (so statically allocate them). Returnes -EINVAL
 ..if index out of range */
int s3c24x0_usb_set_string_descriptor( int index, string_desc_t * p );

/* reverse of above */
string_desc_t *
s3c24x0_usb_get_string_descriptor( int index );

/* kmalloc() a string descriptor and convert "p" to unicode in it */
string_desc_t *
s3c24x0_usb_kmalloc_string_descriptor( const char * p );

/*
 * USB Device - Little Endian : 
 * (B) : byte(8 bit) access
 * (W) : word(32 bit) access
 */

#define bUD(Nb)		__REG(USB_DEVICE_PHYS_ADR + (Nb))
#define UD_FUNC		bUD(0x140) // Function address  (B)
#define UD_PWR		bUD(0x144) // Power management (B)
#define UD_INT		bUD(0x148) // Endpoint interrupt pending/clear (B)
#define UD_USBINT	bUD(0x158) // USB interrupt pending/clear (B)
#define UD_INTE		bUD(0x15c) // Endpoint interrupt enable (B)
#define UD_USBINTE	bUD(0x16c) // USB interrupt enable (B)
#define UD_FRAMEL	bUD(0x170) // Frame number low-byte (B)
#define UD_FRAMEH	bUD(0x174) // Frame number high-byte (B)
#define UD_INDEX	bUD(0x178) // Index (B)
#define UD_FIFO0	bUD(0x1c0) // Endpoint 0 FIFO (B)
#define UD_FIFO1	bUD(0x1c4) // Endpoint 1 FIFO (B)
#define UD_FIFO2	bUD(0x1c8) // Endpoint 2 FIFO (B)
#define UD_FIFO3	bUD(0x1cc) // Endpoint 3 FIFO (B)
#define UD_FIFO4	bUD(0x1d0) // Endpoint 4 FIFO (B)
#define UD_DMACON1	bUD(0x200) // Endpoint 1 DMA control (B)
#define UD_DMAUC1	bUD(0x204) // Endpoint 1 DMA unit counter (B)
#define UD_DMAFC1	bUD(0x208) // Endpoint 1 DMA FIFO counter
#define UD_DMATCL1	bUD(0x20c) // Endpoint 1 DMA Transfer counter low-byte
#define UD_DMATCM1	bUD(0x210) // Endpoint 1 DMA Transfer counter middle-byte
#define UD_DMATCH1	bUD(0x214) // Endpoint 1 DMA Transfer counter high-byte
#define UD_DMACON2	bUD(0x218) // Endpoint 2 DMA control (B)
#define UD_DMAUC2	bUD(0x21c) // Endpoint 2 DMA unit counter (B)
#define UD_DMAFC2	bUD(0x220) // Endpoint 2 DMA FIFO counter
#define UD_DMATCL2	bUD(0x224) // Endpoint 2 DMA Transfer counter low-byte
#define UD_DMATCM2	bUD(0x228) // Endpoint 2 DMA Transfer counter middle-byte
#define UD_DMATCH2	bUD(0x22c) // Endpoint 2 DMA Transfer counter high-byte
#define UD_DMACON3	bUD(0x240) // Endpoint 3 DMA control (B)
#define UD_DMAUC3	bUD(0x244) // Endpoint 3 DMA unit counter (B)
#define UD_DMAFC3	bUD(0x248) // Endpoint 3 DMA FIFO counter
#define UD_DMATCL3	bUD(0x24c) // Endpoint 3 DMA Transfer counter low-byte
#define UD_DMATCM3	bUD(0x250) // Endpoint 3 DMA Transfer counter middle-byte
#define UD_DMATCH3	bUD(0x254) // Endpoint 3 DMA Transfer counter high-byte
#define UD_DMACON4	bUD(0x258) // Endpoint 4 DMA control (B)
#define UD_DMAUC4	bUD(0x25c) // Endpoint 4 DMA unit counter (B)
#define UD_DMAFC4	bUD(0x260) // Endpoint 4 DMA FIFO counter
#define UD_DMATCL4	bUD(0x264) // Endpoint 4 DMA Transfer counter low-byte
#define UD_DMATCM4	bUD(0x268) // Endpoint 4 DMA Transfer counter middle-byte
#define UD_DMATCH4	bUD(0x26c) // Endpoint 4 DMA Transfer counter high-byte
#define UD_MAXP		bUD(0x180) // Endpoint MAX Packet
#define UD_ICSR1	bUD(0x184) // EP In control status register 1 (B)
#define UD_ICSR2	bUD(0x188) // EP In control status register 2 (B)
#define UD_OCSR1	bUD(0x190) // EP Out control status register 1 (B)
#define UD_OCSR2	bUD(0x194) // EP Out control status register 2 (B)
#define UD_OFCNTL	bUD(0x198) // EP Out Write counter low-byte (B)
#define UD_OFCNTH	bUD(0x19c) // EP Out Write counter high-byte (B)

#define UD_FUNC_UD	(1 << 7)
#define fUD_FUNC_ADDR	Fld(7,0)	/* USB Device Addr. assigned by host */
#define UD_FUNC_ADDR	FMsk(fUD_FUNC_ADDR)

#define UD_PWR_ISOUP	(1<<7) // R/W
#define UD_PWR_RESET	(1<<3) // R
#define UD_PWR_RESUME	(1<<2) // R/W
#define UD_PWR_SUSPND	(1<<1) // R
#define UD_PWR_ENSUSPND	(1<<0) // R/W

#define UD_PWR_DEFAULT	0x00

#define UD_INT_EP4	(1<<4)	// R/W (clear only)
#define UD_INT_EP3	(1<<3)	// R/W (clear only)
#define UD_INT_EP2	(1<<2)	// R/W (clear only)
#define UD_INT_EP1	(1<<1)	// R/W (clear only)
#define UD_INT_EP0	(1<<0)	// R/W (clear only)

#define UD_USBINT_RESET	(1<<2) // R/W (clear only)
#define UD_USBINT_RESUM	(1<<1) // R/W (clear only)
#define UD_USBINT_SUSPND (1<<0) // R/W (clear only)

#define UD_INTE_EP4	(1<<4) // R/W
#define UD_INTE_EP3	(1<<3) // R/W
#define UD_INTE_EP2	(1<<2) // R/W
#define UD_INTE_EP1	(1<<1) // R/W
#define UD_INTE_EP0	(1<<0) // R/W

#define UD_USBINTE_RESET	(1<<2) // R/W
#define UD_USBINTE_SUSPND	(1<<0) // R/W

#define fUD_FRAMEL_NUM	Fld(8,0) // R
#define UD_FRAMEL_NUM	FMsk(fUD_FRAMEL_NUM)

#define fUD_FRAMEH_NUM	Fld(8,0) // R
#define UD_FRAMEH_NUM	FMsk(fUD_FRAMEH_NUM)

#define UD_INDEX_EP0	(0x00)
#define UD_INDEX_EP1	(0x01) // ??
#define UD_INDEX_EP2	(0x02) // ??
#define UD_INDEX_EP3	(0x03) // ??
#define UD_INDEX_EP4	(0x04) // ??

#define UD_ICSR1_CLRDT	(1<<6)   // R/W
#define UD_ICSR1_SENTSTL (1<<5)  // R/W (clear only)
#define UD_ICSR1_SENDSTL (1<<4)  // R/W
#define UD_ICSR1_FFLUSH (1<<3)  // W	(set only)
#define UD_ICSR1_UNDRUN  (1<<2)  // R/W (clear only)
#define UD_ICSR1_PKTRDY	 (1<<0)  // R/W (set only)

#define UD_ICSR2_AUTOSET (1<<7) // R/W
#define UD_ICSR2_ISO	 (1<<6)	// R/W
#define UD_ICSR2_MODEIN	 (1<<5) // R/W
#define UD_ICSR2_DMAIEN	 (1<<4) // R/W

#define UD_OCSR1_CLRDT	(1<<7) // R/W
#define UD_OCSR1_SENTSTL	(1<<6)	// R/W (clear only)
#define UD_OCSR1_SENDSTL	(1<<5)	// R/W
#define UD_OCSR1_FFLUSH		(1<<4) // R/W
#define UD_OCSR1_DERROR		(1<<3) // R
#define UD_OCSR1_OVRRUN		(1<<2) // R/W (clear only)
#define UD_OCSR1_PKTRDY		(1<<0) // R/W (clear only)

#define UD_OCSR2_AUTOCLR	(1<<7) // R/W
#define UD_OCSR2_ISO		(1<<6) // R/W
#define UD_OCSR2_DMAIEN		(1<<5) // R/W

#define fUD_FIFO_DATA	Fld(8,0) // R/W
#define UD_FIFO0_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO1_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO2_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO3_DATA	FMsk(fUD_FIFO_DATA)
#define UD_FIFO4_DATA	FMsk(fUD_FIFO_DATA)

#define UD_MAXP_8	(1<<0)
#define UD_MAXP_16	(1<<1)
#define UD_MAXP_32	(1<<2)
#define UD_MAXP_64	(1<<3)

#define fUD_OFCNT_DATA	Fld(8,0)
#define UD_OFCNTL_DATA	FMsk(fUD_OFCNT_DATA) //R
#define UD_OFCNTH_DATA	FMsk(fUD_OFCNT_DATA) //R

#define UD_DMACONx_INRUNOB	(1<<7) // R
#define fUD_DMACON_STATE	Fld(3,4) // R
#define UD_DMACONx_STATE	FMsk(fUD_DMACON_STATE) // R/W
#define UD_DMACONx_DEMEN	(1<<3) // R/W
#define UD_DMACONx_ORUN		(1<<2) // R/W
#define UD_DMACONx_IRUN		(1<<1) // R/W
#define UD_DMACONx_DMAMODE	(1<<0) // R/W

#define fUD_DMAUC_DATA	Fld(8,0)
#define UD_DMAUCx_DATA	FMsk(fUD_DMAUC_DATA)

#define fUD_DMAFC_DATA	Fld(8,0)
#define UD_DMAFCx_DATA	FMsk(fUD_DMAFC_DATA)

#define fUD_DMATC_DATA	Fld(8,0)
#define UD_DMATCL_DATA	FMsk(fUD_DMATC_DATA)
#define UD_DMATCM_DATA	FMsk(fUD_DMATC_DATA)
#define UD_DMATCH_DATA	FMsk(fUD_DMATC_DATA)

#define EP0_CSR_OPKRDY	(1<<0)
#define EP0_CSR_IPKRDY	(1<<1)
#define EP0_CSR_SENTSTL	(1<<2)
#define EP0_CSR_DE	(1<<3)
#define EP0_CSR_SE	(1<<4)
#define EP0_CSR_SENDSTL	(1<<5)
#define EP0_CSR_SOPKTRDY (1<<6)
#define EP0_CSR_SSE	(1<<7)






#endif /* _S3C24X0_USB_H */
