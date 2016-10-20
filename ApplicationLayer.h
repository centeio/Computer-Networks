#include "LinkLayer.h"

struct applicationLayer {
    int fd, status;
    unsigned int messageSize;
    char* fileName;
}

int open(char* port, int status, int baudRate, unsigned int messageSize, int retries, int timeout, char* fileName);