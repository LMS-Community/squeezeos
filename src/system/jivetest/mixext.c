#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <time.h>
#include <errno.h>

int mixerfd;

int main (int argc, char *argv[])
{
	int i;
	int mask = 0;
	char *names[] = SOUND_DEVICE_NAMES;

	if ((mixerfd = open("/dev/sound/mixer", O_RDWR, 0)) == -1) {
		perror("/dev/sound/mixer");
		exit(-1);
	}

	if (ioctl(mixerfd, SOUND_MIXER_READ_DEVMASK, &mask) == -1) {
		perror("SOUND_MIXER_READ_DEVMASK");
		exit(-1);
	}

	printf("Available mixers %x:\n", mask);
	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		if (mask & (1 << i)) {
			printf("\t%i: %s\n", i, names[i]);
		}
	}


	if (argc >= 2) {
		int channel, volume;

		channel = atoi(argv[1]);
		volume = atoi(argv[2]);

		printf("channel %d volume %d\n", channel, volume);
		if (ioctl(mixerfd, MIXER_WRITE(channel), &volume) == -1) {
			perror("SOUND_MIXER_WRITE");
			exit(-1);
		}
	}

	exit (0);
}
