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
 
 
#include "readdisc.h"
#include "streamdisc.h"
#include <errno.h>
#include <ctype.h>

#define NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define NOT_SUPPORTED "<html><head><title>Not Supported</title></head><body>The method you are trying to use is not supported by this server</body></html>"
#define DIRLISTING_OPEN "<html><head><title>Directory Listing</title></head><body><h2>Index of /titles</h2><div class=\"list\"><table summary=\"Directory Listing\" cellpadding=\"2px\" cellspacing=\"2px\"><thead><tr><th class=\"n\">Name</th><th class=\"m\">Last Modified</th><th class=\"s\">Size</th><th class=\"t\">Type</th></tr></thead><tbody>\n"
#define DIRLISTING_CLOSE "</tbody></table></div></body></html>\n"
#define DIRLISTING_ITEM	"<tr><td class=\"n\"><a href=\"%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%.1f%s</td><td class=\"t\">%s</td></tr>\n"

int abort_requested=0;
disc_device_t dd_disc;
char buf[512];

void
signal_callback_handler(int signum){
	abort_requested=1;
}

void log_request(streamdisc_http_request req){
        const char *non_def="Not defined";
        const char *method=(req->method) ? req->method : non_def;
        const char *base_url=(req->base_url) ? req->base_url : non_def;
        const char *path_info=(req->path_info) ? req->path_info : non_def;
        const char *http_range=(req->http_range) ? req->http_range : non_def;
        
        snprintf(buf,511,"Request received and picked up by PID %i:\n--Method: %s\n--Base URL: %s\n--Path Info: %s\n--Range: %s\n",getpid(),method,base_url,path_info,http_range);
        log_msg(buf);
}

int init(int32_t title_number,char *device_path){
	int intRetVal=ERR_FATAL;
	dd_disc=get_device_object();

	/* open device*/
	intRetVal=open_disc(dd_disc,device_path);
	if(ERR_OK!=intRetVal){
		return intRetVal;
	}

	/* set title */
	if(title_number>0){
		intRetVal=set_titleset(dd_disc,title_number);
		return intRetVal;
	}

	/* directory listing update title list*/
	intRetVal=update_title_list(dd_disc);
	return intRetVal;
	
}

int32_t interpret_request(char *path_info){
	if (!strcmp(path_info,"/" )){
		return 0;
	}
	if (!strncmp(path_info,"/title",6)){ /*Format is /titleXXXXX.[m2ts,vob]*/
		if(!strncmp(path_info+11,".m2ts",5) || !strncmp(path_info+11,".vob",4)){
			int32_t title_num=atoi(path_info+6);
			if (title_num) return title_num;
		}
	}
	return -1;
}

int get_range(const char* http_range, off64_t* start, off64_t* end){
	*start=0;
	*end=0;
	
	if(strncmp(http_range,"bytes=",6) || strlen(http_range)<8)	// invalid range, must be at least "bytes=x-" with x in 0,...,9
		return -1;

	off64_t result=0;
	const char* pos=http_range+6;
	while ('\0'!=*pos && isspace(*pos)) 				//discard whitespace
		pos++;


	if  isdigit(*pos){						//first value given, we are in the case bytes=nnn-[mmm]	
		errno=0;
		result=strtoull(pos,(char**)&pos,0);				//strtoull discards whitespace
		if (errno){ 
			return -1;
		}
		*start=result;

		while ('\0'!=*pos && isspace(*pos))			//discard whitespace
			pos++;

		if('-' != *pos)
			return -1;					//must find minus sign after first number
		pos++;
		while ('\0'!=*pos && isspace(*pos))			//discard whitespace
			pos++;

		if('\0'==*pos || ','==*pos){				//second value may be empty, additional comma separated ranges are ignored
			*end=0;
			return 0;
		}
		errno=0;
		result=strtoull(pos,NULL,0);
		if (errno)						//range invalid
			return -1;
		if (result<*start)
			return -1;					//range invalid
		*end=result;
		return 0;		
	}
	if ('-'==*pos){							//only second value given, we are in the case bytes=-mmm which means last mmm bytes are requested
		pos++;
		errno=0;
		result=strtoull(pos,NULL,0);				
		if (errno)						//range invalid
			return -1;
		*start=-result;
		return 0;		
	}
	return -1;                                                      //range invalid
}

void header_notfound(int fd){
	write(fd,"HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n",48);
}

void header_bad(int fd){
	write(fd,"HTTP/1.1 400 Bad Request\n\n",26);
}

void header_unsupported(int fd){
	write(fd,"HTTP/1.1 401 Not Supported\n\n",28);
}

void header_dirlisting(int fd){
	write(fd,"HTTP/1.1 200 OK\nContent-Type: text/html\n\n",41);	
}

void header_redirect_root(int fd, char *base_url){
	snprintf(buf,511,"HTTP/1.1 301 Moved Permanently\nLocation: %s/\n\n",base_url);//location needs slash in the end in order to be interpreted as directory
	write(fd,buf,strlen(buf));
}

void header_title(int fd,off64_t range_start, off64_t range_end){
	
	if(range_end>dd_disc->current_title_size-1 || 0==range_end)
		range_end=dd_disc->current_title_size-1;

	if(range_start<0)
		range_start=dd_disc->current_title_size+range_start-1>=0 ? dd_disc->current_title_size+range_start-1 : 0;
	

	/* mime types */
	const char* tp_bd="video/MP2T";	
	const char* tp_dvd="video/dvd";
	const char* tp= STATUS_DVD==dd_disc->status ? tp_dvd : tp_bd;
	if(range_start==0 && range_end==dd_disc->current_title_size-1){
		snprintf(buf,511,"HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %zd\nAccept-Ranges: bytes\n\n",tp,dd_disc->current_title_size);
	}else if(range_start>dd_disc->current_title_size-1){
		snprintf(buf,511,"HTTP/1.1 416 Requested range not satisfiable\n\n");
	}else{
		snprintf(buf,511,"HTTP/1.1 206 Partial content\nContent-Type:%s\nContent-Length: %zd\nAccept-Ranges: bytes\nContent-Range: %zd-%zd/%zd\n\n",\
		        tp,range_end-range_start+1,range_start,range_end,dd_disc->current_title_size);
	}
	write(fd,buf,strlen(buf));
}

void serve_notfound(int fd){
	write(fd,NOT_FOUND,strlen(NOT_FOUND));
}



void serve_dirlisting(int fd){
	write(fd,DIRLISTING_OPEN,strlen(DIRLISTING_OPEN));
	
	/* filename */	
	const char* fn_pattern_bd="title%05d.m2ts";	
	const char* fn_pattern_dvd="title%05d.vob";
	const char* fn_pattern= STATUS_DVD==dd_disc->status ? fn_pattern_dvd : fn_pattern_bd;
	char fn[1024];
	/* mime types */
	const char* tp_bd="video/MP2T";	
	const char* tp_dvd="video/dvd";
	const char* tp= STATUS_DVD==dd_disc->status ? tp_dvd : tp_bd;
	/* last modified, does not matter here but important for xbmc/kodi client to parse directory listing */
	const char* lm="2000-Jan-01 00:00:00";

	/* size */
	double sz;
	char *unit;

	/* print titles info */
	uint32_t i=1;
	while (i<=dd_disc->titles_count){
		sz=(double)dd_disc->title_sizes[i-1];
		
		if(sz>1024){
			sz/=1024;
			unit="K";
		}else{
			unit="B";
		}

		if(sz>1024){
			sz/=1024;
			unit="M";
		}
		
		if(sz>1024){
			sz/=1024;
			unit="G";
		}
		sprintf(fn,fn_pattern,i); //filename
		snprintf(buf,511,DIRLISTING_ITEM,fn,fn,lm,sz,unit,tp); //html item
		write(fd,buf,strlen(buf));
		i++;
	}

	write(fd,DIRLISTING_CLOSE,strlen(DIRLISTING_CLOSE));
}

int serve_title(int fd,off64_t range_start, off64_t range_end){
	
	
	/* get buffer size*/
	int intBufSizeBytes;
	int intRetVal=get_buffer_size(dd_disc,&intBufSizeBytes);
	if(ERR_OK!=intRetVal) return intRetVal;

        
	
	if(range_end+1>dd_disc->current_title_size || 0==range_end)
		range_end=dd_disc->current_title_size-1;

	if(range_start<0)
		range_start=dd_disc->current_title_size+range_start-1>=0 ? dd_disc->current_title_size+range_start-1 : 0;

	if(range_start>dd_disc->current_title_size-1)
		return 0;

	off64_t offset=range_start;
	off64_t bytes_total=range_end-range_start+1;

        unsigned char* tmpbuf=malloc(intBufSizeBytes*sizeof(unsigned char));
	off64_t bytes_read=0;
	off64_t bytes_to_read=0;
	while(bytes_total>0 && 0==abort_requested){
		bytes_to_read=(bytes_total>intBufSizeBytes)? intBufSizeBytes: bytes_total;	
		bytes_read=read_bytes( dd_disc, offset, bytes_to_read, tmpbuf );
		if(bytes_read<0){
		        free(tmpbuf);
			return ERR_READ;
		}
		
		if(write(fd,tmpbuf,bytes_read)!=(ssize_t)bytes_read){
		        free(tmpbuf);
			return ERR_FILEWRITE;
		}
		bytes_total-=bytes_read;
		offset+=bytes_read;
	}
	free(tmpbuf);
	return ERR_OK;
}

 
int streamdisc_serve(int fd, streamdisc_http_request req, char *device_path){
        int intRetVal=ERR_OK;
	int32_t title_number=0;
	int get=0;
	off64_t start=0;
	off64_t end=0;

	if(signal(SIGTERM, signal_callback_handler)==SIG_ERR || signal(SIGINT, signal_callback_handler)==SIG_ERR){
	        log_err(ERR_SIGNALS);
		return -1;
	}


	if(req->method==NULL){
		log_msg("Invalid request received: Request method undefined.");
		return -1;
	}
	
	if(req->base_url==NULL){
		log_msg("Invalid request received: Base url (e.g. cgi script url) undefined.");
		return -1;
	}

	if(strcmp(req->method,"HEAD") && strcmp(req->method,"GET")){
		header_unsupported(fd); //only support HEAD and GET
		return 0;
	}else if(0==strcmp(req->method,"GET")){
		get=1;
	}

	if(req->path_info==NULL){
		header_redirect_root(fd,req->base_url);
		return 0;
	}

	if(!strcmp(req->path_info,"")){
		header_redirect_root(fd,req->base_url);
		return 0;
	}


	if(req->http_range){
		if(0!=get_range(req->http_range,&start,&end)){
			log_msg("Invalid range request received:");
			log_msg(req->http_range);
			log_msg("Sending HTTP 400 Bad Request.");
			header_bad(fd);
			return 0;
		}
	}

	title_number=interpret_request(req->path_info); /* 0 for title listing, positive number for specific title, negative number for error*/
	
	if(0>title_number){
	        log_msg("The requested page:");
	        log_msg(req->path_info);
	        log_msg("...was not found.");
		header_notfound(fd);
		if(get) serve_notfound(fd);
	}
	if(0<=title_number){
		intRetVal=init(title_number,device_path);
		if (intRetVal!=ERR_OK){
		        log_err(intRetVal);
			header_notfound(fd);
			if(get) serve_notfound(fd);
			return intRetVal;
		}
		if(0==title_number){
			log_msg("Serving directory listing");
			header_dirlisting(fd);
			if(get) serve_dirlisting(fd);
		}else{
		        log_msg("Serving title:");
	                log_msg(req->path_info);
			header_title(fd,start,end);
			if(get) intRetVal=serve_title(fd,start,end);
		}
	}
	return intRetVal;
}
