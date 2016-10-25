#include "LinkLayer.h"

struct applicationLayer {
    int fd;
    int status; //Connection mode, 0 - Receiver, 1 - Transmitter
    unsigned int messageSize;
    unsigned char* fileName;
};

struct applicationLayer* application;

int writeControlPackage(int control, char* fileName, char * fileSize);
int initializeApplicationLayer(unsigned char* port, unsigned int messageSize, int retries, int timeout, unsigned char* fileName, int status);
int send();
int receive();
