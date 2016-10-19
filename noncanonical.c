/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03
#define C_UA 0x07
#define C_SET 0x03

enum STATE{
	start,
	flag,
	a,
	c,
	bcc,
	stop
} state;

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
	int fd,c, res;
	struct termios oldtio,newtio;
	char buf[255];
	enum STATE s = start;

	if ( (argc < 2) || 
	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
	printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	exit(1);
	}


	/*
	Open serial port device for reading and writing and not as controlling tty
	because we don't want to get killed if linenoise sends CTRL-C.
	*/


	fd = open(argv[1], O_RDWR | O_NOCTTY );
	if (fd <0) {perror(argv[1]); exit(-1); }

	if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
	perror("tcgetattr");
	exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



	/* 
	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
	leitura do(s) próximo(s) caracter(es)
	*/



	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
	perror("tcsetattr");
	exit(-1);
	}

	printf("New termios structure set\n");

	char buff[255];
	int n = -1;

	while (STOP==FALSE) {       /* loop for input */
		printf("Waiting for message...\n");
		res = read(fd,buf,1);
		printf("Byte read: %x\n", buf[0]);
		if(buf[0] == FLAG){
			printf("Received flag\n ");
			if(s == bcc) s = stop;
			else s = flag; 
		}
		else if(s == flag && buf[0] == A){
			printf("Received A\n");
			if(s == flag) s = a;
			else s = start;
		}
		else if(s == a && buf[0] == C_SET){
			printf("Received C\n");
			if(s == a) s = c;
			else s = start;
		}
		else if(buf[0] == (A^C_SET)){
			printf("Received A^C\n");
			if( s == c) s = bcc;
			else s = start;
		}
		else{
			printf("Received something else \n");
	};

	if (s == flag) n = 0;
	else if( s == a) n = 1;
	else if( s == c) n = 2;
	else if(s == bcc) n = 3;
	else if( s == stop) n = 4;
	else{};

	if( n >= 0) buff[n] = buf[0];

	if (s == stop) STOP=TRUE;
	}

	printf("Received 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buff[0], buff[1], buff[2], buff[3], buff[4]);

	buff[0] = FLAG;
	buff[1] = A;
	buff[2] = C_UA;
	buff[3] = (buff[1]^buff[2]);
	buff[4] = FLAG;

	printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buff[0], buff[1], buff[2], buff[3], buff[4]);

	int d = write(fd, buff, 5);

	sleep(2);

	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	return 0;
}
