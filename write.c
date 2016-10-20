int send(){
  FILE* file = fopen(appLayer->fileName, "rb");
  if(!file){
    printf("Error: Can't open the specified file \n");
    return 0;
  }

  fseek(file, 0, SEEK_END);
  //coloca o cursor no final do ficheiro
  unsigned int fileSize = ftell(file);
  //guarda o numero de caracteres do ficheiro que é igual ao numero de bytes
  //um caracter ocupa espaco de um byte
  fseek(file, 0, SEEK_SET);
  //coloca o cursor na posicao inicial novamente para posteriormente ler o ficheiro no inicio
  char fileSizeString[20];
  //coloca o tamanho em string 
  sprintf(fileSizeString, "%u", fileSize);
  printf("File size: %u Bytes \n", fileSize);
  
  
  if(llopen(appLayer->fd,appLayer->mode) < 0){
    return 0;
  }
  if(writeControlPackage(2,appLayer->fileName, fileSizeString) < 0){
	  //START = 2 criar macro
    printf("Error: Can't send start control package\n");
    return 0;
  }
	printf("Starting sending...\n");
	unsigned int bytes;
	char fBytes[appLayer->messageMaxSize + 4];
	//Envia o campo de dados P com K octetos
	
	int nPackages = 0;
	//inicializa o numero de packages enviados a zero
	while((bytes = fread(&fBytes[4],sizeof(char), appLayer->messageMaxSize, file)) > 0){
		nPackages++;
		//incrementa o numero de packages enviados
		fBytes[0] = 1;
		//DATA = 1 criar macro
		fBytes[1] = nPackages;
		//N numero de sequencia de modulo 255 L1
		fBytes[2] = bytes >> 8;
		//Avanca 8 para ignorar o 8 bits menos significativos
		//Ficar com o bit mais significativo
		fBytes[3] = bytes & 0xFF;
		//Operacao para guardar os 8 bits  menos significativos L2
		if(llwrite(appLayer->fd, fBytes, bytes + 4) == -1){
			printf("Error: Can't send package %d... \n", nPackages);
			return 0;
		}
		//chamada da funcao llwrite para enviar o pacote de controlo 
	}
  if (fclose(file) != 0) {
    printf("Error: File is not closed....\n");
    return 0;
  if(writeControlPackage(END,appLayer->fileName, fileSizeString) < 0){
    printf("Error: Can't send END control package\n");
    return 0;
  }
  if(llclose(appLayer->fd,appLayer->mode) < 0){
    return 0;
  }
  printf("\n");
  return 1;
}
