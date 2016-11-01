CC = gcc
CFLAGS = -c -Wall

all: serialport

serialport: ApplicationLayer.o LinkLayer.o Lab1.o
	$(CC) ApplicationLayer.o LinkLayer.o Lab1.o -o serialport
	
ApplicationLayer.o: ApplicationLayer.c
	$(CC) $(CFLAGS) ApplicationLayer.c

LinkLayer.o : LinkLayer.c
	$(CC) $(CFLAGS) LinkLayer.c
	
Lab1.o : Lab1.c
	$(CC) $(CFLAGS) Lab1.c
	
clean:
	rm -rf *o serialport
