/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07

enum STATE{
	start,
	flag,
	a,
	c,
	bcc,
	stop
} state;

volatile int STOP=FALSE;
int f, fd, res;
char buf[5];

void sendMessage() {
	
	buf[0] = FLAG;
	buf[1] = A;
	buf[2] = C_SET;
	buf[3] = (buf[1]^buf[2]);
	buf[4] = FLAG;


    printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
    res = write(fd,buf,5);   
    printf("%d bytes written\n", res);
	f = 1;
}

int main(int argc, char** argv)
{
    int counter;
    struct termios oldtio,newtio;
    int i, sum = 0, speed = 0;
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

    newtio.c_cc[VTIME]    = 10;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



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

	//Set up alarm routine
	(void) signal(SIGALRM, sendMessage);


/*
    for (i = 0; i < 255; i++) {
      buf[i] = 'a';
    }
*/    
    /*testing*/
 /*   buf[25] = '\n';
*/

	sendMessage();
 
  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */

	//buff a ser lido e guardado em buf
	char buff[255];
	int n = -1;
	counter = 0;

	while(counter < 3 && s != stop) {
		printf("Counter %d\n", counter);
		f = 0;
		alarm(3);
		if(f == 0) {	
			while (STOP==FALSE && f == 0) {       /* loop for input */
				printf("Waiting for message...\n");
				res = read(fd, buff, 1);
				if(res == 1) {
					printf("Byte read: %x\n", buff[0]);
					if(buff[0] == FLAG){
						printf("Received flag\n");
						if(s == bcc) s = stop;
						else s = flag; 
					}
					else if(s == flag && buff[0] == A){
						printf("Received A\n");
						if(s == flag) s = a;
						else s = start;
					}
					else if(s == a && buff[0] == C_UA){
						printf("Received C\n");
						if(s == a) s = c;
						else s = start;
					}
					else if(buff[0] == (A^C_UA)){
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

					if( n >= 0) buf[n] = buff[0];

					if (s == stop) STOP=TRUE;
				}
			}

			printf("Received 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
		}
		counter++;
	}

	if(counter == 3) printf("Time out.\n");
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
