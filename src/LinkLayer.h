#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7e
#define ESCAPE 0x7d
#define A_TR 0x03
#define A_RT 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_REJ 0x01
#define C_RR 0x05
#define C_DISC 0x0B
#define RECEIVER 0
#define TRANSMITTER 1
#define SUPERVISIONPACKAGE 5

struct linkLayer {
    char port[20];
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int triesMAX;
};

enum STATE{
	start,
	flag,
	a,
	c,
	bcc,
	stop
} state;

struct linkLayer* llink;
struct termios oldtio,newtio;

unsigned int timeExceeded;

void handleAlarm();
int sendMessage(int fd, char* message);
int receiveMessage(int fd, char* message);
int initializeLinkLayer(int fd, char * port, int triesMAX, int timeout);
int llopen(int fd, int connectionMode);
int llwrite(int fd, char* buffer, unsigned int length);
int llread(int fd, char* buffer);
int llclose(int fd, int mode);
unsigned int dataStuffing(char* buffer, unsigned int frameSize);
unsigned int dataDestuffing(char* buffer, unsigned int frameSize);
char findBCC2(char* data, unsigned int size);
