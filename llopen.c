#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>

#define TRIESMAX 5
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define A_TR 0x03
#define A_RT 0x01
#define C_SET 0x03
#define C_UA 0x07
#define RECEIVER 0
#define TRANSMITTER 1
#define CONTROLPACKAGESIZE 5

void handleAlarm() {
	timeExceeded = 1; //change variable name
}

int sendMessage(int fd, char* message) {
	printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", message[0], message[1], message[2], message[3], message[4]);
	return write(fd, message, CONTROLPACKAGESIZE * sizeof(char));
}

int recieveMessage(int fd, char* message) { //Esta função só dá erro se não conseguir ler 1 byte(mudar)
	char buf;

	while (STOP==FALSE) {       /* loop for input */
		printf("Waiting for message...\n");
		res = read(fd, &buf, 1);
		
		if(res == 1) {

			printf("Byte read: %x\n", buf);
			if(buf == FLAG){
				printf("Received flag\n");
				if(s == bcc) s = stop;
				else s = flag; 
			}
			else if(s == flag && buf == A){
				printf("Received A\n");
				s = a;
			}
			else if(s == a && buf == C_UA){
				printf("Received C\n");
				s = c;
			}
			else if(s == c && buf == (A^C_UA)){
				printf("Received A^C\n");
				s = bcc;
			}
			else{
				s = start;
				printf("Received something else \n");
			}

			if (s == flag) n = 0;
			else if(s == a) n = 1;
			else if(s == c) n = 2;
			else if(s == bcc) n = 3;
			else if(s == stop) n = 4;
			else{};

			if( n >= 0) message[n] = buf;

			if (s == stop) STOP=TRUE;
		} else {
			printf("Error reading the message.\n");
			return -1;
		}
	}
	printf("Received 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", message[0], message[1], message[2], message[3], message[4]);
	return 0;
}


int llopen(int fd, int connectionMode) {
	int numTries = 0, isConnected = FALSE;

	//Set up alarm routine
	(void) signal(SIGALRM, handleAlarm);

	//Receiver
	if(TRANSMITTER == connectionMode) {
		while(!isConnected) {
			if(numTries > triesMAX) {
				printf("Error establising connection.\n");			
				return -1;
			}

			char setPackage = malloc(CONTROLPACKAGESIZE * sizeof(char));
			setPackage[0] = FLAG;
			setPackage[1] = A_TR;
			setPackage[2] = C_SET;
			setPackage[3] = setPackage[1] ^ setPackage[2];
			setPackage[5] = FLAG;
			sendMessage(fd, setPackage);
			free(setPackage);
		
			numTries++;
			char* receivedMessage = malloc(CONTROLPACKAGESIZE * sizeof(char));
			alarm(1);
			
			if(recieveMessage(fd, receivedMessage) != -1) {
				if(receivedMessage[1] == A_TR && receivedMessage[2] == C_UA) {
					alarm(0);
					printf("Connection established.\n");
					isConnected = 1;
				}		
			}	
		}
	} else {
		while(!isConnected) {
			char* setPackage = malloc(CONTROLPACKAGESIZE * sizeof(char));
			recieveMessage(fd, setPackage);

			if(setPackage[1] == A_TR && setPackage[2] == C_SET) {
				char* uaPackage = malloc(CONTROLPACKAGESIZE * sizeof(char));

			      	uaPackage[0] = FLAG;
			      	uaPackage[1] = A_TR;
			      	uaPackage[2] = C_UA;
			      	uaPackage[3] = uaPackage[1] ^ uaPackage[2];
			      	uaPackage[4] = FLAG;

				sendMessage(fd, uaPackage);
				free(uaPackage);
				connected = 1;
				printf("Connection established.\n");
      			}
		}
	}
	return fd; //(No código do Trindade) Verificar se é preciso no nosso.
}
