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

void log_and_exit(char *msg){
        fprintf(stderr,"%s\nExiting.\n",msg);
        exit(-1);
}

void web(int fd){
        write(fd,"HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
        exit(0);
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in cli_addr; /* static = initialised to zeros */
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	if( argc < 3  || argc > 3 || !strcmp(argv[1], "-?") ) {
		(void)printf("Usage: streamdisc_server <port> <device>\n\n"
	"\tExample: streamdisc_server 80 /dev/sr0\n\n"
	"\tDaemonizes and returns pid of background process on success.\n"
	"\tNo warranty given or implied.\n\n");
		exit(0);
	}

        int test_fd=open(argv[2], O_RDONLY);
	if (test_fd == -1)
		log_err_die(ERR_NODEV);
	close(test_fd);
        
        
	port = atoi(argv[1]);
	if(port <= 0 || port >60000)
		log_and_exit("Invalid port number specified (try 1->60000)");
	fprintf(stderr,"streamdisc_server starting, pid:%d, port: %d.\n",getpid(),port);
        
	/* Become deamon + unstopable and no zombies children (= no wait()) */
	pid=fork();
	if(pid != 0)
		return pid; /* parent returns pid to shell */
	(void)signal(SIGCLD, SIG_IGN); /* ignore child death */
	(void)signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
	for(i=3;i<32;i++)
		(void)close(i);		/* close open files */
	(void)setpgrp();		/* break away from process group */
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0)
		log_and_exit("Error setting up network socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0)
		log_and_exit("Error binding to network socket");
	if( listen(listenfd,64) <0)
		log_and_exit("Error listening on network socket");
	while(1) {
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			log_and_exit("Error accepting connection");
		if((pid = fork()) < 0) {
			log_and_exit("Error forking server process");
		}
		else {
			if(pid == 0) { 	/* child */
				(void)close(listenfd);
				web(socketfd); /* never returns */
			} else { 	/* parent */
				(void)close(socketfd);
			}
		}
	}
}
