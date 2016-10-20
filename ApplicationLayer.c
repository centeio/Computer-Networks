#include "ApplicationLayer.h"

int open(char* port, int baudRate, unsigned int messageSize, int retries, int timeout, char* fileName, int status){
    
    application = (applicationLayer*) malloc(sizeof(applicationLayer));
    application->fd = open(port, O_RDWR | O_NOCTTY );;
    application->messageSize = messageSize;

    if (application->fd < 0) {
        printf("Error opening port.\n");
        return -1;
    }

    application->status = status;
    application->fileName = fileName;

    if(!startLinkLayer(application->fd, port, status, baudRate, messageSize, retries, timeout)){
        printf("Couldn't initialize link layer.\n");
        return -1;
    }

    if(application->status == TRANSMITTER){
        write();
    }
    else if(application->status == RECEIVER){
        read();
    }

    close(application->fd);
    return 0;
}