#include "../util/so_stdio.h"
#include "utils.h"

#define BUFFER_SIZE (4096)

struct _so_file{
	/* buff [ _left_ptr; _right_ptr ) */
	/* is valid for reading/ writing */
	char _buff[BUFFER_SIZE];

	/* file system file descriptor */
	int _fd;

	/* last operation made on fp: r/w/m */
	int _last_op;

	/* file system file offset  relative to the beginning */
	int _file_offset;

	/* buffer left offset*/
	int _left_ptr;

	/* buffer right offset */
	int _right_ptr;

	/* if this is set to non-zero value then somewthing went wrong */
	int _err;

	/* if this is set to non-zero value then EOF was reached */
	int _eof;
};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	int fd;
	int flags;
	int open_mode = 0644;
	SO_FILE *fp = NULL;

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
	fp->_last_op = NONE_OP;
	return fp;
}

/*
 * closes the stream
 *
 */
int so_fclose(SO_FILE *stream)
{
	so_fflush(stream);

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
	size_t count = 0;
	int rc = 0;

	if (stream->_last_op == READ_OP) {
		// printf("[fflush]: after READ_OP??\n");
	}

	/* best effort to write to the disk
	 */
	while(stream->_left_ptr < stream->_right_ptr) {
		count = stream->_right_ptr - stream->_left_ptr;
		rc = write(stream->_fd,
			   stream->_buff + stream->_left_ptr,
			   count);
		if (rc == -1) {
			stream->_err = 1;
			return SO_EOF;
		} else if (rc == 0) {
			printf("[fputc]: we wrote 0 bytes, we are stuck\n");
		} else {
			stream->_left_ptr += rc;
		}
	}

	/* flushing the buffer should cound as a NONE_OP
	 * and we reset the pointers
	 */
	stream->_last_op = NONE_OP;
	stream->_left_ptr = 0;
	stream->_right_ptr = 0;
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
	size_t items_read = 0;
	int read_byte = 0;
	char *buff = calloc(size, sizeof(char));

	if (!buff) {
		stream->_err = 1;
		return 0;
	}

	/* so we attempt to read each and every byte with so_fgetc
	 *
	 */
	for (items_read = 0; items_read < nmemb; items_read++) {
		for (size_t j = 0; j < size; j++) {
			
			read_byte = so_fgetc(stream);
			if (read_byte == SO_EOF) {
				goto exit_so_fread;
			}
			buff[j] = (char)read_byte;
		}

		memcpy(ptr + items_read * size, buff, size);
	}

exit_so_fread:
	free(buff);
	stream->_last_op = READ_OP;
	return items_read;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{	

	((char*)ptr)[0] = '\0';
	

	return 0;
}

int so_fgetc(SO_FILE *stream)
{
	int rc = 0;
	int read_character = 0;
	int read_len = 0;


	if (stream->_last_op == WRITE_OP) {
		// TODO: flush the buffer
		printf("\n\n\tidk bruh\n\n\n\n");
		return SO_EOF;
	}

	/* the buffer is `empty` and we have to read something from the actual
	 * file from the disk;
	 *
	 * everytime we try to read the whole buffer
	 * 
	 * whenever _left_ptr hits _right_ptr we reset _left_ptr to 0
	 * because we will try to read as much as possible from one read
	 * 
	 * we avoid trying to fill up the buffer if _left_ptr == _right_ptr
	 * > 0 because that read could be attempted with a size small enough
	 * to not help us in the long run
	 */
	if (stream->_left_ptr == stream->_right_ptr) {
		// if (stream->_left_ptr == BUFFER_SIZE) {
		// 	stream->_left_ptr = 0;
		// } else {
		// 	// nothing yet
		// }

		stream->_left_ptr = 0;
		read_len = BUFFER_SIZE - stream->_left_ptr;
		rc = read(stream->_fd, stream->_buff, read_len);
		// printf("citim [%d] primim [%d]\n", read_len, rc);
		if (rc == -1) {
			stream->_err = 1;
			return SO_EOF;
		} else if (rc == 0) {
			stream->_eof = 1;
			return SO_EOF;
		}
		stream->_right_ptr = stream->_left_ptr + rc;
	}

	/* _left_ptr is only incremented by one
	 *
	 */
	read_character = (unsigned char)stream->_buff[stream->_left_ptr];

	/* real file offset only gets incremented by one
	 * because only one character is read fromt the file to the user
	 */
	stream->_file_offset++;
	stream->_left_ptr++;
	stream->_last_op = READ_OP;
	return read_character;
}

int so_fputc(int c, SO_FILE *stream)
{	
	char chr = c;
	if (stream->_last_op == READ_OP) {
		// TODO: invalidate the buffer but dont return EOF just yet
		printf("\n\n\tidk bruh - fputc\n\n\n\n");
		return SO_EOF;
	}

	/* here i'm assuming the buffer is either invalidated or last
	 * operation was a write operation (that writes to the buffer)
	 */
	if (stream->_last_op != READ_OP) {
		// printf("[fputc]: READ_OP????\n");
	}

	/* when the buffer is used for writing there is no need for _left_ptr
	 * because we'll only flush at when the buffer is full or when we are
	 * explicitly told to do so
	 */
	stream->_buff[stream->_right_ptr++] = chr;

	if (stream->_right_ptr == BUFFER_SIZE - 1) {
		if(so_fflush(stream) == SO_EOF) {
			return SO_EOF;
		}
	}



	return c;
}

int so_feof(SO_FILE *stream)
{
	return stream->_eof;
}

int so_ferror(SO_FILE *stream)
{
	return stream->_err;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	return NULL;
}

int so_pclose(SO_FILE *stream)
{
	return 0;
}
