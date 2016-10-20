#include "LinkLayer.h"

void handleAlarm() {
	timeExceeded = 1; //change variable name
}

int sendMessage(int fd, char* message) {
	//Debug: Prints message to send
	printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", message[0], message[1], message[2], message[3], message[4]);
	
	//Sends the message to the port and return the number of bytes read
	return write(fd, message, SUPERVISIONPACKAGE * sizeof(char));
}

int receiveMessage(int fd, char* message) { //Esta função só dá erro se não conseguir ler 1 byte(mudar)
	char buf;

	while (FALSE==STOP && timeExceeded == 0) {       /* loop for input */
	
		printf("Waiting for message...\n");
		
		//Reads one byte
		res = read(fd, &buf, 1);
		
		//State Machine
		if(1 == res) {
			
			if(buf == FLAG){
				if(s == bcc) s = stop;
				else s = flag; 
			}
			else if(s == flag && buf == A){
				s = a;
			}
			else if(s == a && buf == C_UA){
				s = c;
			}
			else if(s == c && buf == (A^C_UA)){
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

			if(n >= 0) message[n] = buf;

			if (s == stop) STOP = TRUE;
			
		} else {
			printf("Unable to read message.\n");
			return -1;
		}
	}
	
	printf("Received 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", message[0], message[1], message[2], message[3], message[4]);
	return 0;
}

int initializeLinkLayer(int fd, char * port, int baudrate, int timeout, int triesMAX){
	
	//Allocates memory for the linkLayer structure
	linkLayer = (struct linkLayer*)malloc(sizeof(struct linkLayer));
	
	//Initializes link layer structure
	strcpy(linkLayer->port, port);
	linkLayer->baudRate = baudrate;
	linkLayer->timeout = timeout;
	linkLayer->triesMAX = triesMAX;
	linkLayer->sequenceNumber = 0;
    
    //Set up alarm routine
	(void) signal(SIGALRM, handleAlarm);
	
	if(termiosSettings(fd) < 0){ //Implementar
		return -1;
	}
	return 0;
}


int llopen(int fd, int status) {
	int numTries = 0, isConnected = FALSE;

	if(TRANSMITTER == status) {
		while(!isConnected) {
			if(numTries > TRIESMAX) {
				printf("Unable to establising connection.\n");			
				return -1;
			}
			
			//Builds the set frame
			char setPackage = malloc(SUPERVISIONPACKAGE * sizeof(char));
			setPackage[0] = FLAG;
			setPackage[1] = A_TR;
			setPackage[2] = C_SET;
			setPackage[3] = setPackage[1] ^ setPackage[2];
			setPackage[5] = FLAG;
			
			//Sends the set frame
			sendMessage(fd, setPackage);
			free(setPackage);
		
			numTries++;
			
			//Allocates memory to receive the message
			char* receivedMessage = malloc(SUPERVISIONPACKAGE * sizeof(char));
			alarm(1); //linklayer->timeout;
			
			//If a message was read
			if(receiveMessage(fd, receivedMessage) != -1) {
				
				//If the message is UA, the connection between the two computers is online
				if(receivedMessage[1] == A_TR && receivedMessage[2] == C_UA) {
					alarm(0);
					printf("Connection established.\n");
					isConnected = 1;
				}		
			}	
		}
	} else {
		while(!isConnected) {
			
			//Allocates memory to receive the set frame
			char* setPackage = malloc(SUPERVISIONPACKAGE * sizeof(char));
			
			//Reads the message from the port
			receiveMessage(fd, setPackage);

			//If the message is SET
			if(setPackage[1] == A_TR && setPackage[2] == C_SET) {

				//Builds the frame UA to send
				char* uaPackage = malloc(SUPERVISIONPACKAGE * sizeof(char));

				uaPackage[0] = FLAG;
				uaPackage[1] = A_TR;
				uaPackage[2] = C_UA;
				uaPackage[3] = uaPackage[1] ^ uaPackage[2];
				uaPackage[4] = FLAG;

				//Sends the UA frame
				sendMessage(fd, uaPackage);
				free(uaPackage);
				
				connected = 1;
				printf("Connection established.\n");
      		}
		}
	}
	return fd; 
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

unsigned int dataStuffing(char* buffer, unsigned int frameSize) {
	unsigned int newframeSize = frameSize;

	//i = 1 Starts at 1 because the initial flag must remain intact 
	//i < frameSize - 1 because the last flag must remain intact
	int i;
	for (i = 1; i < frameSize - 1; i++)
	
		//FLAG = 0x7e ESCAPE = 0x7d
		if (buffer[i] == FLAG || buffer[i] == ESCAPE)
			newframeSize++;
	
	//Reallocates memory to the buff, adding more space in the end		
	buffer = (char*) realloc(buf, newframeSize);
	
	for (i = 1; i < frameSize - 1; i++) {
	
		if (buffer[i] == FLAG || buffer[i] == ESCAPE) {
		
			//Moves everything from position i in the buffer one position ahead
			memmove(buffer + i + 1, buffer + i, frameSize - i);
			frameSize++;
			
			//Set the i position, where last was the FLAG or ESCAPE and replaces it with the ESCAPE flag
			buffer[i] = ESCAPE;
			
			//Applies exclusive or to the flag that needs to be substituted
			buffer[i + 1] ^= 0x20;
		}
	}
	
	//Returns the frase after stuffing mechanism
	return newframeSize;
}

unsigned int dataDestuffing(char* buffer, unsigned int frameSize){

	int i;
	for (i = 1; i < frameSize - 1; i++) {
		//ESCAPE = 0x7d
		if (buf[i] == ESCAPE) {
		
			//Moves the array one position to the left, eliminating the ESCAPE flag
			memmove(buf + i, buf + i + 1, frameSize - i - 1);
			frameSize--;
			
			//Applies exclusive or to the flag to restore the previous byte
			buf[i] ^= 0x20;
			
		}
	}
	
	//Reallocates memory deleting the last position that are not part of the original frame
	buf = (char*) realloc(buf, frameSize);

	//Returns the size of the original frame
	return frameSize;
	
}

