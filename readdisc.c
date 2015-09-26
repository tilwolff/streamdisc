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

#define _LARGEFILE64_SOURCE

#include "readdisc.h"

struct {
    int value;
    char* name;
} error_codes[] = {
    { ERR_OK, "Everything is fine" },
    { ERR_END, "There is nothing more to do" },
    { ERR_NODEV, "Device node not accessible" },
    { ERR_EMPTY, "No DVD or BD disc present in device" },
    { ERR_DVDOPEN, "Cannot open DVD. Check if DVD present" },
    { ERR_BDOPEN, "Cannot open BD. Check if BD present" },
    { ERR_READDVD, "Could not read DVD title" },
    { ERR_READBD, "Could not read BD title" },
    { ERR_READ, "Could not read from disc" },
    { ERR_FILEACCESS, "Could not open file" },
    { ERR_FILEWRITE, "Could not write to file" },
    { ERR_NODISC, "No disc has been opened" },
    { ERR_NO_TITLE_SELECTED, "No title has been selected" },
    { ERR_SIGNALS, "Could not register signal handler" },
    { ERR_FATAL, "A fatal unhandled error occurred." },
    { 0, 0 }
};


disc_device_t get_device_object(void){
	disc_device_t obj=malloc( sizeof (struct disc_device_s) );
	if(NULL==obj) return NULL;
	
	obj->status=STATUS_EMPTY;
	obj->bd=NULL;
	obj->dvd=NULL;
	obj->dvd_title_file=NULL;
	obj->current_title_size=0;
	obj->title_sizes=NULL;
	obj->filtered_title_list=NULL;
	
	return obj;
}

void close_disc(disc_device_t dd){
	if (NULL==dd) return;
	if (STATUS_EMPTY==dd->status) return;
	if (STATUS_DVD==dd->status){
		if(dd->dvd_title_file){
			DVDCloseFile(dd->dvd_title_file);
			dd->dvd_title_file=NULL;
		}
		DVDClose(dd->dvd);
		dd->dvd=NULL;
	}
	if (STATUS_BD==dd->status){
		bd_close(dd->bd);
		dd->bd=NULL;
	}
	dd->status=STATUS_EMPTY;
	dd->current_title_size=0;
	free(dd->title_sizes);
	dd->title_sizes=NULL;
	free(dd->filtered_title_list);
	dd->filtered_title_list=NULL;
}

int open_disc(disc_device_t dd, const char* chFile){
	if (STATUS_EMPTY!=dd->status) return ERR_OK;
	
	//try DVD
	int intRetVal=ERR_EMPTY;
	dvd_stat_t dvdstat;		
	dd->dvd=DVDOpen(chFile);
	if(dd->dvd){
		if(0==DVDFileStat(dd->dvd, 1, DVD_READ_TITLE_VOBS, &dvdstat)){
			dd->status=STATUS_DVD;						
			return ERR_OK;
		}else{
		dd->dvd=NULL;
		intRetVal=ERR_DVDOPEN;
		}
	}
	
	//try BD
	int bd_title_count=0;

	dd->bd = bd_open(chFile, NULL);
	if(dd->bd){
		bd_title_count = bd_get_titles(dd->bd, TITLES_RELEVANT, 120);
		if(bd_title_count){
			dd->status=STATUS_BD;						
			return ERR_OK;
		}else{
		dd->bd=NULL;
		intRetVal=ERR_BDOPEN;
		}
	}
	return intRetVal;
}

int set_titleset(disc_device_t dd, int intTitleSet){
	if (intTitleSet<=0) return ERR_FATAL;
	if (STATUS_DVD==dd->status){
		dvd_stat_t dvdstat;
		if(0!=DVDFileStat(dd->dvd, intTitleSet, DVD_READ_TITLE_VOBS, &dvdstat))
			return (1==intTitleSet) ? ERR_READDVD : ERR_END;
		dd->current_title_size=dvdstat.size;
		dd->dvd_title_file=DVDOpenFile( dd->dvd, intTitleSet, DVD_READ_TITLE_VOBS );
		return (0==dd->dvd_title_file) ? ERR_READDVD : ERR_OK;
	}
	else if (STATUS_BD==dd->status){
		int bd_title_count = bd_get_titles(dd->bd, TITLES_RELEVANT, 120);
		if(!bd_title_count)
			return ERR_READBD;
		if(bd_title_count<intTitleSet)
			return ERR_END;		
		if(!bd_select_title(dd->bd, intTitleSet-1))
			return ERR_READBD;
		dd->current_title_size=(off64_t) bd_get_title_size(dd->bd);
		return ERR_OK;
	}
	else{
		return ERR_NODISC;
	}
}

int update_title_list(disc_device_t dd){
	uint32_t i=1;	
	if (STATUS_DVD==dd->status){
		uint32_t array_size=10;
		dd->title_sizes=malloc(array_size*sizeof(uint64_t));
		dvd_stat_t dvdstat;
		while(0==DVDFileStat(dd->dvd, i, DVD_READ_TITLE_VOBS, &dvdstat)){
			if (i>array_size){
				array_size*=2;
				dd->title_sizes=realloc(dd->title_sizes,array_size*sizeof(uint64_t));
			}		
			dd->title_sizes[i-1]=dvdstat.size;
			i++;
		}
		dd->title_sizes=realloc(dd->title_sizes,i*sizeof(uint64_t));
		if (1==i) return ERR_READDVD;		
		dd->titles_count=i-1;
		return ERR_OK;
	}
	else if (STATUS_BD==dd->status){
		dd->titles_count = bd_get_titles(dd->bd, TITLES_RELEVANT, 120);
		if(!dd->titles_count)
			return ERR_READBD;
		dd->title_sizes=malloc(dd->titles_count*sizeof(uint64_t));
		
		while ((uint32_t)i<=dd->titles_count){
			if(!bd_select_title(dd->bd, i-1))
				return ERR_READBD;
			dd->title_sizes[i-1]=(off64_t) bd_get_title_size(dd->bd);
			i++;
		}
		return ERR_OK;
	}
	else{
		return ERR_NODISC;
	}
}


off64_t read_bytes(disc_device_t dd, off64_t bytes_offset, int bytes_to_read, unsigned char * buf){
	if (bytes_offset>=(off64_t)dd->current_title_size)
		return -ERR_END;
	unsigned char* tmpbuf;
	if (STATUS_DVD==dd->status && dd->current_title_size){


		int blocks_start=bytes_offset / DVD_VIDEO_LB_LEN;
		off64_t bytes_start_raster=blocks_start*DVD_VIDEO_LB_LEN;

		int blocks_end=(bytes_offset + bytes_to_read) / DVD_VIDEO_LB_LEN;
		blocks_end+= ((bytes_offset + bytes_to_read) % DVD_VIDEO_LB_LEN !=0 ? 1 :0);

		size_t blocks_to_read=blocks_end-blocks_start;
		tmpbuf=(unsigned char*)malloc(blocks_to_read*DVD_VIDEO_LB_LEN*sizeof(unsigned char));		
		off64_t blocks_read = DVDReadBlocks(dd->dvd_title_file, blocks_start, blocks_to_read, tmpbuf);
		if(-1==blocks_read){
			free(tmpbuf);
			return -ERR_READDVD;
		}
		
		if( bytes_start_raster+(DVD_VIDEO_LB_LEN* blocks_read) < bytes_offset + bytes_to_read){
			bytes_to_read=bytes_start_raster+(DVD_VIDEO_LB_LEN*blocks_read)-bytes_offset;
		}	

		off64_t mem_offset=bytes_offset-bytes_start_raster;
		memcpy(buf, tmpbuf+mem_offset,bytes_to_read);
		free(tmpbuf);
		return bytes_to_read;
	}

	else if (STATUS_BD==dd->status  && dd->current_title_size){
		int read_res;
		int64_t tell=bd_tell(dd->bd);
		if(tell!=bytes_offset){
			int64_t seek_res=bd_seek(dd->bd, (uint64_t)bytes_offset);
			if(seek_res>(int64_t) bytes_offset){
				return -ERR_READBD;
			}
			off64_t seek_offset=bytes_offset-(off64_t)seek_res;
			//READ AND DISCARD OFFSET	
			if (seek_offset>0){
				tmpbuf=(unsigned char*)malloc(seek_offset*sizeof(unsigned char));
				read_res=bd_read(dd->bd, tmpbuf, seek_offset);
				free(tmpbuf);
				if(read_res!= seek_offset){
					return -ERR_READBD;
				}
			}
		}
		read_res=bd_read(dd->bd, buf, bytes_to_read); //READ WHAT IS REQUESTED
		if(-1==read_res){
			return -ERR_READBD;
		}
		return read_res;
	}
	else if( dd->current_title_size){
		return -ERR_NODISC;
	}
	else{
		return -ERR_NO_TITLE_SELECTED;
	}
}

int get_buffer_size(disc_device_t dd, int* res){
	if (STATUS_DVD==dd->status){
		*res=DVD_VIDEO_LB_LEN*512;
		return ERR_OK;
	}
	else if (STATUS_BD==dd->status){
		*res=6144*512;
		return ERR_OK;
	}
	else{
		*res=0;
		return ERR_NODISC;
	}
}




char* err2msg(int err_code)
{
	int i;
	for (i = 0; error_codes[i].name; ++i){
		if (error_codes[i].value == err_code){
			return error_codes[i].name;
		}
	}
	char* default_msg="An unknown error occured";
	return default_msg;
}


void log_err_die(int err_code){
	char* msg=err2msg(err_code);
	fprintf(stderr,"%s\nExiting.\n",msg);
	exit (err_code);
}
	



