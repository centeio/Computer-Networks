
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

				if(sendMessage(fd, discPackage) < 0){
					"Message failed sending \n";
					return -1;
				}
				free(discPackage);

				counter++;

			}

			char* response = malloc(CONTROLPACKAGESIZE * sizeof(char));

			if(receiveMessage(fd, response) < 0){
				"Message failed receiving \n";
				return -1;				
			}
			if(response[1] == A_TR && response[2] == C_DISC){
				disconnected = 1;
		
				char* UA = malloc(CONTROLPACKAGESIZE * sizeof(char));				

				UA[0] = FLAG;
				UA[1] = A_TR;
				UA[2] = C_UA;
				UA[3] = (buf[1]^buf[2]);
				UA[4] = FLAG;

				if(sendMessage(fd, UA) < 0){
					"Message failed sending \n";
					return -1;					
				}
					
				free(UA);

				printf("UA sent: disconected.\n");
			}
		}
	}

	else if(RECEIVER == mode){
		while(0 == disc){
			char* discPackage = malloc(CONTROLPACKAGESIZE * sizeof(char));				

			if(receiveMessage(fd, discPackage) < 0){
				"Message failed receiving \n";
				return -1;				
			}
				
			if(discPackage[1] == A_TR && discPackage[2] == C_DISC);
			while(counter < TRIESMAX){
				char* response = malloc(CONTROLPACKAGESIZE * sizeof(char));				

				response[0] = FLAG;
				response[1] = A_TR;
				response[2] = C_DISC;
				response[3] = (buf[1]^buf[2]);
				response[4] = FLAG;

				sendMessage(fd, response){
					"Message failed sending \n";
					return -1;					
				}
				free(response);

				counter++;

			}
			free(discPackage);

			char* UA = malloc(CONTROLPACKAGESIZE * sizeof(char));

			if(receiveMessage(fd, UA) < 0){
				"Message failed sending \n";
				return -1;				
			}

			if(UA[1] == A_TR && UA[2] == C_UA){
				disconnected = 1;

				free(UA);

				printf("UA received: disconected.\n");
			}
		}			
	}
	
	return 1;
}


int receive(){
	if(llopen(application->fd, application->mode) <= 0){
		printf("could not open file\n");
		return -1;
	}
		FILE* file; 
		char fileName[30] = "";
		char fileSize[10] = "";
 		char package[3000];
		
	   	memset(package, 0, 3000);
	
		int received = 0;
		while(0 == received){
			char* receiving = malloc(sizeof(char));
			int dataSize = llread(application->fd, package);
				if(dataSize > 0){
					if(3 == package[0])
						done = 1;
					int i = 1;
					while(i < size){
						int j = 0;
						if(0 == package[i]){
							printf("File length\n");
							for(; j < package[i+1]; j++){ // i+1 = V length
								fileSize[j] = package[j+i+2];
							}
						}
						else if(1 == package[i]){
							printf("File name\n");
							for(; j < package[i+1]; j++){ 
								fileName[j] = package[i+j+2];
							}
						}
						
						i+= j + 2;					
					}
				}
			if(fopen(fileName,"wb") < 0){
				printf("Error opening new file");
				return -1;
			}
			
		
			if(1 == package[0] && file != NULL){ //data

				printf("Data package: %d \n", package[1]);
				int psize = (unsigned char)package[2] << 8 | (unsigned char)package[3]; //(K = 256*L2+L1)
				fwrite(&package[4], sizeof(char), psize, file);
			}
				
			memset(package, 0, 3000);
		
		}
	if (fclose(file) < 0){
		printf("File %s was not closed.\n", fileName);
		return -1;
	}	
	if (llclose(application->fd, application->mode) < 0) {
		printf("File transfered successfully. \n");
		return 1;
	}

	printf("Serial port was not closed!\n");

	return -1;
	
	
}