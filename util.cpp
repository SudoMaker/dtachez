//
// Created by root on 2023/5/20.
//

#include "dtachez.hpp"

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
