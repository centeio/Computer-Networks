int initLinkLayer(int fd, char * port, int baudrate, int timeout, int triesMAX){
	linkLayer=(linkLayer*)malloc(sizeof(linkLayer));
	strcpy(linkLayer->port,port);
	linkLayer->baudrate=baudRate;
	linkLayer->timeout=timeout;
	linkLayer->triesMax=numTransmissions;
	linkLayer->sequenceNumber=0;
    //falta função termiosSettings
	if(!termiosSettings(fd)){
		return 0;
	}
	return 1;
}