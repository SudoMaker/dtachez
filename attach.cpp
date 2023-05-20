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
    Copyright (C) 2004-2016 Ned T. Crigler

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
#include "dtachez.hpp"

#ifndef VDISABLE
#ifdef _POSIX_VDISABLE
#define VDISABLE _POSIX_VDISABLE
#else
#define VDISABLE 0377
#endif
#endif

/*
** The current terminal settings. After coming back from a suspend, we
** restore this.
*/
static struct termios cur_term;
/* 1 if the window size changed */
static int win_changed;

static uint8_t this_index;

/* Restores the original terminal settings. */
static void restore_term(void) {
	tcsetattr(0, TCSADRAIN, &orig_term);

	/* Make cursor visible. Assumes VT100. */
	printf("\033[?25h");
	fflush(stdout);
}

/* Connects to a unix domain socket */
static conn_pipes connect_pipes(const char *name) {
	auto do_open = [](const char *nom, int mode) {
		return ensure_open(nom, mode);
	};

	return conn_pipes{
		.fd_miso = do_open(str_fmt("%s_miso", name), O_WRONLY),
		.fd_mosi = do_open(str_fmt("%s_mosi", name), O_RDONLY),
	};
}

static conn_pipes request_and_connect(const char *name) {
	puts("note: if you see this message forever, check for stale pipe files");

	auto pmain = connect_pipes(name);

	uint8_t ctrl_byte = 1 << 7;

	write_all(pmain.fd_miso, &ctrl_byte, 1);
	read_all(pmain.fd_mosi, &this_index, 1);

	close(pmain.fd_miso);
	close(pmain.fd_mosi);

	if (this_index >= 127) {
		puts("error: server is full");
		exit(2);
	}

	return connect_pipes(str_fmt("%s_%u", name, this_index));
}

static void disconnect(const char *name) {
	auto pmain = connect_pipes(name);

	uint8_t ctrl_byte = this_index;

	write_all(pmain.fd_miso, &ctrl_byte, 1);
}

/* Signal */
static RETSIGTYPE die(int sig) {
	/* Print a nice pretty message for some things. */
	if (sig == SIGHUP || sig == SIGINT)
		printf(EOS "\r\n[detached]\r\n");
	else
		printf(EOS "\r\n[got signal %d - dying]\r\n", sig);
	disconnect(sockname);
	exit(1);
}

/* Window size change. */
static RETSIGTYPE win_change(int sig) {
	signal(SIGWINCH, win_change);
	win_changed = 1;
}

/* Handles input from the keyboard. */
static void process_kbd(int s, struct packet *pkt) {
	/* Suspend? */
	if (!no_suspend && (pkt->u.buf[0] == cur_term.c_cc[VSUSP]))
	{
		/* Tell the master that we are suspending. */
		pkt->type = MSG_DETACH;
		write(s, pkt, sizeof(struct packet));

		/* And suspend... */
		tcsetattr(0, TCSADRAIN, &orig_term);
		printf(EOS "\r\n");
		kill(getpid(), SIGTSTP);
		tcsetattr(0, TCSADRAIN, &cur_term);

		/* Tell the master that we are returning. */
		pkt->type = MSG_ATTACH;
		write(s, pkt, sizeof(struct packet));

		/* We would like a redraw, too. */
		pkt->type = MSG_REDRAW;
		pkt->len = redraw_method;
		ioctl(0, TIOCGWINSZ, &pkt->u.ws);
		write(s, pkt, sizeof(struct packet));
		return;
	}
	/* Detach char? */
	else if (pkt->u.buf[0] == detach_char)
	{
		printf(EOS "\r\n[detached]\r\n");
		disconnect(sockname);
		exit(0);
	}
	/* Just in case something pukes out. */
	else if (pkt->u.buf[0] == '\f')
		win_changed = 1;

	/* Push it out */
	write(s, pkt, sizeof(struct packet));
}

int attach_main(int noerror) {
	struct packet pkt;
	unsigned char buf[BUFSIZE];
	fd_set readfds;
	conn_pipes s;

	/* Attempt to open the socket. Don't display an error if noerror is 
	** set. */

	if (access(str_fmt("%s_miso", sockname), R_OK)) {
		if (!noerror) {
			perror("error: unable to open socket file");
		}
		return -1;
	}

	s = request_and_connect(sockname);

	/* The current terminal settings are equal to the original terminal
	** settings at this point. */
	cur_term = orig_term;

	/* Set a trap to restore the terminal when we die. */
	atexit(restore_term);

	/* Set some signals. */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGXFSZ, SIG_IGN);
	signal(SIGHUP, die);
	signal(SIGTERM, die);
	signal(SIGINT, die);
	signal(SIGQUIT, die);
	signal(SIGWINCH, win_change);

	/* Set raw mode. */
	cur_term.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	cur_term.c_iflag &= ~(IXON|IXOFF);
	cur_term.c_oflag &= ~(OPOST);
	cur_term.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	cur_term.c_cflag &= ~(CSIZE|PARENB);
	cur_term.c_cflag |= CS8;
	cur_term.c_cc[VLNEXT] = VDISABLE;
	cur_term.c_cc[VMIN] = 1;
	cur_term.c_cc[VTIME] = 0;
	tcsetattr(0, TCSADRAIN, &cur_term);

	/* Clear the screen. This assumes VT100. */
	write(1, "\33[H\33[J", 6);

	/* Tell the master that we want to attach. */
	memset(&pkt, 0, sizeof(struct packet));
	pkt.type = MSG_ATTACH;
	write(s.fd_miso, &pkt, sizeof(struct packet));

	/* We would like a redraw, too. */
	pkt.type = MSG_REDRAW;
	pkt.len = redraw_method;
	ioctl(0, TIOCGWINSZ, &pkt.u.ws);
	write(s.fd_miso, &pkt, sizeof(struct packet));

	/* Wait for things to happen */
	while (1) {
		int n;

		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(s.fd_mosi, &readfds);
		n = select(s.fd_mosi + 1, &readfds, NULL, NULL, NULL);
		if (n < 0 && errno != EINTR && errno != EAGAIN)
		{
			printf(EOS "\r\n[select failed]\r\n");
			exit(1);
		}

		/* Pty activity */
		if (n > 0 && FD_ISSET(s.fd_mosi, &readfds))
		{
			ssize_t len = read(s.fd_mosi, buf, sizeof(buf));

			if (len == 0)
			{
				printf(EOS "\r\n[EOF - dtach terminating]"
					"\r\n");
				exit(0);
			}
			else if (len < 0)
			{
				printf(EOS "\r\n[read returned an error]\r\n");
				exit(1);
			}
			/* Send the data to the terminal. */
			write(1, buf, len);
			n--;
		}
		/* stdin activity */
		if (n > 0 && FD_ISSET(0, &readfds))
		{
			ssize_t len;

			pkt.type = MSG_PUSH;
			memset(pkt.u.buf, 0, sizeof(pkt.u.buf));
			len = read(0, pkt.u.buf, sizeof(pkt.u.buf));

			if (len <= 0)
				exit(1);

			pkt.len = len;
			process_kbd(s.fd_miso, &pkt);
			n--;
		}

		/* Window size changed? */
		if (win_changed)
		{
			win_changed = 0;

			pkt.type = MSG_WINCH;
			ioctl(0, TIOCGWINSZ, &pkt.u.ws);
			write(s.fd_miso, &pkt, sizeof(pkt));
		}
	}
	return 0;
}

int
push_main()
{
	struct packet pkt;
	conn_pipes s;

	/* Attempt to open the socket. */
	s = request_and_connect(sockname);

	/* Set some signals. */
	signal(SIGPIPE, SIG_IGN);

	/* Push the contents of standard input to the socket. */
	pkt.type = MSG_PUSH;
	for (;;)
	{
		ssize_t len;

		memset(pkt.u.buf, 0, sizeof(pkt.u.buf));
		len = read(0, pkt.u.buf, sizeof(pkt.u.buf));

		if (len == 0)
			return 0;
		else if (len < 0)
		{
			printf("%s: %s: %s\n", progname, sockname,
			       strerror(errno));
			return 1;
		}

		pkt.len = len;
		if (write(s.fd_miso, &pkt, sizeof(struct packet)) < 0)
		{
			printf("%s: %s: %s\n", progname, sockname,
			       strerror(errno));
			return 1;
		}
	}
}
