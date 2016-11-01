#include "ApplicationLayer.h"

int main(int argc, char** argv) {

    if(argc != 7) {
        printf("Number of Arguments Wrong: string port, int messageSize, int retries, int timeout, string fileName, int status\n");
        return -1;
    }
	
    char* port = argv[1];
    int messageSize = atoi(argv[2]);
    int retries = atoi(argv[3]);
    int timeout = atoi(argv[4]);
    char* fileName = argv[5];
    int status = atoi(argv[6]);

    //Application Layer function
    initializeApplicationLayer(port, messageSize, retries, timeout, fileName, status);

    return 0;
}
