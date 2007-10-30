/*
 *
 *  usb_ep0.c - S3C24x0 USB controller driver.
 *              Endpoint zero management
 *
 *  Seungbum Lim <shawn@mizi.com>
 */

///#include <asm/arch/s3c24a0.h>
#include <asm-arm/arch-arm926ejs/s3c2413.h>
#include <s3c24x0_usb.h>  /* public interface */
#include "usb_ctl.h"     /* private stuff */


//#define VERBOSITY 1

enum { false = 0, true = 1 };
#ifndef MIN
#define MIN( a, b ) ((a)<(b)?(a):(b))
#endif

#if 1 && !defined( ASSERT )
#  define ASSERT(expr) \
          if(!(expr)) { \
          printf( "Assertion failed! %s,%s,%s,line=%d\n",\
          #expr,__FILE__,__FUNCTION__,__LINE__); \
          }
#else
#  define ASSERT(expr)
#endif

#if VERBOSITY
#define PRINTKD(fmt, args...) printf( fmt , ## args)
#else
#define PRINTKD(fmt, args...)
#endif

unsigned int ep0_state; // sec like, shawn
__u8 set_configuration;
__u8 set_interface;
__u8 device_status;
__u8 ep0_status;
__u8 ep_bulk_in_status;
__u8 ep_bulk_out_status;
unsigned int control_complete;


/*================================================
 * USB Protocol Stuff
 */

/* Request Codes, standard request spec 1.1 */
enum { GET_STATUS=0,         CLEAR_FEATURE=1,     SET_FEATURE=3,
	   SET_ADDRESS=5,        GET_DESCRIPTOR=6,	  SET_DESCRIPTOR=7,
	   GET_CONFIGURATION=8,  SET_CONFIGURATION=9, GET_INTERFACE=10,
	   SET_INTERFACE=11 };


/* USB Device Requests */
typedef struct
{
    __u8 bmRequestType;
    __u8 bRequest;
    __u16 wValue;
    __u16 wIndex;
    __u16 wLength;
} usb_dev_request_t  __attribute__ ((packed));

static int read_fifo( usb_dev_request_t * p );
static void standard_dev_req(usb_dev_request_t req);
static void set_feature(usb_dev_request_t req);
static void clear_feature(usb_dev_request_t req);
static void set_descriptor(void);
static void get_descriptor(usb_dev_request_t req);
static void ep0_transmit(void);
static void ep0_receive(void);
/***************************************************************************
Inline Helpers
***************************************************************************/

/* Data extraction from usb_request_t fields */
enum { kTargetDevice=0, kTargetInterface=1, kTargetEndpoint=2 };
static inline int request_target( __u8 b ) { return (int) ( b & 0x0F); }

/* Standard Feature Selectors, shawn */
enum { fEndpoint_Halt = 0, fDevice_Remote_Wakeup = 1 };

static inline int windex_to_ep_num( __u16 w ) { return (int) ( w & 0x000F); }
inline int type_code_from_request( __u8 by ) { return (( by >> 4 ) & 3); } // ? [5,6] , shawn


/* print string descriptor */
static inline void psdesc( string_desc_t * p )
{
	 int i;
	 int nchars = ( p->bLength - 2 ) / sizeof( __u16 );
	// printf( "'" );
	 for( i = 0 ; i < nchars ; i++ ) {
	//	  printf( "%c", (char) p->bString[i] );
	 }
	// printf( "'\n" );
}


#if VERBOSITY
/* "pcs" == "print control status" */
static inline void pcs( void )
{
	unsigned long backup;
	__u32 foo ;
	
	backup = UD_INDEX;
	UD_INDEX = UD_INDEX_EP0;	
	foo = UD_ICSR1;
	//printf( "%8.8X\n", foo);
	UD_INDEX = backup;
}
static inline void preq( usb_dev_request_t * pReq )
{
	 static char * tnames[] = { "dev", "intf", "ep", "oth" };
	 static char * rnames[] = { "std", "class", "vendor", "???" };
	 char * psz;
	 switch( pReq->bRequest ) {
	 case GET_STATUS: psz = "get stat"; break;
	 case CLEAR_FEATURE: psz = "clr feat"; break;
	 case SET_FEATURE: psz = "set feat"; break;
	 case SET_ADDRESS: psz = "set addr"; break;
	 case GET_DESCRIPTOR: psz = "get desc"; break;
	 case SET_DESCRIPTOR: psz = "set desc"; break;
	 case GET_CONFIGURATION: psz = "get cfg"; break;
	 case SET_CONFIGURATION: psz = "set cfg"; break;
	 case GET_INTERFACE: psz = "get intf"; break;
	 case SET_INTERFACE: psz = "set intf"; break;
	 default: psz = "unknown"; break;
	 }
	// printf( "- [%s: %s req to %s. dir=%s]\n", psz,
	//		 rnames[ (pReq->bmRequestType >> 5) & 3 ],
	//		 tnames[ pReq->bmRequestType & 3 ],
	//		 ( pReq->bmRequestType & 0x80 ) ? "in" : "out" );
}

#else
#define pcs() (void)(0)
#define preq(x) (void)(0)
#endif


/***************************************************************************
Globals
***************************************************************************/
static const char pszMe[] = "usbep0: ";


/* global write struct to keep write
   ..state around across interrupts */
static struct {
		unsigned char *p;
		int bytes_left;
		unsigned int transfer_length;
		unsigned int transfered_data;
} wr;




/***************************************************************************
Public Interface
***************************************************************************/
#define NULL 0
/* reset received from HUB (or controller just went nuts and reset by itself!)
  so udc core has been reset, track this state here  */
void
ep0_reset(void)
{
	 /* reset state machine */
	 wr.p = NULL;
	 wr.bytes_left = 0;
	 wr.transfer_length = 0;
	 wr.transfered_data = 0;

	 usbd_info.address = 0;
	 control_complete = 0;
}

/* handle interrupt for endpoint zero */

#define EP0_STATE_IDLE		 0
#define EP0_STATE_TRANSFER	 1
#define EP0_STATE_RECEIVER	 2

#define clear_ep0_sst { \
	UD_INDEX = UD_INDEX_EP0; \
	UD_ICSR1 = 0x00; /*EP0_CSR_SENTSTL*/ \
}
#define clear_ep0_se { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg=UD_ICSR1;\
	reg=( reg & ~0xC0) | EP0_CSR_SSE; \
	UD_ICSR1 = reg;\
}
#define clear_ep0_opr { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg=UD_ICSR1;\
	reg=( reg & ~0xC0) | EP0_CSR_SOPKTRDY; \
	UD_ICSR1=reg;\
}
#define set_ep0_ipr { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg=UD_ICSR1;\
	reg= ( reg & ~0xC0) | EP0_CSR_IPKRDY; \
	UD_ICSR1=reg;\
}
#define set_ep0_de { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg = UD_ICSR1\
	reg = EP0_CSR_DE; \
	UD_ICSR1= reg;\
}
#define set_ep0_ss { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg = UD_ICSR1;\
	reg = EP0_CSR_SENDSTL; \
	UD_ICSR1 = reg;\
}
#define set_ep0_de_out { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg= UD_ICSR1;\
	reg = ( reg & ~0xC0) | (EP0_CSR_SOPKTRDY | EP0_CSR_DE); \
	UD_ICSR1 = reg;\
}
#define set_ep0_de_in { \
	__u32 reg;\
    	UD_INDEX = UD_INDEX_EP0; \
	reg = UD_ICSR1;\
	reg = ( reg & ~0xC0) |(EP0_CSR_IPKRDY | EP0_CSR_DE); \
	UD_ICSR1=reg;\
}
#if 1
#define clear_stall_ep4_out { \
	__u32 reg; \
	UD_INDEX = UD_INDEX_EP4; \
	reg = UD_OCSR1; \
	UD_OCSR1 = reg; \
}
#define clear_stall_ep3_out { \
	__u32 reg; \
	UD_INDEX = UD_INDEX_EP3; \
	reg = UD_OCSR1; \
	UD_OCSR1 = reg; \
}
#endif
#define clear_stall_ep1_out { \
        __u32 reg; \
        UD_INDEX = UD_INDEX_EP1; \
        reg = UD_OCSR1; \
        UD_OCSR1 = reg; \
}

#define clear_stall_ep2_out { \
        __u32 reg; \
        UD_INDEX = UD_INDEX_EP2; \
        reg = UD_OCSR1; \
        UD_OCSR1 = reg; \
}


#define rOUT_FIFO_CNT1_REG              (*(volatile unsigned char*)0x44A00198)
#define rEP0_FIFO                       (*(volatile unsigned char*)0x44A001C0)
#define rEP0_CSR                        (*(volatile unsigned char*)0x44A00184)
#define EP0_WR_BITS              0xc0


#define EP0_SERVICED_OUT_PKT_RDY 0x40 
#define EP0_OUT_PKT_READY        0x01 
#define FLUSH_EP0_FIFO()                {while(rOUT_FIFO_CNT1_REG)rEP0_FIFO;}
#define CLR_EP0_OUT_PKT_RDY(ep0_csr)           rEP0_CSR=( ep0_csr & (~EP0_WR_BITS)| EP0_SERVICED_OUT_PKT_RDY )



void ep0_int_hndlr( void )
{
	usb_dev_request_t req;
	int request_type, n ;
	__u32 cs_reg;
	
	UD_INDEX = UD_INDEX_EP0;
	cs_reg = UD_ICSR1;

	PRINTKD( "/\\(%d)\n", UD_FUNC );
	pcs();
	if( cs_reg & EP0_CSR_SE) {
	    clear_ep0_se;
		if(cs_reg & EP0_OUT_PKT_READY)
        	{
            		FLUSH_EP0_FIFO(); //(???)
                //I think this isn't needed because EP0 flush is done automatically.
            		CLR_EP0_OUT_PKT_RDY(cs_reg);
        	}
	

	    ep0_state = EP0_STATE_IDLE;
	     //return; //?? !!
	}

	if( cs_reg & EP0_CSR_SENTSTL) {
	    clear_ep0_sst;
	    ep0_state = EP0_STATE_IDLE;
	    //return; //?? !!
	}
	switch(ep0_state)
	{
	    case EP0_STATE_IDLE :
			if(cs_reg & EP0_CSR_OPKRDY) {
				/* read setup request */
				n = read_fifo(&req);
				
				if ( n != sizeof(req)) {
				  	printf("%ssettup begin : fifo READ ERROR wanted %d bytes got %d. Stalling out...\n", pszMe, sizeof(req), n);
  					set_ep0_ss;
					return;
				}
#if VERBOSITY
				{
					unsigned char * pdb = (unsigned char *)&req;
					PRINTKD( "%2.2X %2.2X %2.2X %2.2X"
						   " %2.2X %2.2X %2.2X %2.2X ",
						pdb[0], pdb[1], pdb[2], pdb[3], 
						pdb[4], pdb[5], pdb[6], pdb[7]);
					preq(&req);
				}
#endif
				
				request_type = type_code_from_request(req.bmRequestType);
				switch( request_type){/* Only Support Standard Request, shawn */
					case 0: // standard request
						standard_dev_req(req);
					break;
					default: // class, vendor, resolved types
						printf("%ssetup begin : unsupported bmRequestType :"
							   " %d ignored\n", pszMe, request_type);
					return;
				}
			}
			break;
		case EP0_STATE_TRANSFER :
			ep0_transmit();
		break;
		case EP0_STATE_RECEIVER : 
			ep0_receive();
		break;
	 }

	 PRINTKD( "---\n" );
	 pcs();
	 PRINTKD( "\\/\n\n" );
}


/* working shawn */
static void standard_dev_req(usb_dev_request_t req)
{
    __u32 address;
    int n, e;

    n = request_target(req.bmRequestType);
    switch( n )
    {
	/* device recipient */
	case kTargetDevice :
    
	    switch(req.bRequest)
	    {
		case SET_FEATURE :
		    set_feature(req); /* work?, shawn */
		    break;
	
		case CLEAR_FEATURE :
		    clear_feature(req); /* work? shawn */
		    break;
		case SET_ADDRESS :
		    address = (__u32)(req.wValue & 0x7F);
		    UD_FUNC = (address | 0x80);
		    usbd_info.address = address;
		    usbctl_next_state_on_event( kEvAddress );
		    set_ep0_de_out; 
		    ep0_state = EP0_STATE_IDLE;
//    printf( "%sI have been assigned address: %d\n", pszMe, address );
		    break;
		case SET_DESCRIPTOR :
		    set_descriptor(); 
		    break;
		case SET_CONFIGURATION :
		    set_configuration = (__u8)req.wValue; /* low byte */
#if 0
		    set_ep0_de_out;
#endif
		    if(req.wValue == 1 ) {  /* configured */
			if( usbctl_next_state_on_event( kEvConfig ) != kError ) {
			    desc_t *pDesc = s3c24x0_usb_get_descriptor_ptr();
			    __u32 in  = __le16_to_cpu(pDesc->b.ep2.wMaxPacketSize);
			    __u32 out = __le16_to_cpu(pDesc->b.ep1.wMaxPacketSize);
			
			   UD_INDEX = 2;
			   UD_MAXP = UD_MAXP_64;
			   UD_INDEX = 1;
			   UD_MAXP = UD_MAXP_64;
			    
			   //printf("%sConfigured (IN MAX PACKET=%d, OUT MAX PACKET=%d)\n",pszMe, in, out);
			}
		    }else if( req.wValue == 0 ) {
			if( usbctl_next_state_on_event( kEvDeConfig ) != kError )
			    printf("%sDe-Configuration\n", pszMe );
		    }else{ 
			printf("%ssetup phase : Unknown [set configuration] data %d\n", pszMe, req.wValue );
		    }
		    set_ep0_de_out;
		    break;
		case GET_STATUS :
		    clear_ep0_opr; 
		    UD_FIFO0 = device_status;
		    UD_FIFO0 = 0x00;
		    set_ep0_de_in; 
		    break;
		case GET_CONFIGURATION :
		    clear_ep0_opr; 
		    UD_FIFO0 = set_configuration;
		    set_ep0_de_in;
		    break;
		case GET_DESCRIPTOR :
		    get_descriptor(req); 
		    break;
		default :
		    set_ep0_de_out;
	    }
	    break;
	    
	    /* Interface Recipient */
	case kTargetInterface :
	    switch ( req.bRequest )
	    {
		case SET_INTERFACE :
		    set_interface = req.wValue; 
		    set_ep0_de_out;
		    break;
		case GET_INTERFACE :
		    clear_ep0_opr;
		    UD_FIFO0 = set_interface;
		    set_ep0_de_in;
		    break;
	    }
	    break;
	case kTargetEndpoint :
	    switch( req.bRequest )
	    {
		case SET_FEATURE :
		    set_feature(req);
		    break;
		case CLEAR_FEATURE :
		    clear_feature(req);
		    break;
		case GET_STATUS :
		    clear_ep0_opr;
		    e = windex_to_ep_num( req.wIndex );
		    if( e == 0 ) {
			UD_FIFO0 = ep0_status;
			UD_FIFO0 = 0x00;
		    }else if( e == 2 ) {
			UD_FIFO0 = ep_bulk_in_status;
			UD_FIFO0 = 0x00;
		    }else if( e == 1 ) {
			UD_FIFO0 = ep_bulk_out_status;
			UD_FIFO0 = 0x00;
		    }

		    set_ep0_de_in; 

		    break;
		case GET_DESCRIPTOR :
		    get_descriptor(req); 
		    break;
	    }
    }
}



static void get_descriptor(usb_dev_request_t req)
{
    string_desc_t * pString;

    desc_t * pDesc = s3c24x0_usb_get_descriptor_ptr();

    int type = req.wValue >> 8;
    int idx  = req.wValue & 0xFF;

    switch( type )
    {
	case USB_DESC_DEVICE :
	    clear_ep0_opr;

	    wr.p = (unsigned char *)&pDesc->dev;
	    PRINTKD("\nget descriptor() DEV: req.wLengh=%d, pDesc->dev.bength=%d\n\n"\
		    ,req.wLength, pDesc->dev.bLength);
	    
	    wr.transfer_length = MIN(req.wLength, pDesc->dev.bLength);
	    wr.transfered_data = 0; // ÃÊ±âÈ­
	    ep0_state = EP0_STATE_TRANSFER;
	    ep0_transmit();
	    break;
	case USB_DESC_CONFIG :
	    clear_ep0_opr;
	    wr.p = (unsigned char *)&pDesc->b;
	    wr.transfer_length = MIN(req.wLength, sizeof( struct cdb ));
	    PRINTKD("\nget descriptor() CONFIG: req.wLengh=%d, pDesc->dev.bength=%d\n\n"\
		    ,req.wLength, pDesc->dev.bLength);
	    ep0_state = EP0_STATE_TRANSFER;
	    wr.transfered_data = 0;
	    ep0_transmit();
	    break;
	case USB_DESC_STRING :
	    PRINTKD("STRING STRING\n");
	    pString = s3c24x0_usb_get_string_descriptor( idx );
	    if( pString ) {
		if (idx != 0) { // if not language index
		    printf("%sReturn string %d: ",pszMe, idx);
		    psdesc( pString );
		}
		clear_ep0_opr;
		wr.p = (unsigned char *)pString;
		wr.transfer_length = MIN(req.wLength, pString->bLength);
		PRINTKD("\nget descriptor() String: req.wLengh=%d, pDesc->dev.bength=%d\n\n"\
		       ,req.wLength, pDesc->dev.bLength);
		ep0_state = EP0_STATE_TRANSFER;
		wr.transfered_data = 0;
		ep0_transmit();
		break;
	    }

	default :
	    clear_ep0_opr;
	    set_ep0_de_in;
	    break;
    }

}

static void ep0_transmit(void)
{
    int i, data_length;
    __u32 reg_ep0_status;
    __u32 reg_int_status;

	UD_INDEX = UD_INDEX_EP0;
	reg_ep0_status = UD_ICSR1;


    PRINTKD("EP0 want to send : %d byte\n",wr.transfer_length);

    
    /* ep0 input fifo check */
    if( ( reg_ep0_status & EP0_CSR_IPKRDY) == 0 ) {
	data_length = wr.transfer_length - wr.transfered_data;
	data_length = MIN(data_length, 8); /* EP0 MAX Packet size == 8 */


	/* need to check RESET, RESUME and SUSPEND Interrupts */
	reg_int_status = UD_USBINT;
	reg_int_status &= UD_USBINT_RESET | UD_USBINT_RESUM | UD_USBINT_SUSPND;
	if( reg_int_status == UD_USBINT_RESET) {// if RESET occures, just return
//		printf(__FUNCTION__"(): UD_USBINT_RESET\n");
	    return;
	}

	
	PRINTKD(" SENDING... [");
	for(i = 0; i < data_length; i++) {
	    UD_FIFO0 = *wr.p;
	    PRINTKD("%2.2X ", *wr.p);
	    wr.p++;
	    wr.transfered_data++;
	    
	}
	PRINTKD("] \n");
	PRINTKD("EP0 transfered : %d bytes\n", wr.transfered_data);
	PRINTKD("wr.transfered_data = %d, wr.transfer_length = %d\n",
			wr.transfered_data, wr.transfer_length);


	if( wr.transfered_data == wr.transfer_length) {
	    reg_int_status = UD_USBINT;
	    reg_int_status &= UD_USBINT_RESET | UD_USBINT_RESUM | UD_USBINT_SUSPND;

	    if(reg_int_status == UD_USBINT_RESET) 
		return;
	    if( (wr.transfer_length % 16) == 0 && control_complete == 0 ) {
		control_complete = 1;
		set_ep0_ipr;
	    }else{
		control_complete = 0;
		set_ep0_de_in;
		ep0_state = EP0_STATE_IDLE;
	    }
	    return;
	}
	set_ep0_ipr;
    }
}

static void set_feature(usb_dev_request_t req)
{
    int ep;
    
    switch( req.wValue )
    {
	case fEndpoint_Halt :
	    ep = windex_to_ep_num( req.wIndex );
	    if( ep == 0 ) {
		printf("%sset feature [endpoint halt] on control\n", pszMe);
		ep0_status = 0x001;
		set_ep0_ss;
		clear_ep0_opr;
	    }else if( ep == 2 ) {
		printf("%set feature [endpoint halt] on xmitter\n",pszMe);
		ep_bulk_in_status = 0x0001;
		UD_INDEX = UD_INDEX_EP2;
		UD_ICSR1 |= UD_ICSR1_CLRDT;
	//	ep2_stall();
		
	    }else if( ep == 1 ) {
		printf("%set feature [endpoint halt] on receiver\n",pszMe);
		ep_bulk_out_status = 0x0001;

		UD_INDEX = UD_INDEX_EP1;
		UD_OCSR1 |= UD_OCSR1_SENDSTL;
	//	ep1_stall();
	    }else {
		printf("%sUnsupported feature selector (%d) in set feature\n", pszMe, req.wValue);
		set_ep0_de_out;
	    }
	    break;
	case fDevice_Remote_Wakeup :
	    device_status = 0x02;
	    set_ep0_de_out;
	    break;
	default :
	    set_ep0_de_out;
	    break;
    }
}
	    
static void clear_feature(usb_dev_request_t req)
{
    int ep;

    switch( req.wValue )
    {
	case fEndpoint_Halt :
	    ep = windex_to_ep_num( req.wIndex );
	    if( ep == 0 )  // ep0
		ep0_status = 0x0000;
	    else if( ep == 2 ) { // ep2  input
		ep_bulk_in_status = 0x0000;
  		clear_stall_ep4_out; // ep4??
		PRINTKD(__FUNCTION__"(): confused. - bushi\n");
	    }else if( ep == 1 ) { // ep1 output
		ep_bulk_out_status = 0x0000;
		clear_stall_ep1_out; //??
	    }else 
		PRINTKD("%sUnsupported endpoint (%d)\n", pszMe, ep);

	    set_ep0_de_out;
	    break;
	case fDevice_Remote_Wakeup :
	    device_status = 0x00;
	    set_ep0_de_out;
	    break;
	default :
	    set_ep0_de_out;
	    break;
    }
}

static void set_descriptor(void)
{
    set_ep0_de_out;
}

static void ep0_receive(void)
{

}
/*
 * read_fifo()
 * Read 1-8 bytes out of FIFO and put in request.
 * Called to do the initial read of setup requests
 * from the host. Return number of bytes read.
 *
 * Like write fifo above, this driver uses multiple
 * reads checked agains the count register with an
 * overall timeout.
 *
 */
static int
read_fifo( usb_dev_request_t * request )
{
	int bytes_read = 0;
	int fifo_count = 0;
	int i, ep;
	unsigned char * pOut = (unsigned char*) request;
       
	ep = windex_to_ep_num( request->wIndex );
	PRINTKD(__FUNCTION__"(): ep = %d\n", ep);
	switch (ep) {
		case 0:
			UD_INDEX = UD_INDEX_EP0;
			break;
		case 2:
			UD_INDEX = UD_INDEX_EP2;
			break;
		case 1:
			UD_INDEX = UD_INDEX_EP1;
			break;
		default:
			PRINTKD(__FUNCTION__"(): ???? ep = %d\n", ep);
			UD_INDEX = UD_INDEX_EP0;
	}
      
	fifo_count = (( UD_OFCNTH << 8) | UD_OFCNTL) & 0xffff;

	ASSERT( fifo_count <= 8 );
	PRINTKD(__FUNCTION__ "(): fifo_count =%d ", fifo_count );

	while( fifo_count-- ) {
		 i = 0;
		 do {
			  *pOut = (unsigned char) UD_FIFO0; 
		  	  udelay( 10 );
  			  i++;
		 } while( ( (( UD_OFCNTH << 8) | UD_OFCNTL) & 0xffff ) != fifo_count && i < 10 );
		 if ( i == 10 ) {
			  printf( "%sread_fifo(): read failure\n", pszMe );
			  usbd_info.stats.ep0_fifo_read_failures++;
		 }
		 pOut++;
		 bytes_read++;
	}

	PRINTKD( "bytes_read=%d\n", bytes_read );
	usbd_info.stats.ep0_bytes_read++;
	return bytes_read;
}

/* end usb_ep0.c */
