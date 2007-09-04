#include <stdio.h>
#include <fcntl.h>

const char* dev = "/dev/misc/irtx";

int main(int argv, char **args) {
	int fd;
	int DrvCount;
	int Tx[100];
	int lead_mark, lead_space, one_mark, one_space, zero_mark, zero_space, last_mark;
	int i;

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "cannot open device %s\n", dev);
		exit(-1);
	}

	// Time values for mark and space for NEC format (regular Slim remote)
	lead_mark = 8800;
	lead_space = 4400;
	one_mark = 550;
	one_space = 1625;
	zero_mark = 550;
	zero_space = 550;
	last_mark = 550;

	// Lead bit:		1	mark and space => 2
	// Command bits: 	32	mark and space => 64
	// Last bit:		1	last mark => 1
	DrvCount=2+64+1;

	// Lead
	Tx[0]=lead_mark;
	Tx[1]=lead_space;

	// SB power toggle command: 0x768940BF
	// 0x07 => 0111
	Tx[2]=zero_mark;
	Tx[3]=zero_space;
	Tx[4]=one_mark;
	Tx[5]=one_space;
	Tx[6]=one_mark;
	Tx[7]=one_space;
	Tx[8]=one_mark;
	Tx[9]=one_space;
	// 0x06 => 0110
	Tx[10]=zero_mark;
	Tx[11]=zero_space;
	Tx[12]=one_mark;
	Tx[13]=one_space;
	Tx[14]=one_mark;
	Tx[15]=one_space;
	Tx[16]=zero_mark;
	Tx[17]=zero_space;
	// 0x08 => 1000
	Tx[18]=one_mark;
	Tx[19]=one_space;
	Tx[20]=zero_mark;
	Tx[21]=zero_space;
	Tx[22]=zero_mark;
	Tx[23]=zero_space;
	Tx[24]=zero_mark;
	Tx[25]=zero_space;
	// 0x09 => 1001
	Tx[26]=one_mark;
	Tx[27]=one_space;
	Tx[28]=zero_mark;
	Tx[29]=zero_space;
	Tx[30]=zero_mark;
	Tx[31]=zero_space;
	Tx[32]=one_mark;
	Tx[33]=one_space;
	// 0x04 => 0100
	Tx[34]=zero_mark;
	Tx[35]=zero_space;
	Tx[36]=one_mark;
	Tx[37]=one_space;
	Tx[38]=zero_mark;
	Tx[39]=zero_space;
	Tx[40]=zero_mark;
	Tx[41]=zero_space;
	// 0x00 => 0000
	Tx[42]=zero_mark;
	Tx[43]=zero_space;
	Tx[44]=zero_mark;
	Tx[45]=zero_space;
	Tx[46]=zero_mark;
	Tx[47]=zero_space;
	Tx[48]=zero_mark;
	Tx[49]=zero_space;
	// 0x0B => 1011
	Tx[50]=one_mark;
	Tx[51]=one_space;
	Tx[52]=zero_mark;
	Tx[53]=zero_space;
	Tx[54]=one_mark;
	Tx[55]=one_space;
	Tx[56]=one_mark;
	Tx[57]=one_space;
	// 0x0F	=> 1111
	Tx[58]=one_mark;
	Tx[59]=one_space;
	Tx[60]=one_mark;
	Tx[61]=one_space;
	Tx[62]=one_mark;
	Tx[63]=one_space;
	Tx[64]=one_mark;
	Tx[65]=one_space;
	// Last mark
	Tx[66]=last_mark;

	write(fd, Tx, DrvCount);
	printf("IR Tx OK - SB power toggle command\n");
}
