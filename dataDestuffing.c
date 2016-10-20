unsigned int dataDestuffing(char* buf, unsigned int frameSize){
	int i;
	for (i = 1; i < frameSize - 1; i++) {
		//ESCAPE = 0x7d Create macro
		if (buf[i] == ESCAPE) {
			
			//Delete byte created with byte stuffing
			memmove(buf + i, buf + i + 1, frameSize - i - 1);
			frameSize--;
			buf[i] ^= 0x20;
			
		}
	}
	
	//updates frame
	buf = (char*) realloc(buf, frameSize);

	//Size of initial frame
	return frameSize;
	
}

