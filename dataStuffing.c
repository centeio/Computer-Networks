unsigned int dataStuffing(char* buf, unsigned int frameSize){
	unsigned int newframeSize = frameSize;

	//i = 1 Starts at 1 because the initial flag must remain equal 
	//i < frameSize - 1 because the last flag must remain equal
	int i;
	for (i = 1; i < frameSize - 1; i++)
	
		//FLAG = 0x7e ESCAPE = 0x7d Create macro 
		if (buf[i] == FLAG || buf[i] == ESCAPE)
			
			newframeSize++;
			
	buf = (char*) realloc(buf, newframeSize);
	
	for (i = 1; i < frameSize - 1; i++) {
		if (buf[i] == FLAG || buf[i] == ESCAPE) {
			
			memmove(buf + i + 1, buf + i, frameSize - i);
			frameSize++;
			buf[i] = ESCAPE;
			//XOR 
			buf[i + 1] ^= 0x20;
		}
	}
	return newframeSize;
	
}