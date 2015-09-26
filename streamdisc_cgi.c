/*
 * This file is part of streamdisc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "readdisc.h"
#include <errno.h>
#include <ctype.h>


#define NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"
#define NOT_SUPPORTED "<html><head><title>Not Supported</title></head><body>The method you are trying to use is not supported by this server</body></html>"
#define DIRLISTING_OPEN "<html><head><title>Directory Listing</title></head><body><h2>Index of /titles</h2><div class=\"list\"><table summary=\"Directory Listing\" cellpadding=\"2px\" cellspacing=\"2px\"><thead><tr><th class=\"n\">Name</th><th class=\"m\">Last Modified</th><th class=\"s\">Size</th><th class=\"t\">Type</th></tr></thead><tbody>\n"
#define DIRLISTING_CLOSE "</tbody></table></div></body></html>\n"
#define DIRLISTING_ITEM	"<tr><td class=\"n\"><a href=\"%s\">%s</a></td><td class=\"m\">%s</td><td class=\"s\">%.1f%s</td><td class=\"t\">%s</td></tr>\n"



int abort_requested=0;
const char *dev_path="/dev/sr0";
disc_device_t dd_disc;
char *script_name;

void
signal_callback_handler(int signum){
	abort_requested=1;
}

int init(int32_t title_number){
	int intRetVal=ERR_FATAL;
	dd_disc=get_device_object();

	/* open device*/
	intRetVal=open_disc(dd_disc,dev_path);
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

	if ('\0'==*pos)							//invalid range
		return -1;
	if  isdigit(*pos){						//first value given, we are in the case bytes=nnn-[mmm]	
		errno=0;
		result=strtoull(pos,&pos,0);				//strtoull discards whitespace
		if (errno){ 
			fprintf(stderr,"Error");
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
}

void header_notfound(){
	printf("HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n");
}

void header_bad(){
	printf("HTTP/1.1 400 Bad Reqest\n\n");
}

void header_unsupported(){
	printf("HTTP/1.1 401 Not Supported\n\n");
}

void header_dirlisting(){
	printf("HTTP/1.1 200 OK\nContent-Type: text/html\n\n");	
}

void header_redirect_root(){
	printf("HTTP/1.1 302 Found\nLocation: %s/\n\n",script_name);//location needs slash in the end in order to be interpreted as directory
}

void header_title(off64_t range_start, off64_t range_end){
	
	if(range_end>dd_disc->current_title_size-1 || 0==range_end)
		range_end=dd_disc->current_title_size-1;

	if(range_start<0)
		range_start=dd_disc->current_title_size+range_start-1>=0 ? dd_disc->current_title_size+range_start-1 : 0;
	

	/* mime types */
	const char* tp_bd="video/MP2T";	
	const char* tp_dvd="video/dvd";
	const char* tp= STATUS_DVD==dd_disc->status ? tp_dvd : tp_bd;
	if(range_start==0 && range_end==dd_disc->current_title_size-1){
		printf("HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %zd\nAccept-Ranges: bytes\n\n",tp,dd_disc->current_title_size);
	}else if(range_start>dd_disc->current_title_size-1){
		printf("HTTP/1.1 416 Requested range not satisfiable\n\n");
	}else{
		printf("HTTP/1.1 206 Partial content\nContent-Type:%s\nContent-Length: %zd\nAccept-Ranges: bytes\nContent-Range: %zd-%zd/%zd\n\n",\
		        tp,range_end-range_start+1,range_start,range_end,dd_disc->current_title_size);
	}
}

void serve_notfound(){
	printf(NOT_FOUND);
}



void serve_dirlisting(){
	printf(DIRLISTING_OPEN);
	
	/* filename */	
	const char* fn_pattern_bd="title%05d.m2ts";	
	const char* fn_pattern_dvd="title%05d.vob";
	const char* fn_pattern= STATUS_DVD==dd_disc->status ? fn_pattern_dvd : fn_pattern_bd;
	char fn[1024];
	/* mime types */
	const char* tp_bd="video/MP2T";	
	const char* tp_dvd="video/dvd";
	const char* tp= STATUS_DVD==dd_disc->status ? tp_dvd : tp_bd;
	/* last modified */
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
		printf(DIRLISTING_ITEM,fn,fn,lm,sz,unit,tp); //html item
		i++;
	}

	printf(DIRLISTING_CLOSE);
}

int serve_title(off64_t range_start, off64_t range_end){
	
	
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

	unsigned char* buf=malloc(intBufSizeBytes*sizeof(unsigned char));
	off64_t bytes_read=0;
	off64_t bytes_to_read=0;
	while(bytes_total>0 && 0==abort_requested){
		bytes_to_read=(bytes_total>intBufSizeBytes)? intBufSizeBytes: bytes_total;	
		bytes_read=read_bytes( dd_disc, offset, bytes_to_read, buf );
		if(bytes_read<0){
			free(buf);
			return ERR_READ;
		}
		
		if(fwrite(buf, 1, bytes_read,stdout)!=(size_t)bytes_read){
			free(buf);
			return ERR_FILEWRITE;
		}
		bytes_total-=bytes_read;
		offset+=bytes_read;


	}

	fflush(stdout);
	return ERR_OK;
}



int main(int argc, char *argv[]){


	if(signal(SIGTERM, signal_callback_handler)==SIG_ERR || signal(SIGINT, signal_callback_handler)==SIG_ERR)
		log_err_die(ERR_SIGNALS);

	if (access(dev_path, F_OK) == -1)
		log_err_die(ERR_NODEV);

	int intRetVal=ERR_OK;
	setenv("HOME","/tmp",0); // set HOME to temp folder IF IT IS NOT SET. This enables dvdcss etc. to use cache
	char *request_method=getenv("REQUEST_METHOD");
	script_name=getenv("SCRIPT_NAME");
	char *path_info=getenv("PATH_INFO");
	char *http_range=getenv("HTTP_RANGE");
	int get=0;
	off64_t start=0;
	off64_t end=0;

	if(request_method==NULL || script_name==NULL){
		fprintf(stderr,"Invalid request received: Request method or script name undefined.\nExiting.\n");
		return -1;
	}

	if(strcmp(request_method,"HEAD") && strcmp(request_method,"GET")){
		header_unsupported(); //only support HEAD and GET
		return 0;
	}
	else if(0==strcmp(request_method,"GET")){
		get=1;
	}

	if(path_info==NULL){
		header_redirect_root();
		return 0;
	}

	if(!strcmp(path_info,"")){
		header_redirect_root();
		return 0;
	}


	if(http_range){
		if(0!=get_range(http_range,&start,&end)){
			fprintf(stderr,"Invalid range request received: %s\nSending HTTP 400 Bad Request\n",http_range);
			header_bad();
			return 0;
		}
	}

	int32_t title_number=interpret_request(path_info); /* 0 for title listing, positive number for specific title, negative number for error*/
	

	if(0>title_number){
		header_notfound();
		if(get) serve_notfound();
	}
	if(0<=title_number){
		intRetVal=init(title_number);
		if (intRetVal!=ERR_OK){
			header_notfound();
			if(get) serve_notfound();
			return intRetVal;
		}
		if(0==title_number){
			header_dirlisting();
			if(get) serve_dirlisting();
		}else{
			header_title(start,end);
			if(get) intRetVal=serve_title(start,end);
		}
	}
	return intRetVal;		
}

	



