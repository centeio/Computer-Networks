int llread(int fd, char * buffer){
	volatile int done = FALSE;

	int state = 0;
	int size = 0;

	char * buf = (char*)malloc(APP_MAX_SIZE*2);
	while(!done){
		char c;
		if(state < 2){
			int retorno = read(fd, &c, 1);
			if(retorno == -1){
				printf("Error in llread()!! \n");
				return -1;
			}
		}

		switch(state){
			case 0:
			if(c == FLAG){
				buf[size] = c;
				size++;
				state++;
			}
			break;
			case 1:
			if(c == FLAG && size != 1){
				buf[size] = c;
				size++;
				state++;
			}
			else if(c == FLAG && size == 1){;}
			else{
				buf[size] = c;
				size++;
			}
			break;
			default:
			done = TRUE;
			break;
		}
	}
	int process = FALSE;
	int newSize = destuff(buf, size);


	if(buf[0] != FLAG || buf[1] != A_TR || buf[3] != (buf[1] ^ buf[2])){
		printf("Frame received with BCC wrong... \n");
		free(buf); 
		return -1;
	}

	int dataSize = newSize - DataSize * sizeof(char);

	char BCC = BCC2(&buf[4], dataSize);
	unsigned int sn = (buf[2] >> 5) & 1;

	char response[SUPERVISIONPACKAGE * sizeof(char)];
	response[0] = FLAG;
	response[1] = A_TR;
	response[4] = FLAG;

	if(linkLayer->sequenceNumber == sn){
      if(BCC != buf[newSize - 2]){ //if frame got corrupted
      	printf("Frame received with BCC2 wrong... \n Rejecting frame (REJ)... \n");
      	response[2] = (linkLayer->sequenceNumber << 5) | C_REJ;
      }
      else{
      	if(linkLayer->sequenceNumber == 0){
      		linkLayer->sequenceNumber = 1;
      	}
      	else{
      		linkLayer->sequenceNumber = 0;
      	}
      	process = TRUE;
      	response[2] = (linkLayer->sequenceNumber << 5) | C_RR;
      }
  }
  else{
  	response[2] = (linkLayer->sequenceNumber << 5) | C_RR;
  }
  response[3] = response[1] ^ response[2];
  sendSupervisonFrame(fd, response);
  if(process){
  	memcpy(buffer, &buffer[4], dataSize);
  	free(buf);
  	return dataSize;
  }
  free(buf);
  return -1;
}