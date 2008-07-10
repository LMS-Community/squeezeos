#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <math.h>

#include <setjmp.h>
#include "../../system/jpeg-6b/jpeglib.h"

#define FBDEVFILE "/dev/fb0"


#define RED     RGB_TO_565 (255,   0,   0)
#define CYAN    RGB_TO_565 (255, 255,   0)
#define GREEN   RGB_TO_565 (  0, 255,   0)
#define MAGENTA RGB_TO_565 (  0, 255, 255)
#define BLUE    RGB_TO_565 (  0,   0, 255)
#define WHITE   RGB_TO_565 (255, 255, 255)
#define GRAY    RGB_TO_565 (128, 128, 128)
#define BLACK   RGB_TO_565 (  0,   0,   0)


// 16 bits per pixel: 5 bits red, 6 bits green and 5 bits blue
// convert a 24-bit RGB color to RGB-565

#define RGB_TO_565(r,g,b) (unsigned short) ((r & 0xf8)<<8) | ((g & 0xfc)<<3) | ((b & 0xf8)>>3)

//===========================================================================

struct fb_var_screeninfo idisp;
unsigned short *ddisp;


void Pixel(int x, int y, unsigned short color)
{
    *(ddisp + y * idisp.xres + x) = color;
}

void Line(int x, int y, int xx, int yy, unsigned short color)
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
            Pixel(x, y, color);

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
            Pixel(x, y, color);

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

void Polygon(const int n,const int coord[], unsigned short color)
{
    if( n >= 2 )
    {
        int count;
        Line( coord[0], coord[1], coord[2], coord[3], color);

	    for(count=1; count<(n-1); count++)
		    Line( coord[(count*2)], coord[((count*2)+1)],
		       coord[((count+1)*2)], coord[(((count+1)*2)+1)], color );
	}
}

void Circle(const int h, const int k, const int r, unsigned short color)
{
    int x=0;
    int y=r;
    int p=(1-r);

    do
	{
	    Pixel((h+x),(k+y),color);
	    Pixel((h+y),(k+x),color);
	    Pixel((h+y),(k-x),color);
	    Pixel((h+x),(k-y),color);
	    Pixel((h-x),(k-y),color);
	    Pixel((h-y),(k-x),color);
	    Pixel((h-y),(k+x),color);
	    Pixel((h-x),(k+y),color);

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

void Ellipse(const int h,const int k,const int a,const int b, unsigned short color)
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

    Pixel((h+x),(k+y),color);
    Pixel((h+x),(k-y),color);
    Pixel((h-x),(k-y),color);
    Pixel((h-x),(k+y),color);

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

        Pixel((h+x),(k+y),color);
	    Pixel((h+x),(k-y),color);
	    Pixel((h-x),(k-y),color);
	    Pixel((h-x),(k+y),color);
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

        Pixel((h+x),(k+y),color);
        Pixel((h+x),(k-y),color);
        Pixel((h-x),(k-y),color);
        Pixel((h-x),(k+y),color);
    }
}

void FillScreen(unsigned int color)
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

struct my_error_mgr 
{
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

static unsigned short CMYK_TO_565(int c, int m, int y, int k, bool inverted)
{
    int r, g, b;
    
    if (inverted)
        c = 255 - c;  m = 255 - m;  y = 255 - y;  k = 255 - k;

	c = c/100;  m = m/100;  y = y/100;  k = k/100;
	r = 1-(c*(1-k))-k;   g = 1-(m*(1-k))-k;   b = 1-(y*(1-k))-k;
	r = (r*255) & 0xFF;  g = (g*255) & 0xFF;  b = (b*255) & 0xFF;

    if(r<0) r = 0;  if(g<0) g = 0;  if(b<0) b = 0;
     	
    RGB_TO_565( (unsigned char)r, (unsigned char)g, (unsigned char)b );
}


int JPEG (int xx, int yy, char * filename)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    FILE * infile;
    unsigned int i, j;
    volatile JSAMPROW row = 0;
    JSAMPROW rowptr[1];
    JDIMENSION nrows;
    
    int channels = 3;
    int inverted = 0;

    if ((infile = fopen(filename, "rb")) == NULL) 
    {
        fprintf(stderr, "Can not open %s\n", filename);
        return 0;
    }

    // Allocate and initialize JPEG decompression object
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) 
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
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
            if (nrows != 1) 
            {
                printf("error: jpeg_read_scanlines returns %u, expected 1", nrows);
            }
            for (j = 0; j < cinfo.output_width; j++, currow += 4) 
            {
                unsigned short color;
                color = CMYK_TO_565 (currow[0], currow[1], currow[2], currow[3], inverted);
                Pixel(j, i, color);
            }
        }
    } 
    else 
    {
        for (i = 0; i < cinfo.output_height; i++) 
        {
            register JSAMPROW currow = row;
            nrows = jpeg_read_scanlines (&cinfo, rowptr, 1);
            if (nrows != 1) 
            {
                printf("error: jpeg_read_scanlines returns %u, expected 1", nrows);
            }
            for (j = 0; j < cinfo.output_width; j++, currow += 3) 
            {
                unsigned short color;
                color =  RGB_TO_565(currow[0], currow[1], currow[2]);
                Pixel(j, i, color);
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


int main(int argv, char **args)
{
	int hdisp, ret;
	
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

    int y, x; 
	int t, tt, offset, color;
	long count;

	//==============================================================

	fprintf(stdout, "FactoryTest: TestDisplay Red");
    fflush(stdout);
    getchar();
	fflush(stdin);

    FillScreen( RED );

	//==============================================================

	fprintf(stdout, "FactoryTest: TestDisplay Green");
    fflush(stdout);
    getchar();
	fflush(stdin);

    FillScreen( GREEN );

	//==============================================================

	fprintf(stdout, "FactoryTest: TestDisplay Blue");
    fflush(stdout);
    getchar();
	fflush(stdin);

    FillScreen( BLUE );
    
	//==============================================================

	fprintf(stdout, "FactoryTest: TestDisplay White");
    fflush(stdout);
    getchar();
	fflush(stdin);

    FillScreen( WHITE );

	//==============================================================

    fprintf(stdout, "FactoryTest: TestDisplay Horz Stripes");
    fflush(stdout);
    getchar();
    fflush(stdin);
    
    FillScreen( BLACK );
    for ( y = 0; y < 320; y+=2 )
        Line( 0, y, 240, y, WHITE );

	//==============================================================

    fprintf(stdout, "FactoryTest: TestDisplay Vert Stripes");
    fflush(stdout);
    getchar();
    fflush(stdin);
    
    FillScreen( BLACK );
    for ( x = 0; x < 240; x+=2 )
        Line( x, 0, x, 320, WHITE );

	//==============================================================

	fprintf(stdout, "FactoryTest: TestDisplay Black");
    fflush(stdout);
    getchar();
	fflush(stdin);

    FillScreen( BLACK );
    
	//==============================================================

	fprintf(stdout, "FactoryTest: TestDisplay Color Gradient");
    fflush(stdout);
    getchar();
	fflush(stdin);

	if( argv == 2 ) 
	{
    	char filename[512];
		strcpy( filename, args[1]);
        JPEG (0, 0, filename);
    }
    else
        printf("No JPEG file given\n");
        
	//==============================================================

	close(hdisp);
	exit(0);
	return 0;
}























