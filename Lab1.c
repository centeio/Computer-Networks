#include "ApplicationLayer.h"

int main(int argc, char** argv)
{
    if(argc != 8) {
        printf("Number of Arguments Wrong: string port, int baudRate, int messageSize, int retries, int timeout, string fileName, int status\n");
        return -1;
    }

    char* port = argv[1];
    int baudRate = atoi(argv[2]);
    int messageSize = atoi(argv[3]);
    int retries = atoi(argv[4]);
    int timeout = atoi(argv[5]);
    char* fileName = argv[6];
    int status = atoi(argv[7]);

    open(port, baudRate, messageSize, retries, timeout, fileName, status);

    return 0;
}