#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define PORT "9000"
#define BACKLOG 10
#define FILEPATH "/var/tmp/aesdsocketdata"
#define BUFSIZE 500

int running, listening, sending;

void sighandler(int sig){
	syslog(LOG_INFO,"Caught signal,exiting");
	printf("Caught signal %d\n",sig);

	running = 0;
	listening = 0;
	sending = 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main (int argc, char* argv[]){

	struct addrinfo hints, *servinfo, *ptr;
	struct sockaddr_storage cus_addr;
	struct sigaction new_act;
	socklen_t sin_size;
	int ret;
	int sockfd, cus_fd;
	FILE *fp;
	int numbytes, revlen, startpos, fsize, sentbytes, offset;
	char *revbuf;
	char buf[BUFSIZE];
	char s[INET6_ADDRSTRLEN];
	int yes=1;
	pid_t process_id=0;
	
	// Start logging
	openlog("aesdsocket", LOG_PID,LOG_USER);

	// Register signal handler for SIGTERM and SIGINT
	new_act.sa_handler = sighandler;
	new_act.sa_flags = 0;
	sigemptyset(&new_act.sa_mask);

	if ( sigaction(SIGTERM, &new_act, NULL) != 0 ) {
		syslog(LOG_ERR, "Failed to register for SIGTERM: %s", strerror(errno));
		return -1;
	}

	if ( sigaction(SIGINT, &new_act, NULL) != 0 ) {
		syslog(LOG_ERR, "Failed to register for SIGINIT: %s", strerror(errno));
		return -1;
	}
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	//Get server address information
	ret = getaddrinfo(NULL, PORT, &hints, &servinfo);
	if(ret){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(ret));
		return 1;
	}
	
	// loop through all the results and bind to the first we can
	for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
		if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype,
		    ptr->ai_protocol)) == -1) {
			perror("server: socket");
			syslog(LOG_ERR, "socket error: %s", strerror(errno));
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(yes)) == -1) {
			perror("setsockopt");
			syslog(LOG_ERR, "setsocket error: %s", strerror(errno));
			exit(1);
		}

		if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			syslog(LOG_ERR, "bind error: %s", strerror(errno));
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (ptr == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	
	// Check if the program run with daemon
	if (argc >= 2){
		if(strcmp(argv[1],"-d") == 0){
			process_id=fork();
		}
	}
	
	if (process_id < 0){
		perror("fork:");
		exit(1);
	}
	
	// Parent exit
	if (process_id > 0){
		exit(0);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	
	running = 1;
	listening = 1;
	sending = 0;
	
	printf("Waiting for connections...\n");
	
	while(running){
		sin_size = sizeof cus_addr;
		cus_fd = accept(sockfd, (struct sockaddr *)&cus_addr, &sin_size);
		if (cus_fd == -1) {
		    perror("accept");
		    continue;
		}

		inet_ntop(cus_addr.ss_family,
		    get_in_addr((struct sockaddr *)&cus_addr),
		    s, sizeof s);
		printf("Accepted connection from %s\n",s);
		syslog(LOG_INFO,"Accepted connection from %s",s);
		
		revlen = 0;
		startpos = 0;
		
		while(listening){
			memset(buf,0,sizeof(buf));
			if ((numbytes = recv(cus_fd, buf, BUFSIZE, 0)) == -1) {
				perror("recv");
				exit(1);
	    		}
    			if (numbytes > 0){
//    				printf("Received %d bytes of data\n",numbytes);
    				if(revlen == 0){
    					revlen += numbytes;
    					revbuf = (char*) malloc(revlen);
    				}else{
    					revlen += numbytes;
    					revbuf = (char*) realloc(revbuf,revlen);
    				}

    				memcpy(&revbuf[startpos],buf,numbytes);
    				startpos += numbytes;
    				
    				if(revbuf[startpos-1] == '\n'){
					    fp = fopen(FILEPATH, "a");
					    fwrite(revbuf,1,revlen,fp);
					    free(revbuf);
					    fclose(fp);
					    revlen = 0;
					    sending = 1;
					    break;			
    				}
    			}else{
				break;
    			}
		
				
		}
		
		if(sending){
			fp = fopen(FILEPATH,"r");
			fseek(fp, 0, SEEK_END);
			fsize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			char tempbuf[fsize+1];
			memset(tempbuf,0,fsize+1);
			fread(tempbuf, fsize, 1, fp);
			tempbuf[fsize+1]='\0';
			
			offset =0;
			while(offset<fsize){
				sentbytes=send(cus_fd,tempbuf+offset,fsize-offset,0);
				if(sentbytes <= 0){
					sending =0;
					break;
				}
				offset +=sentbytes;
			}
			fclose(fp);
			close(cus_fd);
			 
		}

	}
	close(sockfd);
	closelog();
	remove(FILEPATH);
	
	return 0;

}
