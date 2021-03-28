#include "../util/so_stdio.h"
#include "utils.h"

#define BUFFER_SIZE (4096)

struct _so_file{
	/* buff[_left_ptr; _right_ptr) */
	/* is valid for reading/ writing */
	char _buff[BUFFER_SIZE];

	/* file system file descriptor */
	int _fd;

	/* last operation made on fp: r/w/m */
	int _last_op;

	/* file system file offset */
	int _file_offset;

	/* buffer left offset*/
	int _left_ptr;

	/* buffer right offset */
	int _right_ptr;

};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd;
	int flags;
	int open_mode = 0644;
	SO_FILE* fp = NULL;

	flags = get_flag_by_mode(mode);
	if (flags == -1)
		return NULL;

	fd = open(pathname, flags, open_mode);
	if (fd == -1)
		return NULL;

	fp = calloc(1, sizeof(SO_FILE));
	if (!fp) {
		close(fd);
		return NULL;
	}

	fp->_fd = fd;

	return fp;
}

/*
 * closes the stream
 *
 */
int so_fclose(SO_FILE *stream)
{
	if (close(stream->_fd) == -1)
		return -1;

	free(stream);
	return 0;
}

int so_fileno(SO_FILE *stream)
{
	return stream->_fd;
}

int so_fflush(SO_FILE *stream)
{
	return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{
	return 0;
}

long so_ftell(SO_FILE *stream)
{
	return 0;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	return 0;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	return 0;
}

int so_fgetc(SO_FILE *stream)
{
	int rc = 0;
	int read_char = 0;
	int read_len = 0;

	/* the buffer is empty and we have to read something from the actual
	 * file from the disk 
	 */
	if (stream->_left_ptr == stream->_right_ptr) {
		if (stream->_left_ptr == BUFFER_SIZE) {
			stream->_left_ptr = 0;
		} else {
			// nothing yet
		}

		read_len = BUFFER_SIZE - stream->_left_ptr;

		rc = read(stream->_fd, stream->_buff, read_len);
		if (rc == -1 || rc == 0)
			return SO_EOF;
		stream->_right_ptr = stream->_left_ptr + rc;
		stream->_file_offset += rc;
		// printf("Am citit cu un read [%d] bytes.\n", rc);
	}

	read_char = stream->_buff[stream->_left_ptr];
	stream->_left_ptr++;
	stream->_last_op = READ_OP;

	return read_char;
}

int so_fputc(int c, SO_FILE *stream)
{
	return 0;
}

int so_feof(SO_FILE *stream)
{
	return 0;
}

int so_ferror(SO_FILE *stream)
{
	return 0;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	return NULL;
}

int so_pclose(SO_FILE *stream)
{
	return 0;
}
