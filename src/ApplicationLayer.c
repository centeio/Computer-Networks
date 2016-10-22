#include "ApplicationLayer.h"

int writeControlPackage(int control, char* fileName, char* fileSize) {

    // C T1 L1 V1(fileSize) T2 L2 V2(fileName) 
    unsigned int controlPackageSize = 5 + strlen(fileSize) + strlen(fileName);

	//Initializes control package variable
    char controlPackage[controlPackageSize];

	//for loop counter
    int i;
    
    //C 2: Start; 3: End
    controlPackage[0] = control;
    
    //T1 - SIZE(0)
    controlPackage[1] = 0; 

    //L1 - Size of V1 field
    controlPackage[2] = strlen(fileSize);

    //Copies the file size to the V1 field
    for (i = 0; i < strlen(fileSize); i++)
        controlPackage[i + 3] = fileSize[i];

    //T2 NAME(1)
    controlPackage[3 + strlen(fileSize)] = 1;
    
    //L2 - Size of V2 field
    controlPackage[4 + strlen(fileSize)] = strlen(fileName);

    //Copies the file name to the V2 field
    for (i = 0; i < strlen(fileName); i++)
        controlPackage[5 + strlen(fileSize) + i] = fileName[i];

    //llwrite - LinkLayer function 
    return llwrite(application->fd, controlPackage, controlPackageSize);
}

int initializeApplicationLayer(char* port, unsigned int messageSize, int retries, int timeout, char* fileName, int status) {
    
    //Allocates memory for application struct
    application = (struct applicationLayer*) malloc(sizeof(struct applicationLayer));
    
    //Opens the port and stores the file descriptor in the application structure
    application->fd = open(port, O_RDWR | O_NOCTTY );
    
    //Stores the messageSize in the application structure
    application->messageSize = messageSize;

	//Checks if the port was successfully opened
    if (application->fd < 0) {
        printf("Unable to opening port.\n");
        return -1;
    }

	//Stores the mode of the computer: 0 - Receiver, 1 - Transmitter
    application->status = status;
    
    //Stores the file name in the application structure
    application->fileName = fileName;

	//Initializes link layer
    if(initializeLinkLayer(application->fd, port, retries, timeout) < 0) {
        printf("Unable to initialize link layer.\n");
        return -1;
    }
	
	//Checks if the computter is in transmitter mode
    if(application->status == TRANSMITTER)
        send();
    //Checks if the computer is in receiver mode
    else if(application->status == RECEIVER)
        receive();
    //Else nothing happens, simply closes the port
	
	//Restores the termios
	if (tcsetattr(application->fd, TCSANOW, &oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }

	//Closes the port
    close(application->fd);

	free(application);
    return 0;
}

int send(){

	printf("Start writing\n");
	//Opens the file as a binary file for reading
	FILE* file = fopen(application->fileName, "rb");
	if(!file) {
		//Debug: Error message when unable to open file.
		printf("Unable to open the specified file.\n");
		return -1;
	}

	//Places the file pointer at the end of the file
	fseek(file, 0, SEEK_END);
	
	//For binary streams, this is the number of bytes from the beginning of the file.
	unsigned int fileSize = ftell(file);

	//Places the file pointer at the beginning of the file
	fseek(file, 0, SEEK_SET);
	
	//Declaration of the array that will store the file size
	char sizeString[10];

	//Transforms the size of the file into a string
	sprintf(sizeString, "%u", fileSize);
	
	//Degub: Prints the file size
	printf("File size: %u Bytes.\n", fileSize);

	//Tries to establish connection by sending the C_SET command
	if(llopen(application->fd, application->status) < 0)
		return -1;
		
	//Sends the start package to the receiver, 2: START
	if(writeControlPackage(2, application->fileName, sizeString) < 0) {
		//Debug: Error message when unable to send start control package
		printf("Unable to send start control package.\n");
		return -1;
	}
	
	//Debug: Message when sending process starts
	printf("Starting sending...\n");
	
	//Variable to store the number of bytes read per iteration
	unsigned int bytesRead;
	
	//C N L2 L1 P(messageSize)
	unsigned char* data = (unsigned char*)malloc((application->messageSize + 4) * sizeof(unsigned char));

	//Initializes the data package sequence number
	unsigned int packageSequenceNumber = 0;
	
	//Iteration that reads while there is something to read
	while((bytesRead = fread(data + 4, sizeof(char), application->messageSize, file)) > 0) {

		//C - Data which is represented as the number 1
		data[0] = 1;

		//N - Package sequence number
		data[1] = (packageSequenceNumber % 255);

		printf("Data package %d.\n", data[1]);
		
		//L2 - bytes / 256
		data[2] = bytesRead >> 8;
		
		//L1 - bytes % 256
		data[3] = bytesRead & 0xFF;
		
		//Calls the LinkLayer function to send the data
		if(llwrite(application->fd, data, bytesRead + 4) == -1) {
			//Debug: Error message when unable to send data package
			printf("Unable to write package %d.\n", packageSequenceNumber);
			return -1;
		}
		
		//Updates the package sequence number
		packageSequenceNumber++;
	}

	free(data);
	
	//Closes the file
	if (fclose(file) != 0) {
	
		//Debug: Error message when unable to close file
		printf("Unable to close file.\n");
		return -1;
	}
	
	//Sends the end package to the receiver, 3 - END
	if(writeControlPackage(3, application->fileName, sizeString) < 0) {
		
		//Debug: Error message when unable to send end package
		printf("Unable to send end package.\n");
		return -1;
	}
		
	//Calls close function from Linklayer
	if(llclose(application->fd, application->status) < 0)
		return -1;
	
	printf("\n");
	return 0;
}

int receive(){

	//Tries to establish connection by reading the C_UA response
	if(llopen(application->fd, application->status) <= 0)
		return -1;

	//Variable initialization
	FILE* file; 
	char fileName[30] = "";
	char fileSize[10] = "";
	char package[3000];

   	memset(package, 0, 3000);

	int received = 0;

	while(0 == received){
		
		int dataSize = llread(application->fd, package);
		
		//If llread didn't fail reading the data package	
		if(dataSize > 0){
				
			//If the package received is the end package
			if(3 == package[0])
				received = 1;
			
			//if the package received is the start package
			if(2 == package[0]) {

				//i starts at 1, because the first position was already tested
				int i = 1;
				while(i < dataSize) {

					int j = 0;
					
					//If the next section of the data package is representing the file size
					if(0 == package[i]) {

						printf("Reading file size.\n");
						for(; j < package[i+1]; j++) { // i+1 = V length
							fileSize[j] = package[j+i+2];
						}
					}
					//If the next section of the data package is representing the file name
					else if(1 == package[i]) {

						printf("Reading file name.\n");
						for(; j < package[i+1]; j++) { 
							fileName[j] = package[i+j+2];
						}
					}
					//i moves to the next T position, T2 i(C) += j(V) + 2(T1 + L1)
					i += j + 2;					
				}

				//Creates a new file with the name sent by the transmitter
				if(fopen(fileName, "wb") < 0){
					printf("Unable to open the file.\n");
					return -1;
				}
			}

			//If the package received is a data package
			if(1 == package[0] && file != NULL) { 
				//Prints the data package sequence number
				printf("Data package: %d.\n", package[1]);
				int psize = (unsigned char)package[2] << 8 | (unsigned char)package[3]; //(K = 256*L2+L1)
				fwrite(&package[4], sizeof(char), psize, file);
			}
		}

		//Reset the package information to zero to prevent "garbage" data
		memset(package, 0, 3000);
	}

	//Closes the file that was transfered
	if (fclose(file) < 0){
		printf("File %s was not closed.\n", fileName);
		return -1;
	}	

	//Closes the port
	if (llclose(application->fd, application->status) < 0) {
		printf("Serial port was not closed. \n");
		return -1;
	}

	printf("File transfered successfully.\n");
	return 0;
}
