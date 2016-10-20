#include "ApplicationLayer.h"

int writeControlPackage(int control, char* fileName, char * fileSize) {

    // C T1 L1 V1(fileSize) T2 L2 V2(fileName) 
    unsigned int controlPackageSize = 5 + strlen(fileName) + strlen(fileSize);

    char controlPackage[controlPackageSize];
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

int open(char* port, int baudRate, unsigned int messageSize, int retries, int timeout, char* fileName, int status) {
    
    application = (applicationLayer*) malloc(sizeof(applicationLayer));
    application->fd = open(port, O_RDWR | O_NOCTTY );;
    application->messageSize = messageSize;

    if (application->fd < 0) {
        printf("Error opening port.\n");
        return -1;
    }

    application->status = status;
    application->fileName = fileName;

    if(!startLinkLayer(application->fd, port, status, baudRate, messageSize, retries, timeout)) {
        printf("Couldn't initialize link layer.\n");
        return -1;
    }

    if(application->status == TRANSMITTER) {
        write();
    }
    else if(application->status == RECEIVER) {
        read();
    }

    close(application->fd);
    return 0;
}