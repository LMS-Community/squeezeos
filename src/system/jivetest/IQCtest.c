/*
 * IQCtest.c
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/soundcard.h>
#include <linux/fb.h>
#include <math.h>
#include <linux/input.h>
#include <setjmp.h>

#ifndef EV_SYN
#define EV_SYN 0
#endif 

static const char *ir_devname     = "/dev/misc/irtx";
static const char *wheel_devname  = "/dev/input/event1";
static const char *matrix_devname = "/dev/input/event2";
static const char *motion_devname = "/dev/input/event3";
static const char *io_devname     = "/dev/misc/jive_mgmt";
static const char *mixer_devname  = "/dev/mixer";

static int fd_matrix = -1;
static int fd_wheel  = -1;
static int fd_motion = -1;
static int fd_mixer  = -1;
static int fd_IR     = -1;

static unsigned char ucZippyIR= 0;

#define RGB_TO_565(r,g,b) (unsigned short) ((r & 0xf8)<<8) | ((g & 0xfc)<<3) | ((b & 0xf8)>>3)

//#define RED     RGB_TO_565 (255,   0,   0)
//#define CYAN    RGB_TO_565 (255, 255,   0)
//#define GREEN   RGB_TO_565 (  0, 255,   0)
//#define MAGENTA RGB_TO_565 (  0, 255, 255)
//#define BLUE    RGB_TO_565 (  0,   0, 255)
//#define WHITE   RGB_TO_565 (255, 255, 255)
//#define GRAY    RGB_TO_565 (128, 128, 128)
//#define BLACK   RGB_TO_565 (  0,   0,   0)


//===================================================================

void wait_for_keypress();

//===========================================================================

bool InitializeIO()
{
    if ((fd_matrix = open(matrix_devname, O_RDONLY)) < 0 )
        printf("Matix Error: Unable to initialize %s\n", matrix_devname); 

    if ((fd_wheel = open(wheel_devname, O_RDONLY)) < 0 )
        printf("Wheel Error: Unable to initialize %s\n", wheel_devname); 

    if ((fd_motion = open(motion_devname, O_RDONLY)) < 0 )
        printf("Motion Error: Unable to initialize %s\n", motion_devname); 

	if ((fd_IR = open(ir_devname, O_RDWR)) < 0 )
		printf("IR Error: Unable to initialize %s\n", ir_devname);
		
	if ((fd_mixer = open(mixer_devname, O_RDWR)) < 0 )
		printf("Mixer Error: Unable to initialize %s \n", mixer_devname);

    return true;
}


//===================================================================


bool Splash (char *filename)
{
  	system( "./draw jpeg 0 0 IQCsplash.jpg" );
    return true;
}

//===================================================================

enum keys 
{
    kAdd     = 0x0001,
    kPlay    = 0x0002,
    kGo      = 0x0004,
    kBack    = 0x0008,
    kHome    = 0x0010,
    kVolUp   = 0x0020,
    kVolDn   = 0x0040,
    kRW      = 0x0080,
    kPause   = 0x0100,
    kFF      = 0x0200,

    kUp      = 0x1000,
    kDown    = 0x2000,
    kHold    = 0x4000,
    
    kError   = 0x8000,
    
    kAll     = 0x03FF
};

struct key_event {
	unsigned short code;
	unsigned short value;
};

unsigned short KeypressEvent()
{
    struct key_event key;
    int rd, i, j, k;
	struct input_event ev[64];
	struct input_event ev_matrix[64];

    int keyevent;

	rd = read(fd_matrix, ev, sizeof(struct input_event) * 64);

	if (rd < (int) sizeof(struct input_event)) 
	{
		perror("Keypad read error");
		return -1;
	}

	for (i = 0; i < rd / sizeof(struct input_event); i++) 
	{
		if (ev[i].type == 1)
		{
            key.code = ev[i].code;
    		key.value = ev[i].value;
    		keyevent = 0;

		    printf("Key code: %d  value: %d ", key.code, key.value);

			switch (key.code)
			{
    			case 12:  keyevent = kVolDn; printf("VolDn\n"); break;
    			case 13:  keyevent = kVolUp; printf("VolUp\n"); break;
    			case 30:  keyevent = kAdd;   printf("Add\n");   break;
    			case 35:  keyevent = kHome;  printf("Home\n");  break;
    			case 44:  keyevent = kRW;    printf("RW\n");    break;
    			case 45:  keyevent = kPlay;  printf("Play\n");  break;
    			case 46:  keyevent = kPause; printf("Pause\n"); break;
    			case 48:  keyevent = kFF;    printf("FF\n");    break;
    			case 105: keyevent = kBack;  printf("Back\n");  break;
    			case 106: keyevent = kGo;    printf("Go\n");    break;
    			default:  keyevent = kError; printf("Error\n"); break;
            }
            
            switch (key.value)
            {
			    case 0:  keyevent |= kUp;    break;
			    case 1:  keyevent |= kDown;  break;
                case 2:  keyevent |= kHold;  break;
			    default: keyevent |= kError; break;
			}
		
			return keyevent;
		}
	}

	return 0;
}


bool Keypad ()
{
    unsigned int key, kHistory;
    int x, y;
    char c[12];
    char s[128];
    bool done = false;

  	system( "./draw jpeg 0 0 BeginKeypad.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

  	system( "./draw jpeg 0 0 Keypad.jpg" );

    kHistory = 0;

    system("./draw circle 165 260 15 blue");  // FF
    system("./draw circle  68 260 15 blue");  // RW
    system("./draw circle 173 170 15 blue");  // Home
    system("./draw circle 173  50 15 blue");  // Play
    system("./draw circle  56  50 15 blue");  // Add
    system("./draw circle  56 170 15 blue");  // Back
    system("./draw circle 115 260 15 blue");  // Pause
    system("./draw circle  92 215 15 blue");  // VolUp
    system("./draw circle 140 215 15 blue");  // VolDown

    if( ucZippyIR )    // Go 
    {    
        system("./draw circle 115 110 15 blue");
    } else {
        kHistory |= kGo;
    }
    
    while( !done )
    {
        key = KeypressEvent();

        kHistory |= key;

        if ((kHistory & kAll) == kAll )
            done = true; 

        if(key & kFF)    { x = 165; y = 260; }
        if(key & kRW)    { x = 68;  y = 260; }
        if(key & kHome)  { x = 173; y = 170; }
        if(key & kPlay)  { x = 173; y = 50;  }
        if(key & kAdd)   { x = 56;  y = 50;  }
        if(key & kBack)  { x = 56;  y = 170; }
        if(key & kPause) { x = 115; y = 260; }
        if(key & kVolDn) { x = 92;  y = 215; }
        if(key & kVolUp) { x = 140; y = 215; }

        if( ucZippyIR )  
        {    
            if(key & kGo)    { x = 115; y = 110; }   
        }    
            
        if(key & kUp)   { strcpy(c,"green"); }
        if(key & kDown) { strcpy(c,"red");   }
        if(key & kHold) { strcpy(c,"white"); }
       
        sprintf( s, "./draw filledcircle %d %d 15 blue %s ", x, y, c );
        system( s );       
    }
    
 
    system( "./draw jpeg 0 0 completed.jpg" );
    return true;
}

//===================================================================


int WheelEvent () 
{
	struct input_event ev[64];
	size_t rd;
	int i;
	short scroll = 0;

	rd = read(fd_wheel, ev, sizeof(struct input_event) * 64);

	if (rd < (int) sizeof(struct input_event)) 
	{
		perror("read error");
		return -1;
	}

	for (i = 0; i < rd / sizeof(struct input_event); i++) 
	{
		if (ev[i].type == EV_REL) 
		{
			scroll += ev[i].value;
		}
	}

	return scroll;
}


bool Zippy ()
{
    bool done = false;
    int scroll = 0;
    int scrollLeft = 0;
    int scrollRight = 0;
    
    bool sRight = false;
    bool sLeft  = false;
    bool eRight = false;
    bool eLeft  = false;

  	system( "./draw jpeg 0 0 BeginZippy.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();
    wait_for_keypress();
    
  	system( "./draw jpeg 0 0 Keypad.jpg" );
    system( "./draw circle 160 110 15 blue" );
    system( "./draw circle  75 110 15 blue" );

    while (!done)
    {
        scroll = WheelEvent();

        // look for 360 degrees in each direction
        if (scroll < 0)
            scrollRight += abs(scroll);
        if (scroll > 0)
            scrollLeft += abs(scroll);            

        if ( !sRight && !eRight )
        {
            if (scrollRight > 0)
            {
                system("./draw filledcircle 75 110 15 blue red");
                sRight = true;
            }
        }
        if( sRight && !eRight )
        {
            if (scrollRight >= 12)
            {
                system("./draw filledcircle 75 110 15 blue green");
                eRight = true;
            
            }
        }
        
        if ( !sLeft && !eLeft )
        {
            if (scrollLeft > 0)
            {
                system("./draw filledcircle 160 110 15 blue red");
                sLeft = true;
            }
        }
        if( sLeft && !eLeft )
        {
            if (scrollLeft >= 12)
            {
                system("./draw filledcircle 160 110 15 blue green");
                eLeft = true;            
            }
        }
        
        if( eRight && eLeft )
            done = true;
    }

    system( "./draw jpeg 0 0 completed.jpg" );
    return false;
}

//===================================================================

bool LCD ()
{   
    int x, y;
    char s[512];
    unsigned int c;
    
  	system( "./draw jpeg 0 0 BeginLCD.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

    system( "./draw fillscreen RED" );
    sleep(1);

    system( "./draw fillscreen GREEN" );
    sleep(1);

    system( "./draw fillscreen BLUE" );
    sleep(1);

    system( "./draw fillscreen WHITE" );
    sleep(1);

  	system( "./draw jpeg 0 0 horzstrips.jpg" );
    sleep(1);
    
  	system( "./draw jpeg 0 0 vertstrips.jpg" );
    sleep(1);
	
    system( "./draw fillscreen BLACK" );
    sleep(1);
	
  	system( "./draw jpeg 0 0 gradient.jpg" );
    sleep(1);
 
  	system( "./draw jpeg 0 0 completed.jpg" );
    return true;
}

//===================================================================

bool LCDBacklight()
{
    int fd;
    unsigned short value;
    unsigned long wait;
    unsigned char cnt = 13;
    char tmp[32];

  	system( "./draw jpeg 0 0 BeginLCDBacklight.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

  	system( "./draw jpeg 0 0 BeginLCDBacklight.jpg" );

   	system( "jivectl 11 0xFFFF" );

	while (cnt)
	{	
        if (value > 0)
            value = 0x0000;
        else
            value = 0xFFFF;
	    
	    sprintf (tmp, "jivectl 11 %d", value);
    	system (tmp);
    	
        usleep(250000L);
                
    	cnt--;
    }
    
    system( "./draw jpeg 0 0 completed.jpg" );
    return true;
}


//===================================================================

bool KeyBacklight()
{
    int fd;
    unsigned long wait;
    unsigned short value;
    unsigned char cnt = 13;
    char tmp[32];
    
  	system( "./draw jpeg 0 0 BeginKeypadBacklight.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

  	system( "./draw jpeg 0 0 BeginKeypadBacklight.jpg" );

   	system( "jivectl 13 0xFFFF" );

	while (cnt)
	{	
        if (value > 0)
            value = 0x0000;
        else
            value = 0xFFFF;
	    
	    sprintf (tmp, "jivectl 13 %d", value);
    	system (tmp);
    	
        usleep(250000L);
                
    	cnt--;
    }
    
    system( "./draw jpeg 0 0 completed.jpg" );
    return true;
}


//===================================================================

typedef struct motion_event {
    short x;
    short y;
    short z;
} motion;

motion MotionEvent() 
{
	struct motion_event motion;
	struct input_event ev[64];
	size_t rd;
	int i;
	short n = 0;
	short last_x = 0;
	short last_y = 0;
	short last_z = 0;

	rd = read(fd_motion, ev, sizeof(struct input_event) * 64);

	if (rd < (int) sizeof(struct input_event)) 
	{
		printf("Motion read error\n");
	
		motion.x = 0;
		motion.y = 0;
		motion.z = 0;
		return motion;
	}

	for (i = 0; i < rd / sizeof(struct input_event); i++) 
	{
		if (ev[i].type == EV_SYN)    // 0x00
		{
			n++;
    		motion.x = last_x;  // motion.x += last_x;
			motion.y = last_y;  // motion.y += last_y; 
			motion.z = last_z;  // motion.z += last_z;
		}
		else 
        if (ev[i].type == EV_ABS)    // 0x03
		{
			// motion event
			// accumulate with new values and unchanged values
			switch (ev[i].code) 
			{
			case ABS_X:
				last_x = ev[i].value;
				break;
			case ABS_Y:
				last_y = ev[i].value;
				break;
			case ABS_Z:
				last_z = ev[i].value;
				break;
			}
		}
	}

	if (n > 0) 
	{
		motion.x /= n;
		motion.y /= n;
		motion.z /= n;
	}
 
 	return motion;
}


bool Accelerometer()
{
	struct motion_event motion;
	struct motion_event MAXmotion;
    char s[128];

  	system( "./draw jpeg 0 0 BeginAccelerometer.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

    bool done = false;
    MAXmotion.x = 0;
    MAXmotion.y = 0;
    MAXmotion.z = 0;

  	system( "./draw jpeg 0 0 BlankBox.jpg" );
  	system( "./draw filledrect 55 120 175 140 blue white" );
  	system( "./draw filledrect 55 150 175 170 blue white" );
  	system( "./draw filledrect 55 180 175 200 blue white" );
    
    while(!done)
    {    
        motion = MotionEvent();

        if (MAXmotion.x < abs(motion.x))
        {
            MAXmotion.x = abs(motion.x);
            sprintf( s, "./draw filledrect 55 120 %d 140 blue green", 55 + (MAXmotion.x/127*120) );
            printf("dcs: %s \n", s);
            system( s );       
        }
        if (MAXmotion.y < abs(motion.y))
        {
            MAXmotion.y = abs(motion.y);
            sprintf( s, "./draw filledrect 55 150 %d 170 blue green", 55 + (MAXmotion.y/127*120) );
            printf("dcs: %s \n", s);
            system( s );       
        }
        if (MAXmotion.z < abs(motion.z))
        {
            MAXmotion.z = abs(motion.z);
            sprintf( s, "./draw filledrect 55 180 %d 200 blue green", 55 + (MAXmotion.z/127*120) );
            printf("dcs: %s \n", s);
            system( s );       
        }
        
        if((MAXmotion.x > 20) && (MAXmotion.y > 20) && (MAXmotion.z > 20))
            done = true;
    }

    system( "./draw jpeg 0 0 completed.jpg" );
    return true;
}


//===================================================================

bool Battery()
{
  	system( "./draw jpeg 0 0 BeginBattery.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

  	system( "./draw jpeg 0 0 BeginBattery.jpg" );


//   	system( "jivectl 17" );

    system( "./draw jpeg 0 0 completed.jpg" );

}


//===================================================================


bool IR()
{
  	system( "./draw jpeg 0 0 BeginIR.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

  	system( "./draw jpeg 0 0 BeginIR.jpg" );

    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    usleep(250000L);
    system ("testir 0x768940BF \n");
    
    system( "./draw jpeg 0 0 completed.jpg" );
    return true;    
}

//===================================================================

bool Audio()
{
  	system( "./draw jpeg 0 0 BeginAudio.jpg" );
    usleep(500000L);
    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
    wait_for_keypress();

  	system( "./draw jpeg 0 0 BeginAudio.jpg" );

    // adjust mixer for left channel
    system("amixer -c 0 sset Headphone,0 100%,0% unmute");
    system("aplay sample.wav");

    // adjust mixer for right channel
    system("amixer -c 0 sset Headphone,0 0%,100% unmute");
    system("aplay sample.wav");

    // adjust mixer for both channels
    system("amixer -c 0 sset Headphone,0 100%,100% unmute");
    system("aplay sample.wav");

    system( "./draw jpeg 0 0 completed.jpg" );
    return true;
}

//===================================================================

void wait_for_keypress()
{
    while(1)
    { 
        unsigned short b = KeypressEvent();

        if( b & kUp )
            return;
    }
}

unsigned short wait_for_a_key( unsigned int searchKeys )
{
    while(1)
    {
        unsigned short b = KeypressEvent();
	
        if( b & kUp )
            if( b & searchKeys )
                return b;
    }
}

//===========================================================================

int main (int argc, char **argv)
{
    unsigned char kTest;

    InitializeIO();

    system( "./draw jpeg 0 0 BeginIQC.jpg" );
    usleep(500000);
    
    // Press 1 for BON, Press 2 for IQC    
    system( "./draw jpeg 0 0 PressTestKey.jpg" );
    kTest = wait_for_a_key( kAdd + kPlay + kBack );

    printf("kTest: %d \n", kTest);
    
    if( kTest == kBack )
    {
	printf("Exiting IQCtest");
        return(0);
    }
    
    if( kTest == kPlay )
        ucZippyIR = 1;
    
    Keypad();
    sleep(1);
    
    if( ucZippyIR )
    {
        Zippy();
        sleep(1);
    }
    
    KeyBacklight();
    sleep(1);
    
    LCD();
    sleep(1);
    
    LCDBacklight();
    sleep(1);
        
    Accelerometer();
    sleep(1);

    if( ucZippyIR )
    {    
        IR();
        sleep(1);
    }
    
    Audio();
    sleep(1);

//    Battery();
//    usleep(500000L);
//    system( "./draw jpeg 0 0 PressAnyKey.jpg" );
//    wait_for_keypress();
    
    system( "./draw jpeg 0 0 EndIQC.jpg" );
    
    int cnt = 0;

    while(1)
    {
        wait_for_keypress();
        cnt++;
        if(cnt > 20)
            return(0);
    }
}


//===================================================================



