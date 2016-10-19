int llwrite(int fd, char * buffer, unsigned int length){
  char* frame = (char*)malloc(length + DataSize * sizeof(char));
  char BCC_2 = BCC2(buffer, length);
  frame[0] = FLAG;
  frame[1] = A_TR;
  //linkLayer e sequenceNumber por definir 
  frame[2] = (linkLayer->sequenceNumber << 5);
  frame[3] = frame[1]^frame[2];
  memcpy(&frame[4], buffer, length);
  frame[4 + length] = BCC_2;
  frame[5 + length] = FLAG;

  //funcao stuff por implementar 
  int newSize = stuff(frame, length + DataSize * sizeof(char));

  int done = FALSE;
  int tries = 0;
  while(!done){
    if(tries == 0 || timeExceeded){
      if (tries >= linkLayer->numTransmissions) {
        printf("Error: number of tries exceeded.\n");
        printf("Connection aborted.\n");
        return -1;
      }
      if(write(fd, frame, newSize) == -1){
        printf("Error in write data frame...\n");
        return -1;
      }
      tries++;
      alarm(1);
    }
    char response[MAX_SIZE];
    
		recieveMessage(fd, response);
    if(response[1] == A_TR || response[0] == FLAG){
      if((response[2] & 0x0F) == C_REJ){
				globalStatistics->numberOfREJ++;
        if((response[2] >> 5) == linkLayer->sequenceNumber){
          alarm(0);
          tries = 0;
        }
        else{
          printf("Something went really wrong in REJ... \n");
          return -1;
        }
      }
      else if ((response[2] & 0x0F) == C_RR){
        if((response[2] >> 5) != linkLayer->sequenceNumber){
          alarm(0);
          linkLayer->sequenceNumber = (response[2] >> 5);
          done = TRUE;
        }
        else{
					timeExceeded = 1;
        }
      }
    }
  }
  return newSize;
}