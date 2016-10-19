
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
#define A_TR 0x03
#define A_RT 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0B
#define TRIESMAX 5
#define TRANSMITTER 0
#define RECEIVER  1
#define CONTROLPACKAGESIZE 5

void sendMessage(int fd, char* message) {

    printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
    res = write(fd,message,CONTROLPACKAGESIZE * sizeof(char));   
    printf("%d bytes written\n", res);
	f = 1;
}

int llclose(int fd, int mode){
	printf("closing\n");
	int disc = 0, counter = 0;

	//TRSNAMITTER
	if(TRANSMITTER == mode){
		while(0 == disc){
			while(counter < TRIESMAX){
				char* discPackage = malloc(CONTROLPACKAGESIZE * sizeof(char));				

				discPackage[0] = FLAG;
				discPackage[1] = A_TR;
				discPackage[2] = C_DISC;
				discPackage[3] = (buf[1]^buf[2]);
				discPackage[4] = FLAG;

				sendMessage(fd, discPackage);
				free(discPackage);

				counter++;

			}

			char* response = malloc(CONTROLPACKAGESIZE * sizeof(char));

			receiveMessage(fd, response);
			if(response[1] == A_RT && response[2] == C_DISC){
				disconnected = 1;
		
				char* UA = malloc(CONTROLPACKAGESIZE * sizeof(char));				

				UA[0] = FLAG;
				UA[1] = A_TR;
				UA[2] = C_UA;
				UA[3] = (buf[1]^buf[2]);
				UA[4] = FLAG;

				sendMessage(fd, UA);
				free(UA);

				printf("UA sent: disconected.\n");
			}

	else if(RECEIVER == mode){
		while(0 == disc){
				char* discPackage = malloc(CONTROLPACKAGESIZE * sizeof(char));				

				receiveMessage(fd, discPackage);
				if(discPackage[1] == A_TR && discPackage[2] == C_DISC);
				while(counter < TRIESMAX){
					char* response = malloc(CONTROLPACKAGESIZE * sizeof(char));				

					response[0] = FLAG;
					response[1] = A_RT;
					response[2] = C_DISC;
					response[3] = (buf[1]^buf[2]);
					response[4] = FLAG;

					sendMessage(fd, response);
					free(response);

					counter++;

				}
				free(discPackage);

			char* UA = malloc(CONTROLPACKAGESIZE * sizeof(char));

			receiveMessage(fd, UA);

			if(UA[1] == A_TR && UA[2] == C_DISC){
				disconnected = 1;

				free(UA);

				printf("UA received: disconected.\n");
			}		

	}


	return 1;

}
