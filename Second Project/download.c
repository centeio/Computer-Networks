#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MESSAGE_SIZE 200

int getip(char *host, char *hostaddr)
{
	struct hostent *h = malloc(sizeof(struct hostent));

/*
struct hostent {
	char    *h_name;	Official name of the host.
    char    **h_aliases;	A NULL-terminated array of alternate names for the host.
	int     h_addrtype;	The type of address being returned; usually AF_INET.
    int     h_length;	The length of the address in bytes.
	char    **h_addr_list;	A zero-terminated array of network addresses for the host.
	Host addresses are in Network Byte Order.
};

#define h_addr h_addr_list[0]	The first address in h_addr_list.
*/
    if ((h=gethostbyname(host)) == NULL) {
        herror("gethostbyname");
        return -1;
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));
    strcpy(hostaddr, inet_ntoa(*((struct in_addr *)h->h_addr)));
	printf("Free h\n");
    return 0;
}

int ftpLogin(int socketfd, char* user, char* pass){
  	printf("Login\n");
	char reply[MESSAGE_SIZE] = "", *usercmd = malloc(sizeof(char) * MESSAGE_SIZE), *passcmd = malloc(sizeof(char) * MESSAGE_SIZE);

	read(socketfd, reply, MESSAGE_SIZE);

	sprintf(usercmd, "USER %s\n", user);
  	printf("%s\n", usercmd);
	write(socketfd, usercmd, strlen(usercmd));
	read(socketfd, reply, MESSAGE_SIZE);
	printf("%s\n", reply);

	bzero(reply, strlen(reply));

	sprintf(passcmd, "PASS %s\n", pass);
  	printf("%s\n", passcmd);
	write(socketfd, passcmd, strlen(passcmd));
	read(socketfd, reply, MESSAGE_SIZE);

	printf("%s\n", reply);

	if (reply[0] == '2' && reply[1] == '3' && reply[2]== '0'){
		free(usercmd);
		free(passcmd);
		return 0; //SUCCESS
	}

	free(usercmd);
	free(passcmd);
	return -1;

}

int ftpPasv(int shost, char* host, unsigned int *port){
	char reply[MESSAGE_SIZE] = "", *command =  "pasv\n";
  	int h1, h2, h3, h4, h5, h6;

  	bzero(reply, strlen(reply));

	write(shost, command, strlen(command));
	read(shost, reply, MESSAGE_SIZE);

  	if(sscanf(reply, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", &h1, &h2, &h3, &h4, &h5, &h6) != 6){
    	printf("Answer does not match for Passive Mode:\n --> %s \n", reply);
    	return -1;
  	}

	sprintf(host, "%d.%d.%d.%d", h1, h2, h3, h4);
	*port = h5*256 + h6;

	return 0;
}

int ftpDownload(int socketfd, int shost, char* filename, char* path) {
	char reply[MESSAGE_SIZE] = "", *retrcmd = malloc(sizeof(char) * MESSAGE_SIZE);

	bzero(reply, strlen(reply));
	sprintf(retrcmd, "retr %s\n", path);
	printf("Path: %s\nFilename. %s\n", path, filename);

	write(socketfd, retrcmd, strlen(retrcmd));
	read(socketfd, reply, MESSAGE_SIZE);
	printf("Reply: %s\n", reply);


  if (reply[0] == '1' && reply[1] == '5' && reply[2]== '0') {

    int fd =  open(filename, O_WRONLY | O_CREAT, 0777), nbytes = 0, counter = 0;
    char* trans = malloc(sizeof(char) * MESSAGE_SIZE);

    while((nbytes = read(shost, trans, MESSAGE_SIZE)) != 0) {
		if(!(counter % 1000))
			printf(".");
		fflush(stdout);
		counter++;	      
		write(fd, trans, nbytes);
	}

	printf("\n");

    close(fd);

    bzero(reply, strlen(reply));
    read(socketfd, reply, MESSAGE_SIZE);

    if (reply[0] == '2'){

		printf("Download succeeded\n");
		free(trans);
		free(retrcmd);
		return 0; //SUCCESS
	}

	printf("Download failed\n");

  }

	free(retrcmd);
	return -1;

}

int parseURL(char * url, char * host, char * path, char * user, char * pass, char* filename){

	int ret = sscanf(url, "ftp://%[^:]:%[^@]@%[^/]/%s\n", user, pass, host, path);
	if(ret != 4){
		ret = sscanf(url, "ftp://%[^/]/%s\n", host, path);
		if (ret != 2) {
			return -1;
      	}
      	strcpy(user, "anonymous");
      	strcpy(pass, "mail@domain");
    } //not success

	char * last = strrchr(path,'/');
	strcpy(filename, last+1);

	printf("Filename: %s\n", filename);
	return 0; //success
}

int ftpConnect(char* server_ip, const unsigned int port){
	//connect socket
	struct	sockaddr_in server_addr;
	int sockfd;

	/*server address handling*/
	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("socket()");
		return -1;
	}

	/*connect to the server*/
  if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){	
        	perror("connect()");
			return -1;
	}

	return sockfd;
}

int main(int argc, char **argv){

	char host[MESSAGE_SIZE] = "";
	char path[MESSAGE_SIZE] = ""; 
	char user[MESSAGE_SIZE] = "";
	char pass[MESSAGE_SIZE] = "";
	char filename[MESSAGE_SIZE] = "";



	if(argc != 2){
		printf("usage: download ftp://[<user>:<password>@]<host>/<url-path> \n");
		return -1;
	}
  	printf("Going to parse\n");

	if(parseURL(argv[1], host, path, user, pass, filename) == -1){
		printf("url does not match ftp://[<user>:<password>@]<host>/<url-path> \n");
		return -1;
	}
  	printf("Finished parse\n");

	
	char* hostaddr = malloc(sizeof(char) * MESSAGE_SIZE);

	if(getip(host, hostaddr) != 0){
		printf("Failed getting ip\n");
		free(hostaddr);		
		return -1;
	}

	unsigned int port = 21, sport;

	printf("Starting connection...\n");
	printf("%s\n", hostaddr);

	int sockfd = ftpConnect(hostaddr, port);

	printf("ftpconnect done\n");

	if(sockfd == -1){
		free(hostaddr);
		printf("Connection failed\n");
		return -1;
	}

	//login
	if(ftpLogin(sockfd, user, pass) != 0){
	free(hostaddr);
	close(sockfd);
    printf("Login failed\n");
    return -1;
  }

	printf("Login succeeded\n");

	//passv - calcular a porta
	char *shost = malloc(sizeof(char)*MESSAGE_SIZE);
	if(ftpPasv(sockfd, shost, &sport) != 0){
		printf("Passive Mode Failed\n");
		free(hostaddr);
		free(shost);
		close(sockfd);
		return -1;
  }


	printf("Passive Mode Established\n");
	int sockhost = ftpConnect(shost, sport);
	if(sockhost == -1){
		printf("Connection failed\n");
		free(hostaddr);
		free(shost);
		close(sockfd);
		return -1;
	}
	//download
	if(ftpDownload(sockfd, sockhost, filename, path) != 0){
		free(hostaddr);
		free(shost);
		close(sockfd);
		close(sockhost);

    printf("Download failed\n");
    return -1;
  }
  printf("Download finished\n");

	char* quit = "quit\n";
	write(sockfd, quit, strlen(quit));
	write(sockhost, quit, strlen(quit));

	free(hostaddr);
	free(shost);
	close(sockfd);
	close(sockhost);

	return 0;

}
