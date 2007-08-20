#include <stdio.h>
#include <fcntl.h>

const char* dev = "/dev/misc/irtx";

int main(int argv, char **args) {
	int fd;
	int DrvCount;
	int Tx[100];

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "cannot open device %s\n", dev);
		exit(-1);
	}

	DrvCount=66;
	/* Reder */             Tx[0]=4500;     Tx[1]=4500;
	/* Bin Data             0000 0111 0000 0111 0001 0000 1110 1111 => 1110 0000 1110 0000 0000 1000 1111 0111 */
	/* Cus1 1110 */         Tx[2]=560;      Tx[3]=1690;     Tx[4]=560;      Tx[5]=1690;     Tx[6]=560;      Tx[7]=1690;     Tx[8]=560;
	Tx[9]=565;
	/* Cus2 0000 */ Tx[10]=560;     Tx[11]=565;     Tx[12]=560;     Tx[13]=565;     Tx[14]=560;     Tx[15]=565;     Tx[16]=560;     Tx[17]=565;
	/* Cus3 1110 */         Tx[18]=560;     Tx[19]=1690;    Tx[20]=560;     Tx[21]=1690;    Tx[22]=560;     Tx[23]=1690;    Tx[24]=560;
	Tx[25]=565;
	/* Cus4 0000 */ Tx[26]=560;     Tx[27]=565;     Tx[28]=560;     Tx[29]=565;     Tx[30]=560;     Tx[31]=565;     Tx[32]=560;     Tx[33]=565;
	
	/* data1 0000 */        Tx[34]=560;     Tx[35]=565;     Tx[36]=560;     Tx[37]=565;     Tx[38]=560;     Tx[39]=565;     Tx[40]=560;
	Tx[41]=565;
	/* data2 1000 */        Tx[42]=560;     Tx[43]=1690;    Tx[44]=560;     Tx[45]=565;     Tx[46]=560;     Tx[47]=565;     Tx[48]=560;
	Tx[49]=565;
	/* data3 1111 */        Tx[50]=560;     Tx[51]=1690;    Tx[52]=560;     Tx[53]=1690;    Tx[54]=560;     Tx[55]=1690;    Tx[56]=560;
	Tx[57]=1690;
	/* data4 0111 */        Tx[58]=560;     Tx[59]=565;     Tx[60]=560;     Tx[61]=1690;    Tx[62]=560;     Tx[63]=1690;    Tx[64]=560;
	Tx[65]=1690;

	write(fd, Tx, DrvCount);
	printf("IR Tx OK\n");
}
