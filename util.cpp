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

#include "dtachez.hpp"

#include <cstdarg>

void write_all(int fd, const void *buf, size_t count) {
	size_t total_written = 0;
	while (total_written < count) {
		ssize_t bytes_written = write(fd, (uint8_t *)buf + total_written, count - total_written);
		if (bytes_written == -1) {
			if (errno == EINTR || errno == EAGAIN) {
				// Try again
				continue;
			} else {
				// Error occurred, return the error code
				THROW_ERROR("failed to write");
			}
		} else if (bytes_written == 0) {
			// End of file
			break;
		} else {
			total_written += bytes_written;
		}
	}

	if (count != total_written)
		THROW_ERROR("incomplete write");
}

void read_all(int fd, void *buf, size_t count) {
	size_t total_read = 0;
	while (total_read < count) {
		ssize_t bytes_read = read(fd, (uint8_t *)buf + total_read, count - total_read);
		if (bytes_read == -1) {
			if (errno == EINTR || errno == EAGAIN) {
				// Try again
				continue;
			} else {
				// Error occurred, return the error code
				THROW_ERROR("failed to read");
			}
		} else if (bytes_read == 0) {
			// End of file
			break;
		} else {
			total_read += bytes_read;
		}
	}

	if (count != total_read)
		THROW_ERROR("incomplete read");
}

int ensure_open(const char *s, int m) {
	int fd = open(s, m);

	if (fd < 0) {
		THROW_ERROR("open");
	}

	return fd;
}

void ensure_mkfifo(const char *s) {
	// Make it 0600 to prevent any surprises
	if (mkfifo(s, 0600)) {
		if (errno != EEXIST) {
			THROW_ERROR("mkfifo");
		}
	}
}

/* Sets a file descriptor to non-blocking mode. */
int setnonblocking(int fd) {
	int flags;

#if defined(O_NONBLOCK)
	flags = fcntl(fd, F_GETFL);
	if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return -1;
	return 0;
#elif defined(FIONBIO)
	flags = 1;
	if (ioctl(fd, FIONBIO, &flags) < 0)
		return -1;
	return 0;
#else
#warning Do not know how to set non-blocking mode.
	return 0;
#endif
}

char * __attribute__ ((__format__ (__printf__, 1, 2))) _str_fmt(const char *fmt, ...) {
	static char format_buf[PATH_MAX + 128];

	va_list ap;

	va_start(ap, fmt);
	vsnprintf(format_buf, sizeof(format_buf)-1, fmt, ap);
	va_end(ap);

	return format_buf;
}
