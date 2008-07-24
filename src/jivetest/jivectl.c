#include <stdio.h>
#include <fcntl.h>

const char* dev = "/dev/misc/jive_mgmt";

int main(int argv, char **args) {
	int fd;

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "cannot open device %s\n", dev);
		exit(-1);
	}
		
	int c = strtol(args[1], NULL, 0);
	int v = -1;
	if (argv > 2) {
		v = strtol(args[2], NULL, 0);
	}

	printf("%s : %d %d\n", args[1], c, v);

	ioctl(fd, c, &v);

	printf("= 0x%x (%d)\n", v, v);

	close(fd);
}
