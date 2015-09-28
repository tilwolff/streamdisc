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
 
 struct streamdisc_http_request_s{
	char *method;         /* support GET and HEAD
	                      */
	char *base_url;     /* e.g. http://127.0.0.1/cgi-bin/streamdisc for cgi version, 
	                              http://127.0.0.1 for standalone 
	                      */
	char *path_info;      /* for title listing:  /
	                         for specific title: /titlexxxxx.[m2ts,vob] 
	                         if empty, redirect to / 
	                      */
	char *http_range;     /* bytes=[xxx]-[yyy] */
};

typedef struct streamdisc_http_request_s* streamdisc_http_request;

void streamdisc_serve(int /* fd*/, streamdisc_http_request, char* /*device path*/);
