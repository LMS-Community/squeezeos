
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <math.h>

#include <setjmp.h>
#include "../../system/jpeg-6b/jpeglib.h"

#define FBDEVFILE "/dev/fb0"

// 16 bits per pixel: 5 bits red, 6 bits green and 5 bits blue
// convert a 24-bit RGB color to RGB-565

#define RGB_TO_565(r,g,b) (unsigned short) ((r & 0xf8)<<8) | ((g & 0xfc)<<3) | ((b & 0xf8)>>3)

//===========================================================================

struct fb_var_screeninfo idisp;
unsigned short *ddisp;


void pixel(int x, int y, unsigned short color)
{
    *(ddisp + y * idisp.xres + x) = color;
}


void line(int x, int y, int xx, int yy, unsigned short color)
{    
    int dXY = 0;
    
    int dx = (xx-x);
    int dy = (yy-y);
            
    //Direction pointer.
    int step_x = 0;
    int step_y = 0;

    //Moving right step +1 else -1
    if(dx >= 0)
        step_x = 1;
    else
    {
        step_x = -1; 
        dx = -dx;
    }
    
    if(dy >= 0)                                     
        step_y = 1;
    else
    {
        step_y = -1;
        dy = -dy;
    }
            
    dXY = dy - dx;

    int i; 

    if(dx >= dy)
    {
        //Step x direction by one until the end of width.
        for(i=0; i<dx; i++)
        {
            pixel(x, y, color);

            //Adjust error_term and step down or up by one in the y path.
            if(dXY > 0)
            {
                dXY -= dx;     //err minus the width
                y += step_y;   //Step down or up by one.
            }
            
            dXY += dy;      //Add err_term by the height
            x += step_x;    //step right or left
        }
    }
    else
    {
        //Step y direction by one until the end of height.
        for(i=0; i<dy; i++)
        {
            pixel(x, y, color);

            //Adjust error_term and step down or up by one in the x path.
            if(dXY > 0)
            {
                dXY -= dy;     //err minus the width
                x += step_x;   //Step down or up by one.
            }
            
            dXY += dx;         //Add err_term by the height
            y += step_y;       //step right or left
        }
    
    }   
}


void rect(int x, int y, int xx, int yy, unsigned short color)
{
    line(x, y, x, yy, color);
    line(x, y, xx, y, color);
    line(xx, y, xx, yy, color);
    line(x, yy, xx, yy, color); 
}


void filledrect(int x, int y, int xx, int yy, unsigned short color, unsigned short fillcolor)
{
    int i;
    
    for(i=y; i<yy; i++)
        line(x+1, i+1, xx-1, i-1, fillcolor);

    line( x,  y,  x, yy, color);
    line( x,  y, xx , y, color);
    line(xx,  y, xx, yy, color);
    line( x, yy, xx, yy, color); 
}


void polygon(int n, int coord[], unsigned short color)
{
    if( n >= 2 )
    {
        int count;
        line( coord[0], coord[1], coord[2], coord[3], color);

	    for(count=1; count<(n-1); count++)
		    line( coord[(count*2)], coord[((count*2)+1)], 
		          coord[((count+1)*2)], coord[(((count+1)*2)+1)], color );
	}
}


void circle(int h, int k, int r, unsigned short color)
{
    int x=0;
    int y=r;
    int p=(1-r);

    do
	{
	    pixel((h+x),(k+y),color);
	    pixel((h+y),(k+x),color);
	    pixel((h+y),(k-x),color);
	    pixel((h+x),(k-y),color);
	    pixel((h-x),(k-y),color);
	    pixel((h-y),(k-x),color);
	    pixel((h-y),(k+x),color);
	    pixel((h-x),(k+y),color);

	    x++;

	    if(p<0)
		    p+=((2*x)+1);

	    else
		{
		    y--;
		    p+=((2*(x-y))+1);
		}
    }
    while(x<=y);
}


void filledcircle(int h, int k, int r, unsigned short color, unsigned short fillcolor)
{
    int x=0;
    int y=r;
    int p=(1-r);

    do
	{
	    pixel((h+x),(k+y),color);
	    pixel((h+y),(k+x),color);
	    pixel((h+y),(k-x),color);
	    pixel((h+x),(k-y),color);
	    pixel((h-x),(k-y),color);
	    pixel((h-y),(k-x),color);
	    pixel((h-y),(k+x),color);
	    pixel((h-x),(k+y),color);

        line(h-x+1, k+y, h+x-1, k+y, fillcolor);
        line(h-x+1, k-y, h+x-1, k-y, fillcolor);
        line(h-y+1, k+x, h+y-1, k+x, fillcolor);
        line(h-y+1, k-x, h+y-1, k-x, fillcolor);

	    x++;

	    if(p<0)
		    p+=((2*x)+1);

	    else
		{
		    y--;
		    p+=((2*(x-y))+1);
		}
    }
    while(x<=y);
}


void ellipse(int h, int k, int a, int b, unsigned short color)
{
    float aa=(a*a);
    float bb=(b*b);
    float aa2=(aa*2);
    float bb2=(bb*2);

    float x=0;
    float y=b;

    float fx=0;
    float fy=(aa2*b);

    float p=(int)(bb-(aa*b)+(0.25*aa)+0.5);

    pixel((h+x),(k+y),color);
    pixel((h+x),(k-y),color);
    pixel((h-x),(k-y),color);
    pixel((h-x),(k+y),color);

    while(fx<fy)
	{
	    x++;
	    fx+=bb2;

	    if(p<0)
		    p+=(fx+bb);

	    else
		{
		    y--;
		    fy -= aa2;
		    p += (fx+bb-fy);
		}

        pixel((h+x),(k+y),color);
	    pixel((h+x),(k-y),color);
	    pixel((h-x),(k-y),color);
	    pixel((h-x),(k+y),color);
	}

    p = (int)((bb*(x+0.5)*(x+0.5))+(aa*(y-1)*(y-1))-(aa*bb)+0.5);

    while(y>0)
    {
        y--;
        fy-=aa2;

        if(p>=0)
	        p+=(aa-fy);

        else
	    {
	        x++;
	        fx+=bb2;
	        p+=(fx+aa-fy);
	    }

        pixel((h+x),(k+y),color);
        pixel((h+x),(k-y),color);
        pixel((h-x),(k-y),color);
        pixel((h-x),(k+y),color);
    }
}


void filledellipse(int h, int k, int a, int b, unsigned short color, unsigned short fillcolor)
{
    float aa=(a*a);
    float bb=(b*b);
    float aa2=(aa*2);
    float bb2=(bb*2);

    float x=0;
    float y=b;

    float fx=0;
    float fy=(aa2*b);

    float p=(int)(bb-(aa*b)+(0.25*aa)+0.5);

    pixel((h+x),(k+y),color);
    pixel((h+x),(k-y),color);
    pixel((h-x),(k-y),color);
    pixel((h-x),(k+y),color);

    line(h-x+1, k+y, h+x-1, k+y, fillcolor);
    line(h-x+1, k-y, h+x-1, k-y, fillcolor);
    line(h-y+1, k+x, h+y-1, k+x, fillcolor);
    line(h-y+1, k-x, h+y-1, k-x, fillcolor);

    while(fx<fy)
	{
	    x++;
	    fx+=bb2;

	    if(p<0)
		    p+=(fx+bb);

	    else
		{
		    y--;
		    fy -= aa2;
		    p += (fx+bb-fy);
		}

        pixel((h+x),(k+y),color);
	    pixel((h+x),(k-y),color);
	    pixel((h-x),(k-y),color);
	    pixel((h-x),(k+y),color);

        line(h-x+1, k+y, h+x-1, k+y, fillcolor);
        line(h-x+1, k-y, h+x-1, k-y, fillcolor);
        line(h-y+1, k+x, h+y-1, k+x, fillcolor);
        line(h-y+1, k-x, h+y-1, k-x, fillcolor);
    }

    p = (int)((bb*(x+0.5)*(x+0.5))+(aa*(y-1)*(y-1))-(aa*bb)+0.5);

    while(y>0)
    {
        y--;
        fy-=aa2;

        if(p>=0)
	        p+=(aa-fy);

        else
	    {
	        x++;
	        fx+=bb2;
	        p+=(fx+aa-fy);
	    }

        pixel((h+x),(k+y),color);
        pixel((h+x),(k-y),color);
        pixel((h-x),(k-y),color);
        pixel((h-x),(k+y),color);

        line(h-x+1, k+y, h+x-1, k+y, fillcolor);
        line(h-x+1, k-y, h+x-1, k-y, fillcolor);
        line(h-y+1, k+x, h+y-1, k+x, fillcolor);
        line(h-y+1, k-x, h+y-1, k-x, fillcolor);
    }
}

void fillscreen(unsigned int color)
{
	int y, x, offset;
	
	for (y = 0; y < idisp.yres; y++) 
	{
		offset = y * idisp.xres;
	
		for (x = 0; x < idisp.xres; x++) 
		    *(ddisp + offset + x) = color;
	}
}


//================================================================================
//========================= JPEG DECOMPRESSION INTERFACE =========================

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

// Replace the standard error_exit method
void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

static unsigned short CMYK_TO_565(int c, int m, int y, int k, bool inverted)
{
    int r, g, b;
    
    if (inverted) {
        c = 255 - c;  m = 255 - m;  y = 255 - y;  k = 255 - k;
    }

	c = c/100;  m = m/100;  y = y/100; k = k/100;
	r = 1-(c*(1-k))-k;  g = 1-(m*(1-k))-k;  b = 1-(y*(1-k))-k;
	r = (r*255) & 0xFF;  g = (g*255) & 0xFF;  b = (b*255) & 0xFF;
    if(r<0) r = 0;  if(g<0) g = 0;  if(b<0) b = 0;
     	
    RGB_TO_565( (unsigned char)r, (unsigned char)g, (unsigned char)b );
}

// JPEG 
int jpeg (int xx, int yy, char *filename, unsigned long transparent)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    FILE * infile;		    /* source file */
    unsigned int i, j;
    volatile JSAMPROW row = 0;
    JSAMPROW rowptr[1];
    JDIMENSION nrows;
    int channels = 3;
    int inverted = 0;

    if ((infile = fopen(filename, "rb")) == NULL) 
    {
        printf("Cannot open %s\n", filename);
        return 0;
    }

    // Allocate and initialize JPEG decompression object
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    
    if (setjmp(jerr.setjmp_buffer)) 
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        printf("JPEG error in %s\n", filename);
        return 0;
    }

    jpeg_create_decompress(&cinfo);

    // Specify data source
    jpeg_stdio_src(&cinfo, infile);

    // Read file parameters with jpeg_read_header()
    (void) jpeg_read_header(&cinfo, TRUE);

    //--------------------------

    if((cinfo.jpeg_color_space == JCS_CMYK) || (cinfo.jpeg_color_space == JCS_YCCK))
        cinfo.out_color_space = JCS_CMYK;
    else
        cinfo.out_color_space = JCS_RGB;    
    
    // Decompress the image
    (void) jpeg_start_decompress(&cinfo);
    
    if (cinfo.out_color_space == JCS_RGB) 
    {
        channels = 3;
    } 
    else if (cinfo.out_color_space == JCS_CMYK) 
    {
        jpeg_saved_marker_ptr marker;
        channels = 4;
 
        marker = cinfo.marker_list;
        while (marker) 
        {
            if ((marker->marker == (JPEG_APP0 + 14)) && (marker->data_length >= 12) && 
                    (!strncmp((const char *) marker->data, "Adobe", 5))) 
            {
                inverted = 1;
                break;
            }
            marker = marker->next;
        }
    }    
    
    row = malloc(cinfo.output_width * channels * sizeof(JSAMPLE));
    //memset( row, 0, cinfo.output_width * channels * sizeof(JSAMPLE));
    rowptr[0] = row;

    if (cinfo.out_color_space == JCS_CMYK) 
    {
        for (i = 0; i < cinfo.output_height; i++) 
        {
            register JSAMPROW currow = row;
            nrows = jpeg_read_scanlines (&cinfo, rowptr, 1);
            if (nrows != 1) {
                printf("JPEG error: jpeg_read_scanlines returns %u, expected 1", nrows);
            }
            for (j = 0; j < cinfo.output_width; j++, currow += 4) 
            {
                unsigned short color;
                color = CMYK_TO_565 (currow[0], currow[1], currow[2], currow[3], inverted);
                
                if( color != transparent )
                    pixel(j, i, color);
            }
        }
    } else {
        for (i = 0; i < cinfo.output_height; i++) 
        {
            register JSAMPROW currow = row;
            nrows = jpeg_read_scanlines (&cinfo, rowptr, 1);
            if (nrows != 1) 
            {
                printf("JPEG error: jpeg_read_scanlines returns %u, expected 1", nrows);
            }
            for (j = 0; j < cinfo.output_width; j++, currow += 3) 
            {
                unsigned short color;
                color =  RGB_TO_565(currow[0], currow[1], currow[2]);

                if( color != transparent )
                    pixel(j, i, color);
            }
        }
    } 

    // Finish decompression
    (void) jpeg_finish_decompress(&cinfo);

    // Release JPEG decompression object
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return 1;
}


//================================================================================
//
    
void usage(void)
{
    printf("Usage: \n");
    printf("   draw pixel x y color \n");
    printf("   draw line x y xx yy color \n");
    printf("   draw rect x y xx yy color \n");
    printf("   draw filledrect x y xx yy color fillcolor \n");
    printf("   draw polygon n coord[] color \n");
    printf("   draw circle h k r color \n");
    printf("   draw filledcircle h k r color fillcolor \n");
    printf("   draw ellipse h k a b color \n");
    printf("   draw filledellipse h k a b color fillcolor \n");
    printf("   draw fillscreen color \n");
    printf("   draw jpeg x y filename \n");
    printf("   draw tjpeg x y tranparentcolor filename\n");
    printf("  \n");
    printf("      color: \n");
    printf("        [by RGB  ]  (100,200,89) \n");
    printf("        [by value]  11245 \n");
    printf("        [by name ]  red blue ivory slateblue white ...\n");
    printf("  \n");
    printf("   draw color\n");
    printf("      returns list of colors\n");
    exit(-1);
}


//================================================================================
//

struct color { char name[24]; unsigned int r; unsigned int g; unsigned int b; }; 

const struct color colors[] = {
    { "snow",       	    255, 250, 250 },
    { "ghostwhite", 	    248, 248, 255 },
    { "whitesmoke", 	    245, 245, 245 },
    { "gainsboro", 	 	    220, 220, 220 },
    { "floralwhite", 	    255, 250, 240 },
    { "oldlace",     	    253, 245, 230 },
    { "linen", 		 	    250, 240, 230 },
    { "antiquewhite", 	    250, 235, 215 },
    { "papayawhip", 	    255, 239, 213 },
    { "blanchedalmond", 	255, 235, 205 },
    { "bisque", 		    255, 228, 196 },
    { "peachpuff", 	 	    255, 218, 185 },
    { "navajowhite", 	    255, 222, 173 },
    { "moccasin", 		    255, 228, 181 },
    { "cornsilk", 		    255, 248, 220 },
    { "ivory", 		 	    255, 255, 240 },
    { "lemonchiffon", 	    255, 250, 205 },
    { "seashell", 	 	    255, 245, 238 },
    { "honeydew", 	 	    240, 255, 240 },
    { "mintcream", 	 	    245, 255, 250 },
    { "azure", 		 	    240, 255, 255 },
    { "aliceblue", 		    240, 248, 255 },
    { "lavender", 		    230, 230, 250 },
    { "lavenderblush", 	    255, 240, 245 },
    { "mistyrose", 	 	    255, 228, 225 },
    { "white",	 	 	    255, 255, 255 },
    { "darkslategray", 	     47,  79,  79 },
    { "dimgray", 	 	    105, 105, 105 },
    { "slategray", 	 	    112, 128, 144 },
    { "lightslategrey", 	119, 136, 153 },
    { "grey", 		 	    190, 190, 190 },
    { "lightgrey", 	 	    211, 211, 211 },
    { "midnightblue", 	     25,  25, 112 },
    { "navy", 		 	      0,   0, 128 },
    { "navyblue", 	 	      0,   0, 128 },
    { "cornflowerblue", 	100, 149, 237 },
    { "darkslateblue", 	     72,  61, 139 },
    { "slateblue", 	 	    106,  90, 205 },
    { "mediumslateblue",    123, 104, 238 },
    { "lightslateblue", 	132, 112, 255 },
    { "mediumblue", 	      0,   0, 205 },
    { "royalblue", 	 	     65, 105, 225 },
    { "blue", 		 	      0,   0, 255 },
    { "dodgerblue", 	     30, 144, 255 },
    { "deepskyblue", 	      0, 191, 255 },
    { "skyblue", 	 	    135, 206, 235 },
    { "lightskyblue", 	    135, 206, 250 },
    { "steelblue", 	 	     70, 130, 180 },
    { "lightsteelblue", 	176, 196, 222 },
    { "lightblue", 	 	    173, 216, 230 },
    { "powderblue", 	    176, 224, 230 },
    { "paleturquoise", 	    175, 238, 238 },
    { "darkturquoise", 	      0, 206, 209 },
    { "mediumturquoise",     72, 209, 204 },
    { "turquoise", 	 	     64, 224, 208 },
    { "cyan", 		 	      0, 255, 255 },
    { "lightcyan", 	 	    224, 255, 255 },
    { "cadetblue", 	 	     95, 158, 160 },
    { "mediumaquamarine",   102, 205, 170 },
    { "aquamarine", 	    127, 255, 212 },
    { "darkgreen", 	 	      0, 100,   0 },
    { "darkolivegreen", 	 85, 107,  47 },
    { "darkseagreen", 	    143, 188, 143 },
    { "seagreen", 	 	     46, 139,  87 },
    { "mediumseagreen", 	 60, 179, 113 },
    { "lightseagreen", 	     32, 178, 170 },
    { "palegreen", 	 	    152, 251, 152 },
    { "springgreen", 	      0, 255, 127 },
    { "lawngreen", 	        124, 252,   0 },
    { "green", 		          0, 255,   0 },
    { "mediumspringgreen",	  0, 250, 154 },
    { "greenyellow", 	    173, 255,  47 },
    { "limegreen", 	         50, 205,  50 },
    { "yellowgreen", 	    154, 205,  50 },
    { "forestgreen", 	     34, 139,  34 },
    { "olivedrab", 	        107, 142,  35 },
    { "darkkhaki", 	        189, 183, 107 },
    { "khaki", 		        240, 230, 140 },
    { "palegoldenrod", 	    238, 232, 170 },
    { "lightgoldenrodyellow", 	 250, 250, 210 },
    { "lightyellow", 	    255, 255, 224 },
    { "yellow", 		    255, 255,   0 },
    { "gold", 		        255, 215,   0 },
    { "lightgoldenrod",     238, 221, 130 },
    { "goldenrod", 	        218, 165,  32 },
    { "darkgoldenrod", 	    184, 134,  11 },
    { "rosybrown", 	        188, 143, 143 },
    { "indianred", 	        205,  92,  92 },
    { "saddlebrown", 	    139,  69,  19 },
    { "sienna", 		    160,  82,  45 },
    { "peru", 		        205, 133,  63 },
    { "burlywood", 	        222, 184, 135 },
    { "beige", 		        245, 245, 220 },
    { "wheat", 		        245, 222, 179 },
    { "sandybrown", 	    244, 164,  96 },
    { "tan", 		        210, 180, 140 },
    { "chocolate", 	        210, 105,  30 },
    { "firebrick", 	        178,  34,  34 },
    { "brown", 		        165,  42,  42 },
    { "darksalmon", 	    233, 150, 122 },
    { "salmon", 		    250, 128, 114 },
    { "lightsalmon", 	    255, 160, 122 },
    { "orange", 		    255, 165,   0 },
    { "darkorange", 	    255, 140,   0 },
    { "coral", 		        255, 127,  80 },
    { "lightcoral",	        240, 128, 128 },
    { "tomato",		        255,  99,  71 },
    { "orangered",	        255,  69,   0 },
    { "red",		        255,   0,   0 },
    { "hotpink", 	        255, 105, 180 },
    { "deeppink", 	        255,  20, 147 },
    { "pink", 		        255, 192, 203 },
    { "lightpink", 	        255, 182, 193 },
    { "palevioletred", 	    219, 112, 147 },
    { "maroon", 		    176,  48,  96 },
    { "mediumvioletred",    199,  21, 133 },
    { "violetred", 	        208,  32, 144 },
    { "magenta", 	        255,   0, 255 },
    { "violet", 		    238, 130, 238 },
    { "plum", 		        221, 160, 221 },
    { "orchid", 		    218, 112, 214 },
    { "mediumorchid", 	    186,  85, 211 },
    { "darkorchid", 	    153,  50, 204 },
    { "darkviolet", 	    148,   0, 211 },
    { "blueviolet", 	    138,  43, 226 },
    { "purple", 		    160,  32, 240 },
    { "mediumpurple", 	    147, 112, 219 },
    { "thistle",     	    216, 191, 216 }
};


unsigned short parsecolor( const char *line )
{
    int itemsparsed = 0;
    char field[128];
    int n;
    
    char ret[512];
    int r, g, b;
    
    // [by RGB  ]  (100,200,89)
    itemsparsed = sscanf ( line, "( %d, %d, %d )", r, g, b );
    if (itemsparsed > 0)
        return ( RGB_TO_565(r,g,b) );

    // [by value]  11245
    itemsparsed = sscanf ( line, " %d ", n );
    if (itemsparsed > 0)
    {
        if(( n >= 0x0000 ) && ( n <= 0xFFFF ))
            return n;
        else
            return 0x0000;
    }
    
    // [by name ]  RED BLUE WHITE
    itemsparsed = sscanf ( line, "%s", &field[0]);
    if (itemsparsed > 0)
    {
        int i;
        unsigned char numcolors = sizeof(colors) / sizeof(struct color);
        for( i = 0; i < numcolors; i++ )
        {
            if( strcmp( colors[i].name, line ) == 0 )
                return ( RGB_TO_565( colors[i].r, colors[i].g, colors[i].b ));
        }
    }
    
    return 0x0000;
}


inline char *toLowerCase( char *str )
{
    int j = strlen(str) + 1;
    int i = 0;
    while(j--)
    {
        str[i] = tolower(str[i]);
        i++;
    }
    return str;
}

//================================================================================
//
    
int main(int argv, char **args)
{
	int hdisp, ret;

    int x, y, xx, yy, a, b, *coord;
    unsigned short color, fillcolor;
    char filename[1024];
    
    if( argv < 2 )
        usage();

	//==============================================================
	// Initialize

	hdisp = open(FBDEVFILE, O_RDWR);
	
	if(hdisp < 0) 
	{
		perror("fbdev open");
		exit(1);
	}
	
	ret = ioctl(hdisp, FBIOGET_VSCREENINFO, &idisp);

	if(ret < 0) 
	{
		perror("fbdev ioctl");
		exit(1);
	}

	if(idisp.bits_per_pixel != 16) 
	{
		fprintf(stderr, "bpp is not 16\n");
		exit(1);
	}

	ddisp = (unsigned short *) mmap(0, idisp.xres * idisp.yres * 16 / 8, 
	                        PROT_READ|PROT_WRITE, MAP_SHARED, hdisp, 0);

	if((unsigned)ddisp == (unsigned)-1) 
	{
		perror("fbdev mmap");
		exit(1);
	}

    //==============================================================
    // Parse command line options
    
    if (strncmp(args[1], "pixel", 5) == 0)
    {
        //  #draw pixel x y color
        if (argv == 5)
        {
            unsigned short c;
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            c = parsecolor( toLowerCase(args[4]) );
            pixel(x,y,c);            
            printf("  pixel (%d, %d, %d)\n", x, y, c);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "line", 4) == 0)
    { 
        //  #draw line x y xx yy color
        if (argv == 7)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            xx = atoi(args[4]); 
            yy = atoi(args[5]); 
            color = parsecolor( toLowerCase(args[6]) );
            line(x,y,xx,yy,color);
            printf("  line (%d, %d, %d, %d, %d)\n", x, y, xx, yy, color);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "rect", 4) == 0)
    { 
        //  #draw rect x y xx yy color
        if (argv == 7)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            xx = atoi(args[4]); 
            yy = atoi(args[5]); 
            color = parsecolor( toLowerCase(args[6]) );
            rect(x,y,xx,yy,color);
            printf("  rect (%d, %d, %d, %d, %d)\n", x, y, xx, yy, color);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "filledrect", 10) == 0)
    { 
        //  #draw filledrect x y xx yy color fillcolor
        if (argv == 8)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            xx = atoi(args[4]); 
            yy = atoi(args[5]); 
            color = parsecolor( toLowerCase(args[6]) );
            fillcolor = parsecolor( toLowerCase(args[7]) );
            filledrect(x,y,xx,yy,color,fillcolor);
            printf("  filledrect(%d, %d, %d, %d, %d, %d)\n", x, y, xx, yy, color, fillcolor);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "polygon", 7) == 0)
    { 
        //  #draw polygon n coord[] color
        if (argv == 5)
        {
            x = atoi(args[2]);
            coord = malloc(x * sizeof(int)); 
            for( y=0; y<x; y++)
                coord[y] = args[3][y];
            color = parsecolor( toLowerCase(args[4]) );
            polygon(x, coord, color);
            printf("  polygon (%d, coords, %d)\n", x, color);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "circle", 6) == 0)
    { 
        //  #draw circle h k r color
        if (argv == 6)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            a = atoi(args[4]); 
            color = parsecolor( toLowerCase(args[5]) );
            circle(x,y,a,color);
            printf("  circle (%d, %d, %d, %d)\n", x, y, a, color);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "filledcircle", 12) == 0)
    { 
        //  #draw filledcircle h k r color fillcolor
        if (argv == 7)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            a = atoi(args[4]); 
            color = parsecolor( toLowerCase(args[5]) );
            fillcolor = parsecolor( toLowerCase(args[6]) );
            filledcircle(x,y,a,color,fillcolor);
            printf("  circle (%d, %d, %d, %d. %d)\n", x, y, a, color, fillcolor);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "ellipse", 7) == 0)
    { 
        //  #draw ellipse h k a b color
        if (argv == 7)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            a = atoi(args[4]); 
            b = atoi(args[5]);
            color = parsecolor( toLowerCase(args[6]) );
            ellipse(x,y,a,b,color);
            printf("  ellipse (%d, %d, %d, %d. %d)\n", x, y, a, b, color);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "filledellipse", 13) == 0)
    { 
        //  #draw filledellipse h k a b color fillcolor
        if (argv == 8)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]); 
            a = atoi(args[4]); 
            b = atoi(args[5]);
            color = parsecolor( toLowerCase(args[6]) );
            fillcolor = parsecolor( toLowerCase(args[7]) );
            filledellipse(x,y,a,b,color,fillcolor);
            printf("  filledellipse (%d, %d, %d, %d. %d, %d)\n", x, y, a, b, color, fillcolor);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "fillscreen", 10) == 0)
    {
        //  #draw fillscreen color
        if (argv == 3)
        {
            color = parsecolor( toLowerCase(args[2]) );
            fillscreen(color);
            printf("  fillscreen (%d)\n", color);
        }    
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "jpeg", 4) == 0)
    {
        //  #draw jpeg x y filename
        if (argv == 5)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]);
            strcpy(filename, args[4]);
            jpeg(x,y,filename,0x10000);  // transparent value greater than color 
            printf("  jpeg %d %d %s\n", x, y, filename);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "tjpeg", 5) == 0)
    {
        //  #draw jpeg x y transparentColor filename
        if (argv == 5)
        {
            x = atoi(args[2]); 
            y = atoi(args[3]);
            color = parsecolor( toLowerCase(args[4]) );
            strcpy(filename, args[5]);
            jpeg( x, y, filename, color);
            printf("  tjpeg %d %d %x %s\n", x, y, color, filename);
        }
        else
        {
            usage();
        }
    }
    else if (strncmp(args[1], "color", 5) == 0)
    {
        // #draw color
        int i;
        unsigned char numcolors = sizeof(colors) / sizeof(struct color);
        
        printf("Defined colors: \n");
        
        for( i = 0; i < numcolors; i++ )
            printf( "   %s [%3d,%3d,%3d] \n", colors[i].name, colors[i].r, colors[i].g, colors[i].b );
    }
    
	close(hdisp);
	exit(0);
	return 0;
}























