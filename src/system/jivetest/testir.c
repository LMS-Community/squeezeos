#include <stdio.h>
#include <fcntl.h>

const char* dev = "/dev/misc/irtx";

// No cmd line arg, sends a SB power toggle ir command
// One cmd line arg (i.e. 0x768940BF) sends a SB default remote ir command
// More than one cmd line arg (max 100), all in hex form like this 0x000000F6
// - First arg is carrier frequency
// - Following args are raw mark or space times
// - Setting the highest bit (i.e. 0x80000000) sends a mark (i.e. with carrier)
// - Not setting the highest bit (i.e. 0x00000000) sends a space

int main(int argv, char **args) {
	#define BUFFER_MAX 100

	int numOfArgs;
	int fd;
	unsigned int Tx[BUFFER_MAX];
	int i;
	unsigned int carrier_frequency;
	unsigned int lead_mark, lead_space, one_mark, one_space, zero_mark, zero_space, last_mark;
	char szIRCmd[11] = "0x768940BF";	// Default: SB power toggle ir command
	unsigned long ircmd;
	char szValue[11] = "0x00000000";

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "cannot open device %s\n", dev);
		exit(-1);
	}

	// First command line argument is the program name
	numOfArgs = argv;
	if( numOfArgs > ( BUFFER_MAX + 1)) {
		numOfArgs = BUFFER_MAX + 1;
	}

	// No cmd line argument -> issue default SB power toggle command
	// One cmd line argument -> regular Slim remote command
	if( numOfArgs <= 2) {
		if( numOfArgs == 2) {
			strncpy( szIRCmd, args[1], 10);
			szIRCmd[10] = '\0';
		}
		ircmd = strtoul( szIRCmd, 0, 16);
		// Time values for mark and space for NEC format (regular Slim remote)
		carrier_frequency = 38000;	// Carrier frequency in Hz
		lead_mark = 8800 | 0x80000000;	// Highest bit set => mark
		lead_space = 4400;		// Highest bit not set => space
		one_mark = 550 | 0x80000000;
		one_space = 1625;
		zero_mark = 550 | 0x80000000;
		zero_space = 550;
		last_mark = 550 | 0x80000000;
		// Lead
		Tx[0]=carrier_frequency;
		Tx[1]=lead_mark;
		Tx[2]=lead_space;
		// Command
		for( i = 66; i >= 3; i -= 2) {
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
		Tx[67]=last_mark;

		// Carrier frequency:	1	=> 1
		// Lead bit:		1	mark and space => 2
		// Command bits: 	32	mark and space => 64
		// Last bit:		1	last mark => 1
		write(fd, Tx, ( 1+2+64+1));
		printf("IR Tx OK - %s\n", szIRCmd);

	// More than two cmd line arguments
	// Pass raw data to ir driver
	} else {
		for( i = 1; i < numOfArgs; i++) {
			strncpy( szValue, args[i], 10);
			szValue[10] = '\0';
			Tx[i-1] = strtoul( szValue, 0, 16);
			//printf("IR Tx[%d] = %x\n", i-1, Tx[i-1]);
		}
		write(fd, Tx, ( numOfArgs - 1));
		printf("IR Tx OK - #values: %d\n", ( numOfArgs - 1));
	}
}
