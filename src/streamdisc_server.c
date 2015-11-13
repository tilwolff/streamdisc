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

void parse_request(int fd,streamdisc_http_request req){
	long i=0, j=0, len=0;
	char* buffer=malloc(BUFSIZE*sizeof(char));
	len =read(fd,buffer,BUFSIZE); 	/* read Web request in one go */
	
	if(len == 0 || len == -1) {	/* read failure stop now */
		log_msg("Failed to read browser request.");
		free(buffer);
		return;                 /* req->method is still null, streamdisc_serve will throw error so we do not care here*/
	}
	if(len > 4 && len < BUFSIZE){	/* return code is valid chars. should contains at least "GET /"*/
		buffer[len]=0; 		/* terminate the buffer */
	}
	else{
	        log_msg("Invalid browser request.");
       		free(buffer);
	        return;                 /* req->method is still null, streamdisc_serve will throw error so we do not care here*/
	}
	for(i=0;i<len;i++){	        /* remove CF and LF characters */
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
	        log_msg("Invalid browser request.");
	        free(buffer);
	        return;
	}

        
	while(i<BUFSIZE) { /* null terminate after the second space to ignore extra stuff */
		if(buffer[i] == ' ') { /* string is "GET URI " +lots of other stuff */
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
        free(buffer);
        req->base_url="";
}

int main(int argc, char **argv)
{
	int i, port, listenfd, socketfd;
	pid_t pid;
	socklen_t length;
	static struct sockaddr_in cli_addr;             /* static = initialised to zeros */
	static struct sockaddr_in serv_addr;            /* static = initialised to zeros */
        static struct streamdisc_http_request_s req;    /* static = initialised to zeros */

	if( argc < 3  || argc > 4 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "-?") ) {
		(void)printf("Usage: streamdisc_server <port> <device> [-b]\n\n"
	"\tExample (foreground): streamdisc_server 80 /dev/sr0\n"
	"\t\tLogs to stderr.\n\n"
	
        "\tExample (background): streamdisc_server 80 /dev/sr0 -b\n"
	"\t\tOld-style daemon runs in background and logs to /var/log/streamdisc.log.\n\n"
	"\tNo warranty given or implied.\n\n");
		exit(0);
	}
        
        int background=0;
        if( 4==argc && !strcmp(argv[3],"-b")){
                background=1;
                remove_log_file();
        }
        else {
                set_log(1 /*log to stderr */,0 /*log to file */);
        }

        /* check if device file is accessilbe */
        int test_fd=open(argv[2], O_RDONLY); 
	if (test_fd == -1){
		log_err_die(ERR_NODEV);
	}
	close(test_fd);
        
        
	port = atoi(argv[1]);
	if(port <= 0 || port >60000){
		log_msg_die("Invalid port number specified (try 1->60000)");
        }
        
        /* setup the network socket */
	listenfd = socket(AF_INET, SOCK_STREAM,0);
	if(listenfd <0){
	        log_msg_die("Error setting up network socket.");
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0){
	        (void)close(listenfd);
	        log_msg_die("Error binding to network socket.");
	}
		
	if(listen(listenfd,64) <0){
	        (void)close(listenfd);
	        log_msg_die("Error listening on network socket.");
	}
        
        
        if(background){	/* Become deamon + unstoppable and no zombies children (= no wait()) */
	        pid=fork();
	        if(pid != 0){
	                exit( 0 ); /* parent returns to shell */
	        }
	        set_log(0 /*log to stderr */,1 /*log to file */);
	}
        
        char* logbuf=malloc(128*sizeof(char));
        snprintf(logbuf,127,"streamdisc_server starting, pid:%d, port: %d.",getpid(),port);
	log_msg(logbuf);
	free(logbuf);
	
	if(background){
	        (void)signal(SIGCHLD, SIG_IGN); /* ignore child death */
	        (void)signal(SIGHUP, SIG_IGN);  /* ignore terminal hangups */
	        for(i=0;i<32768;i++){              /* close open files */
	                if (i!=listenfd) (void)close(i);
	        }
	        (void)setpgrp();		/* break away from process group */
        }

	while(1){
		length = sizeof(cli_addr);
		socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);
		if(socketfd < 0){
		        (void)close(listenfd);
		        log_msg_die("Error accepting connection.");
		}
                pid = fork();
		if(pid < 0){
		        (void)close(listenfd);
		        log_msg_die("Error forking server process.");
		}
		else if(0==pid) { 	/* child. interpret request and serve if possible */
			(void)close(listenfd);
			parse_request(socketfd,&req);
			log_request(&req);
			(void)streamdisc_serve(socketfd, &req, argv[2]); /* do not care about return value */
			(void)close(socketfd);
			exit(0);
		} 
		else { 	/* parent */
			(void)close(socketfd);
		}
	}
}
