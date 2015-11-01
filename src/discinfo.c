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

#define _LARGEFILE64_SOURCE

#include "readdisc.h"


void usage(const char *prog_name)
{
printf("\nUsage:\n   %s <device file>\n\n", prog_name);
}

int main(int argc, char *argv[]){
	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	int intRetVal=ERR_FATAL;

	disc_device_t dd_disc=get_device_object();

	/* open device*/
	intRetVal=open_disc(dd_disc,argv[1]);
	if(ERR_OK!=intRetVal)
		log_err_die(intRetVal);

	/* update title list*/
	intRetVal=update_title_list(dd_disc);
	if(ERR_OK!=intRetVal)
		log_err_die(intRetVal);

	/* print title info */
	uint32_t i=1;
	while (i<=dd_disc->titles_count){
		double size=(double)dd_disc->title_sizes[i-1];
		
		char *unit="B";
		if(size>1024){
			size/=1024;
			unit="K";
		}

		if(size>1024){
			size/=1024;
			unit="M";
		}
		
		if(size>1024){
			size/=1024;
			unit="G";
		}
		printf("Title: %d\t , size %7.2f%s\n",i,size,unit);
		i++;
	}
	
	return intRetVal;	
}
