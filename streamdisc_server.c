/*
 * This file is part of streamdisc
 *
 *
 * streamdisc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * streamdisc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with streamdisc; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "streamdisc.h"
#include "readdisc.h"
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 8192

void log_and_exit(char *msg){
        char logbuffer[BUFSIZE];
        int fd=STDERR_FILENO;

        sprintf(logbuffer,"%s\nExiting.\n",msg);
        //if((fd = open("streamdisc_server.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		(void)write(fd,logbuffer,strlen(logbuffer));   
	//	(void)close(fd);
	//}
        exit(-1);
}

void parse_request(int fd,streamdisc_http_request req){
	long i=0, j=0, len=0;
	static char buffer[BUFSIZE]; /* static so zero filled */
	len =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	
	if(len == 0 || len == -1) {	/* read failure stop now */
		log_and_exit("Failed to read browser request");
	}
	if(len > 0 && len < BUFSIZE){	/* return code is valid chars */
		buffer[len]=0; 		/* terminate the buffer */
	}
	else{
	        return;                    /* req->method is still null, streamdisc_serve will throw error so we do not care here*/
	}
	for(i=0;i<len;i++){	/* remove CF and LF characters */
		if(buffer[i] == '\r' || buffer[i] == '\n'){
			buffer[i]=0;
		}
	}

	if( !strncmp(buffer,"GET ",4) || !strncmp(buffer,"get ",4) ) {
		req->method="GET";
		i=j=4;
	}
	else if( !strncmp(buffer,"HEAD ",5) || !strncmp(buffer,"head ",5)){
		req->method="HEAD";
		i=j=5;
	}
	else{
	        return;
	}

        
	while(i<BUFSIZE) { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') { /* string is "GET URL " +lots of other stuff */
			buffer[i] = 0;
			break;
		}
		i++;
	}
	/* retrieve requested uri */
	if( !strncmp(buffer+j,"http://",7)){ /* absolute uri */
	        j=j+7;
	        while(0==buffer[j] && '/'!=buffer[j]){
	                j++;
	        }
	        if(0==buffer[j]){
	                req->path_info="";
	        }
	        else{
	                req->path_info=strdup(buffer+j+1);
	        }
	}
	else { /*relative uri */
	        req->path_info=strdup(buffer+j);
	}

	/* retrieve requested range */
	while( j < len){
	   
	        /* moving to new line: look for pos with pos-1=null and pos!=null */
	        if( 0!=buffer[j] || 0==buffer[j-1]){
        	        if( !strncmp(buffer+j,"Range: ",7)){
                	        j=j+7;
                	        req->http_range=strdup(buffer+j);
                	}
                }
                j++;
	}
	
	
	
//req->http_range="";
req->base_url="";
if (req->http_range) fprintf(stderr,"PATH_INFO: %s\nMETHOD: %s\nBASE_URL: %s\nHTTP_RANGE: %s\n",req->path_info,req->method,req->base_url,req->http_range);
}

int main(int argc, char **argv)
{
	int i, port, listenfd, socketfd;
	pid_t pid;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
        static struct streamdisc_http_request_s req; /* static = initialised to zeros */

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		(void)printf("Usage: streamdisc_server <port> <device>\n\n"
	"\tExample: streamdisc_server 80 /dev/sr0\n\n"
	"\tDaemonizes and returns pid of background process on success.\n"
	"\tNo warranty given or implied.\n\n");
		exit(0);
	}

        int test_fd=open(argv[2], O_RDONLY);
	if (test_fd == -1){
		log_err_die(ERR_NODEV);
	}
	close(test_fd);
        
        
	port = atoi(argv[1]);
	if(port <= 0 || port >60000){
		log_and_exit("Invalid port number specified (try 1->60000)");
        }
        
	/* Become deamon + unstopable and no zombies children (= no wait()) */
	pid=fork();
	if(pid != 0){
	        fprintf(stderr,"Exiting with pid %i\n",getpid());fflush(stderr);
	        exit( pid ); /* parent returns to shell */
	}

	fprintf(stderr,"streamdisc_server starting, pid:%d, port: %d.\n",getpid(),port);
	(void)signal(SIGCHLD, SIG_IGN); /* ignore child death */
	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
	for(i=3;i<32;i++){
	        (void)close(i); /* close open files */
	}
	(void)setpgrp();		/* break away from process group */
	/* setup the network socket */
	listenfd = socket(AF_INET, SOCK_STREAM,0);
	if(listenfd <0){
	        log_and_exit("Error setting up network socket");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0){
	        log_and_exit("Error binding to network socket");
	}
		
	if(listen(listenfd,64) <0){
	        log_and_exit("Error listening on network socket");
	}
	fprintf(stderr,"pid: %i,before the loop\n",getpid());
	while(1) {
	        fprintf(stderr,"pid: %i,entered the loop\n",getpid());
		length = sizeof(cli_addr);
		socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);
		fprintf(stderr,"pid: %i,socketfd: %i\n",getpid(),socketfd);
		if(socketfd < 0){
		        log_and_exit("Error accepting connection");
		}
		else{
		        fprintf(stderr,"Here I am\n");fflush(stderr);
		}
                pid = fork();
		if(pid < 0){
		        log_and_exit("Error forking server process");
		}
		else if(pid == 0) { 	/* child */

			(void)close(listenfd);
			parse_request(socketfd,&req);
			fprintf(stderr,"Here I am after parse_request\n");fflush(stderr);
			streamdisc_serve(socketfd, &req, argv[2]);/* never returns */
		} 
		else { 	/* parent */
			(void)close(socketfd);
		}
	}
}
