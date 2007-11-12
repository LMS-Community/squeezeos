#include <stdio.h>
#include <fcntl.h>

const char* dev = "/dev/misc/irtx";

int main(int argv, char **args) {
	int fd;
	int DrvCount;
	int Tx[100];
	int lead_mark, lead_space, one_mark, one_space, zero_mark, zero_space, last_mark;
	int i;
	char szIRCmd[11] = "0x768940BF";	// Default: SB power toggle command
	unsigned long ircmd;

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


	if( argv > 1) {
		strncpy( szIRCmd, args[1], 10);
		szIRCmd[10] = '\0';
	}

	ircmd = strtoul( szIRCmd, 0, 16);

	// Lead
	Tx[0]=lead_mark;
	Tx[1]=lead_space;

	// Command
	for( i = 65; i >= 2; i -= 2) {
		if( ircmd & 0x01) {
			printf( "1 ");
			Tx[i-1]=one_mark;
			Tx[i]=one_space;
		} else {
			printf( "0 ");
			Tx[i-1]=zero_mark;
			Tx[i]=zero_space;
		}
		ircmd = ircmd >> 1;
	}
	printf( "\n");

	// Last mark
	Tx[66]=last_mark;

	write(fd, Tx, DrvCount);
	printf("IR Tx OK - %s\n", szIRCmd);
}
