#include "LinkLayer.h"

void handleAlarm() {
	timeExceeded = 1;
	printf("Alarm called.\n");
 }

int sendMessage(int fd, char* message) {
	//Debug: Prints message to send
	printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", message[0], message[1], message[2], message[3], message[4]);
	
	//Sends the message to the port and return the number of bytes read
	return write(fd, message, SUPERVISIONPACKAGE * sizeof(char));
}

int receiveMessage(int fd, char* message) {
	char buf;
	int res;
	int STOP = FALSE;
	int n = -1;
	enum STATE s = start;

	while (FALSE == STOP && !timeExceeded) {       /* loop for input */

		printf("Waiting for message...\n");
		
		//Reads one byte
		res = read(fd, &buf, 1);
		
		//State Machine
		if(1 == res) {
			
			if(buf == FLAG) {
				if(s == bcc) s = stop;
				else s = flag; 
			}
			else if(s == flag && (buf == A_TR || buf == A_RT)) {
				s = a;
			}
			else if(s == a && (buf == C_SET || buf == C_UA || (buf & 0x0F) == C_RR || (buf & 0x0F) == C_REJ || buf == C_DISC)) {
				s = c;
			}
			else if(s == c && buf == (message[1]^message[2])) {
				s = bcc;
			}
			else{
				s = start;
				printf("Received something else.\n");
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
	printf("Received 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", message[0], message[1], message[2], message[3], message[4]);	
	}
	
	return 0;
}

int initializeLinkLayer(int fd, char * port, int triesMAX, int timeout) {
	
	//Allocates memory for the linkLayer structure
	llink = (struct linkLayer*)malloc(sizeof(struct linkLayer));
	
	//Initializes link layer structure
	strcpy(llink->port, port);
	llink->timeout = timeout;
	llink->triesMAX = triesMAX;
	llink->sequenceNumber = 0;
    
    //Set up alarm routine
	(void) signal(SIGALRM, handleAlarm);
	
	//Changes the termios settings
	if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* Set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 10;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  	/* 
    	VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    	leitura do(s) próximo(s) caracter(es)
  	*/

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

    printf("New termios structure set\n");
	return 0;
}


int llopen(int fd, int status) {
	int counter = 0, isConnected = FALSE;

	if(TRANSMITTER == status) {
		while(isConnected == FALSE) {
			
			//Only retries to send the frame if the time was Exceeded
			if(counter == 0 || timeExceeded) {
				timeExceeded = 0;
				
				//If the number of tries was exceeded
				if(counter >= llink->triesMAX) {
					printf("Unable to establising connection.\n");			
					return -1;
				}
			
				//Builds the set frame
				char* setPackage = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));
				setPackage[0] = FLAG;
				setPackage[1] = A_TR;
				setPackage[2] = C_SET;
				setPackage[3] = setPackage[1] ^ setPackage[2];
				setPackage[5] = FLAG;
			
				//Sends the set frame
				sendMessage(fd, setPackage);
				free(setPackage);
		
				counter++;
				alarm(llink->timeout);
			}
			
			//Allocates memory to receive the message
			char* receivedMessage = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));
		
			//If a message was read
			if(receiveMessage(fd, receivedMessage) != -1) {
				//If the message is UA, the connection between the two computers is online
				if(receivedMessage[1] == A_TR && receivedMessage[2] == C_UA) {
					alarm(0);
					printf("Connection established.\n");
					isConnected = TRUE;
				}		
			}
			free(receivedMessage);
		}
		timeExceeded = 0;
	} else {
		while(isConnected == FALSE) {
			
			//Allocates memory to receive the set frame
			char* setPackage = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));
			
			//Reads the message from the port
			receiveMessage(fd, setPackage);

			//If the message is SET
			if(setPackage[1] == A_TR && setPackage[2] == C_SET) {

				//Builds the frame UA to send
				char* uaPackage = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));

				uaPackage[0] = FLAG;
				uaPackage[1] = A_TR;
				uaPackage[2] = C_UA;
				uaPackage[3] = uaPackage[1] ^ uaPackage[2];
				uaPackage[4] = FLAG;

				//Sends the UA frame
				sendMessage(fd, uaPackage);
				free(uaPackage);
				
				isConnected = TRUE;
				printf("Connection established.\n");
      		}
			free(setPackage);
		}
	}
	return fd; 
}

int llwrite(int fd, char* buffer, unsigned int length) {

	//6 for the F, A, C, BCC1, BCC2 and F
	char* frame = (char*)malloc(length + 6 * sizeof(char));
	
	//Creates the BBC2 flag depeding on the data
	char BCC2 = findBCC2(buffer, length);
	
	//Builds the frame to send
	frame[0] = FLAG;
	frame[1] = A_TR;
	frame[2] = (llink->sequenceNumber << 6);
	frame[3] = frame[1]^frame[2];
	memcpy(&frame[4], buffer, length);
	frame[4 + length] = BCC2;
	frame[5 + length] = FLAG;

	//Data stuffing
	int newSize = dataStuffing(frame, length + 6 * sizeof(char));

	int STOP = FALSE;
	int counter = 0;

	while(!STOP){
		
		//It only retries to send the code if the time was exceeded
		if(counter == 0 || timeExceeded){
			timeExceeded = 0;
			//If the number of tries was exceeded
			if (counter >= llink->triesMAX) {
				printf("Unable to send data package.\n");
				return -1;
			}
			
			printf("%x, %x, %x, %x.\n", frame[0], frame[1], frame[2], frame[3]);
			//Sends Information frame
			if(write(fd, frame, newSize) == -1){
				printf("Unable to write data package\n");
				return -1;
			}
			counter++;
			alarm(llink->timeout);
		}

		char response[SUPERVISIONPACKAGE];

		receiveMessage(fd, response);
		
		if(response[0] == FLAG && response[1] == A_TR) {
			
			//If the Information frame was rejected
			if((response[2] & 0x0F) == C_REJ) {
				printf("REJ\n");

				//If the sequence number is the same as the one sent, retry to send the Information frame
				if((response[2] >> 7) == llink->sequenceNumber) {
				  alarm(0);
				  counter = 0;
				}

				//If the sequence number is not the same, the response was not correctly built
				else {
				  printf("REJ response was not correctly built.\n");
				  return -1;
				}
			}

			else if ((response[2] & 0x0F) == C_RR) {
				printf("RR\n");
				
				//If the sequence number is not the same as the one sent, the frame was accepted 
				if((response[2] >> 7) != llink->sequenceNumber) {
					alarm(0);
					llink->sequenceNumber = (response[2] >> 7);
					STOP = TRUE;
				}

				//The header of the package was accepted, but the data field needs to be resent
				else {
					
					//Sets timeExceeded to 1 to be able to resend the Information frame
					handleAlarm();
				}
			}
		}
	}
	
	free(frame);
	//Reset timeExceeded flag
	timeExceeded = 0;

	return newSize;
}

int llread(int fd, char* buffer) {
	int STOP = FALSE;

	//0 - Before receiving the first FLAG flag | 1 - After receiving the first FLAG flag and before receiving the last FLAG flag || 2 - After receiving the last
	// FLAG flag
	int state = 0;
	int size = 0;

	char* buff = (char*)malloc(3000);

	while(!STOP){

		char c;
		if(state < 2) {
			int res = read(fd, &c, 1);
			if(res == -1){
				printf("Unable to read Information package.\n");
				return -1;
			}
		}

		switch(state){
			case 0:
				if(c == FLAG) {
					buff[size] = c;
					size++;
					state = 1;
				}
				break;
			case 1:
				if(c == FLAG && size != 1) {
					buff[size] = c;
					size++;
					state = 2;
				}

				else if(c == FLAG && size == 1){;}

				else {
					buff[size] = c;
					size++;
				}
				break;
			default:
				STOP = TRUE;
				break;
		}
	}

	printf("Received 0x%x 0x%x 0x%x 0x%x.\n", buff[0], buff[1], buff[2], buff[3]);
	int process = FALSE;
	int newSize = dataDestuffing(buff, size);

	if(buff[0] != FLAG || buff[1] != A_TR || buff[3] != (buff[1] ^ buff[2])){
		printf("Received frame header was not corretly built\n");
		free(buff); 
		return -1;
	}

	//6 - F, A, C, BCC1, BCC2 and F
	int dataPackageSize = newSize - 6 * sizeof(char);

	//Creates BCC2 depending on the data field
	char BCC2 = findBCC2(&buff[4], dataPackageSize);
	
	//Only the last bit is considered
	unsigned int sequenceNumber = (buff[2] >> 6) & 1;

	char response[SUPERVISIONPACKAGE * sizeof(char)];
	response[0] = FLAG;
	response[1] = A_TR;
	response[4] = FLAG;

	//If the right frame was received (With the expected sequence number)
	if(llink->sequenceNumber == sequenceNumber) {

		//If the data BBC does not match, the frame was corrupted
		if(BCC2 != buff[newSize - 2]) {
			printf("Data BCC does not match the data BCC received.\nFrame rejected.\n");
			response[2] = (llink->sequenceNumber << 7) | C_REJ;
		}
		else {
			//Updates the sequenceNumber to the next one
			if(llink->sequenceNumber == 0) {
				llink->sequenceNumber = 1;
			}
		  	else {
		  		llink->sequenceNumber = 0;
		  	}
		  	process = TRUE;
			
			//Sends RR as a response			  	
			response[2] = (llink->sequenceNumber << 7) | C_RR;
	  }
	}
	else{
		//If it's duplicate, send RR response with the same sequence number that was updated when the frame was received the first time
		response[2] = (llink->sequenceNumber << 7) | C_RR;
	}

	//BCC1
	response[3] = response[1]^response[2];
	
	//Sends the response
	sendMessage(fd, response);
	
	//If the Information frame was accepted 
	if(process) {

		//Updates the argument buffer to contain the data
		memcpy(buffer, &buff[4], dataPackageSize);
		free(buff);
		return dataPackageSize;
	}

	free(buff);
	return -1;
}

int llclose(int fd, int mode){
	printf("Closing\n");
	int disc = 0, counter = 0;

	if(TRANSMITTER == mode){
		while(0 == disc) {

			//Only retries to send the frame if the time was exceeded
			if(counter == 0 || timeExceeded) {
				timeExceeded = 0;
				//If the number of tries was exceeded
				if(counter >= llink->triesMAX) {
					printf("Unable to disconnect.\n");			
					return -1;
				}
			
				//Allocates memory for the disconnect frame
				char* discPackage = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));				
				
				//Builds the disconnect frame
				discPackage[0] = FLAG;
				discPackage[1] = A_TR;
				discPackage[2] = C_DISC;
				discPackage[3] = (discPackage[1]^discPackage[2]);
				discPackage[4] = FLAG;

				//Sends the disconnect frame
				if(sendMessage(fd, discPackage) < 0) {
					printf("Unable to send disconnect frame.\n");
					return -1;
				}

				free(discPackage);
				alarm(llink->timeout);
				
				//Updates the number of tries
				counter++;
			}
			
			//Allocates space for the response
			char* response = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));
		
			//Reads the response
			if(receiveMessage(fd, response) < 0) {
				printf("Unable to read disconnect response.\n");
				return -1;				
			}

			//If the received frame is the disconnect frame, it disconnected successfully
			if(response[1] == A_RT && response[2] == C_DISC) {
				disc = 1;
				
				//Allocates space for the response (UA)
				char* UA = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));				

				//Builds the UA frame to respond
				UA[0] = FLAG;
				UA[1] = A_RT;
				UA[2] = C_UA;
				UA[3] = (UA[1]^UA[2]);
				UA[4] = FLAG;

				//Sends the UA frame as a response
				if(sendMessage(fd, UA) < 0) {
					printf("Unable to write UA frame as a response to disconnect.\n");
					return -1;					
				}
				
				free(UA);
				printf("Disconnected.\n");
			}
			free(response);
		}
		alarm(0);
		sleep(3);
	}
	else if(RECEIVER == mode) {
		
		//Allocates memory for the disconnect frame read from the transmitter
		char* discPackage = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));				
		
		//Reads the disconnect frame
		if(receiveMessage(fd, discPackage) < 0){
			printf("Unable to receive disconnect package from transmitter.\n");
			return -1;				
		}				
	
		//If the frame received is the disconnect frame
		if(discPackage[1] == A_TR && discPackage[2] == C_DISC) {

			//While it's not disconnected
			while(0 == disc) {

				//It only resends if the time has exceeded
				if (counter == 0 || timeExceeded) {
					timeExceeded = 0;

					if (counter >= llink->triesMAX) {
						printf("Unable to disconnect.\n");
						return -1;
					}

					//Allocates memory for the disconnect response
					char* response = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));				

					//Builds the disconnect response
					response[0] = FLAG;
					response[1] = A_RT;
					response[2] = C_DISC;
					response[3] = (response[1]^response[2]);
					response[4] = FLAG;

					//Sends the disconnect frame as a response
					if(sendMessage(fd, response) < 0) {
						printf("Unable to send disconnect frame as a response.\n");
						return -1;					
					}

					free(response);
					counter++;
					alarm(llink->timeout);
				}

				//Allocates memory for the UA response from the transmitter
				char* UA = (char*)malloc(SUPERVISIONPACKAGE * sizeof(char));

				//Reads the UA response
				if(receiveMessage(fd, UA) < 0) {
					printf("Unable to read the UA response from the transmitter\n");
					return -1;				
				}

				//If the UA response is accepted
				if(UA[1] == A_RT && UA[2] == C_UA) {
					disc = 1;
					free(UA);
					printf("Disconnected.\n");
				}
			}
		}
		free(discPackage);			
	}

	free(llink);
	return 0;
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
	
	//Reallocates memory for the buffer, adding more space in the end		
	buffer = (char*) realloc(buffer, newframeSize);
	
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
		if (buffer[i] == ESCAPE) {
		
			//Moves the array one position to the left, eliminating the ESCAPE flag
			memmove(buffer + i, buffer + i + 1, frameSize - i - 1);
			frameSize--;
			
			//Applies exclusive or to the flag to restore the previous byte
			buffer[i] ^= 0x20;
			
		}
	}
	
	//Reallocates memory deleting the last position that are not part of the original frame
	buffer = (char*) realloc(buffer, frameSize);

	//Returns the size of the original frame
	return frameSize;
	
}

char findBCC2(char* data, unsigned int size) {
	int i;
	char BCC2 = 0;
	
	//Iterates through the data buffer and apply the exclusive or to all the elements
	for(i = 0; i < size; i++) {
		BCC2 ^= data[i];
	}

	return BCC2;
}

