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


int main(int argc, char *argv[]){

        char* dev_path="/dev/sr0";
        int test_fd=open(dev_path, O_RDONLY);
	if (test_fd == -1)
		log_err_die(ERR_NODEV);
	close(test_fd);
	
	struct streamdisc_http_request_s req;
	req.method=getenv("REQUEST_METHOD");
	req.base_url=getenv("SCRIPT_NAME");
	req.path_info=getenv("PATH_INFO");
	req.http_range=getenv("HTTP_RANGE");
	
	streamdisc_serve(STDOUT_FILENO, &req, dev_path); //never returns
}

	



