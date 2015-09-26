/*
 * This file is part of libbluray
 * Copyright (C) 2009-2010  Obliter0n
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

#ifndef READDISC_H
#define READDISC_H

#define _LARGEFILE64_SOURCE

#include <libbluray/bluray.h>
#include <dvdread/dvd_reader.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define BD_BLOCK_SIZE       	6144
#define DVD_READ_AT_ONCE 	2048

enum {
	STATUS_DVD,
	STATUS_BD,
	STATUS_EMPTY
};

enum {
	TITLES_SELECT_AT_LEAST_20_PCT_OF_MAX_TITLE,
	TITLES_SELECT_AT_LEAST_20_MINUTES,
	TITLES_SELECT_ALL
};

enum {
	ERR_OK=0,
	ERR_END,
	ERR_NODEV,
	ERR_EMPTY,
	ERR_DVDOPEN,
	ERR_BDOPEN,
	ERR_READDVD,
	ERR_READBD,
	ERR_READ,
	ERR_FILEACCESS,
	ERR_FILEWRITE,
	ERR_NODISC,
	ERR_NO_TITLE_SELECTED,
	ERR_SIGNALS,
	ERR_FATAL
};

struct disc_device_s{
	BLURAY *bd;
	dvd_reader_t *dvd;
	dvd_file_t *dvd_title_file;
	int status;
	uint64_t current_title_size;
	uint32_t titles_count;
	uint64_t *title_sizes;
	uint32_t *filtered_title_list;
};

typedef struct disc_device_s* disc_device_t;

disc_device_t get_device_object(void);

void close_disc(disc_device_t);

int open_disc(disc_device_t, const char*);

int update_title_list(disc_device_t);

int set_titleset(disc_device_t, int);

int get_buffer_size(disc_device_t, int*);

// bytes_read = read_bytes(disc_device, offset, bytes_count, buf);
off64_t read_bytes( disc_device_t, off64_t, int, unsigned char * );

char* err2msg(int);

void log_err_die(int);
#endif

