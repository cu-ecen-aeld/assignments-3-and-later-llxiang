#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){
	FILE* fp;
	int res = 0;
	
	openlog("writer",LOG_PID,LOG_USER);
	syslog(LOG_DEBUG,"start logging");
	
	if (argc < 3){
		syslog(LOG_ERR,"The full path to a file or the text string is not specified argc:%d",argc);
		res = 1;
		}
	else{
		fp = fopen(argv[1],"w");
		if (fp == NULL){
			syslog(LOG_ERR, "Failed to open the file: %s", strerror(errno));
			res = 1;
			}
		else{
			if(fprintf(fp, "%s",argv[2])<0){
				syslog(LOG_ERR, "Failed to write the file: %s", strerror(errno));
				fclose(fp);
				res = 1;
				}
			else{
				syslog(LOG_DEBUG,"Writing %s to %s",argv[2],argv[1]);
				}
			}
		}
		
	closelog();
	return res;
	}
