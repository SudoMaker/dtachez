/*
    This file is part of dtachez.

    Copyright (C) 2023 SudoMaker, Ltd.
    Author: Reimu NotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    This program is based on dtach, which was originally released under
    the GPLv2 license by Ned T. Crigler.

    Below is the previous license header.
*/

/*
    dtach - A simple program that emulates the detach feature of screen.
    Copyright (C) 2001, 2004-2016 Ned T. Crigler

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cinttypes>
#include <climits>
#include <ctime>

#include "config.h"

#include <pty.h>

#include <fcntl.h>

#define THROW_ERROR(s)		{puts(s); exit(2);}

#ifdef HAVE_UTIL_H
#include <util.h>
#endif

#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#include <unistd.h>

#include <sys/ioctl.h>

#include <sys/resource.h>

#include <termios.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISSOCK
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif

extern char *progname, *sockname;
extern int detach_char, no_suspend, redraw_method;
extern struct termios orig_term;
extern int dont_have_tty;

enum {
	MSG_PUSH	= 0,
	MSG_ATTACH	= 1,
	MSG_DETACH	= 2,
	MSG_WINCH	= 3,
	MSG_REDRAW	= 4,
};

enum {
	REDRAW_UNSPEC	= 0,
	REDRAW_NONE	= 1,
	REDRAW_CTRL_L	= 2,
	REDRAW_WINCH	= 3,
};

/* The client to master protocol. */
struct packet {
	unsigned char type;
	unsigned char len;
	union {
		unsigned char buf[sizeof(struct winsize)];
		struct winsize ws;
	} u;
};

struct conn_pipes {
	int fd_miso, fd_mosi;
};

/*
** The master sends a simple stream of text to the attaching clients, without
** any protocol. This might change back to the packet based protocol in the
** future. In the meantime, however, we minimize the amount of data sent back
** and forth between the client and the master. BUFSIZE is the size of the
** buffer used for the text stream.
*/
#define BUFSIZE 4096

/* This hopefully moves to the bottom of the screen */
#define EOS "\033[999H"

int attach_main(int noerror);
int master_main(char **argv, int waitattach, int dontfork);
int push_main(void);

extern int setnonblocking(int fd);
extern void write_all(int fd, const void *buf, size_t count);
extern void read_all(int fd, void *buf, size_t count);
extern int ensure_open(const char *s, int m);
extern void ensure_mkfifo(const char *s);
extern char *_str_fmt(const char *fmt, ...) __attribute__ ((__format__ (__printf__, 1, 2)));

#define str_fmt(...) strdupa(_str_fmt(__VA_ARGS__))

#ifdef sun
#define BROKEN_MASTER
#endif
