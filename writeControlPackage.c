int writeControlPackage(int control, char* fileName, char * fileSize){
  unsigned int size = 5 + strlen(fileName) + strlen(fileSize);
  // 5 : T1 L1 T2 L2 C
  // strlen(fileName) tamanho de V1
  // strlen(filesize) tamanho de V2 
  
  char controlPackage[size];
  // 
  int i, j = 0;

  controlPackage[0] = control;
  //Campo de controlo
  //2 START
  //3 END
  
	controlPackage[1] = 0; // Criar macro para SIZE(0)
	//SIZE( T = 0);
	controlPackage[2] = strlen(fileSize);
	//Tamanho de octetos do campo V
	for (i = 0; i < strlen(fileSize); i++)
		controlPackage[i + 3] = fileSize[i];
	//Leitura de todos os octetos

	controlPackage[3 + strlen(fileSize)] = 1; //Criar macro para NAME(1)
		
	controlPackage[4 + strlen(fileSize)] = strlen(fileName);
	//Tamanho de octetos do campo V
	for (i = 0; i < strlen(fileName); i++)
		controlPackage[5 + strlen(fileSize) + i] = fileName[i];
	//Leitura de todos os octetos
	
  return llwrite(appLayer->fd, controlPackage, size);
  //Chamada da funcao llwrite para enviar o pacote de controlo 
}