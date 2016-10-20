#include "ApplicationLayer.h"

int writeControlPackage(int control, char* fileName, char * fileSize) {

    // C T1 L1 V1(fileSize) T2 L2 V2(fileName) 
    unsigned int controlPackageSize = 5 + strlen(fileName) + strlen(fileSize);

	//Initializes control package variable
    char controlPackage[controlPackageSize];

	//for loop counter
    int i;
    
    //C 2 - Start 3 - End
    controlPackage[0] = control;
    
    //T1 - SIZE
    controlPackage[1] = 0; // Criar macro para SIZE(0)

    //L1 - Size of V1 field
    controlPackage[2] = strlen(fileSize);

    //Copies the file size to the V1 field
    for (i = 0; i < strlen(fileSize); i++)
        controlPackage[i + 3] = fileSize[i];

    //T2 NAME
    controlPackage[3 + strlen(fileSize)] = 1; //Criar macro para NAME(1)
    
    //L2 - Size of V2 field
    controlPackage[4 + strlen(fileSize)] = strlen(fileName);

    //Copies the file name to the V2 field
    for (i = 0; i < strlen(fileName); i++)
        controlPackage[5 + strlen(fileSize) + i] = fileName[i];

    //llwrite - LinkLayer function 
    return llwrite(appLayer->fd, controlPackage, size);
}

int startApplicationLayer(char* port, int baudRate, unsigned int messageSize, int retries, int timeout, char* fileName, int status) {
    
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
    if(initializeLinkLayer(application->fd, port, baudRate, messageSize, retries, timeout) < 0) {
        printf("Unable to initialize link layer.\n");
        return -1;
    }
	
	//Checks if the computter is in transmitter mode
    if(application->status == TRANSMITTER)
        write();
    //Checks if the computer is in receiver mode
    else if(application->status == RECEIVER)
        read();
    //Else nothing happens, simply closes the port

	//Closes the port
    close(application->fd);
    return 0;
}

int write(){

	//Opens the file as a binary file for reading
	FILE* file = fopen(application->fileName, "rb");
	if(!file){
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
		
	//Sends the start package to the receiver, 2 - START
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
	char* data = (char*)malloc((application->messageSize + 4) * sizeof(char));

	//Initializes the data package sequence number
	int packageSequenceNumber = 0;
	
	//Iteration that reads while there is something to read
	while((bytesRead = fread(data + 4, sizeof(char), application->messageSize, file)) > 0) {

		//C - Data which is represented as the number 1
		data[0] = 1;

		//N - Package sequence number (%255)
		data[1] = packageSequenceNumber;
		
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
