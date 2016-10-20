#include "LinkLayer.h"

struct applicationLayer {
    int fd;
    int status; //Connection mode, 0 - Receiver, 1 - Transmitter
    unsigned int messageSize;
    char* fileName;
};

int writeControlPackage(int control, char* fileName, char * fileSize);
int startApplicationLayer(char* port, int status, int baudRate, unsigned int messageSize, int retries, int timeout, char* fileName);
int write();
