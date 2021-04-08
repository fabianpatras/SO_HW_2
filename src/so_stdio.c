#include "../util/so_stdio.h"
#include "utils.h"

#define BUFFER_SIZE (4096)

struct _so_file {
	/* buff [ _left_ptr ; _right_ptr ) */
	/* is valid for reading/ writing */
	char _buff[BUFFER_SIZE];

	/* file system file descriptor */
	int _fd;

	/* last operation made on fp: r/w/m */
	int _last_op;

	/* file system file offset  relative to the beginning */
	off_t _file_offset;

	/* buffer left offset*/
	int _left_ptr;

	/* buffer right offset */
	int _right_ptr;

	/* if this is set to non-zero value then somewthing went wrong */
	int _err;

	/* if this is set to non-zero value then EOF was reached */
	int _eof;

	/* pid of process if SO_FILE is associated with a process */
	pid_t _pid;
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

int so_fclose(SO_FILE *stream)
{
	int rc1 = 0;
	int rc2 = 0;

	if (stream->_pid) {
		printf("FILE is a process\n");
		return -1;
	}

	rc1 = so_fflush(stream);
	rc2 = close(stream->_fd);

	free(stream);
	if (rc1 == -1 || rc2 == -1)
		return -1;


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

	if (stream->_last_op == READ_OP)
		goto so_fflush_end;


	/* best effort to write to the disk
	 */
	while (stream->_left_ptr < stream->_right_ptr) {
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
			stream->_file_offset += rc;
		}
	}

so_fflush_end:
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
	off_t ret_offset = -1;
	off_t delta = 0;
	int rc = 0;

	/* checking if we have to invalidate the internal buffer or to
	 * flush it
	 */
	if (stream->_last_op == READ_OP) {
		/* if the last op was a read op and the buffer is not empty
		 * then there was something read from the file descriptor
		 * and the lseek would return a value that is not the true
		 * location of the disk (fd) file offset
		 */
		if (stream->_left_ptr < stream->_right_ptr) {
			delta = stream->_right_ptr - stream->_left_ptr;
			stream->_left_ptr = 0;
			stream->_right_ptr = 0;
		}
	} else if (stream->_last_op == WRITE_OP) {
		rc = so_fflush(stream);
		if (rc == SO_EOF)
			return -1;

	}

	if (whence == SEEK_SET || whence == SEEK_END) {
		ret_offset = lseek(stream->_fd, offset, whence);
	} else if (whence == SEEK_CUR) {
		/* in this case we'll have to take into account the delta
		 * that might be > 0 from the fact the buffer still had
		 * some data available in the buffer from an old read
		 */
		ret_offset = lseek(stream->_fd, offset - delta, whence);
	}
	if (ret_offset == (off_t)-1) {
		stream->_err = 1;
		return -1;
	}
	stream->_file_offset = ret_offset;


	return 0;
}

long so_ftell(SO_FILE *stream)
{
	int rc = so_fseek(stream, 0, SEEK_CUR);

	if (rc == 0)
		return lseek(stream->_fd, 0, SEEK_CUR);


	return -1;
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
			if (read_byte == SO_EOF)
				goto exit_so_fread;

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
	size_t i = 0;
	size_t j = 0;
	int rc = 0;
	unsigned char data = 0;

	stream->_last_op = WRITE_OP;

	for (i = 0; i < nmemb; i++) {
		for (j = 0; j < size; j++) {
			data = ((unsigned char *)ptr)[i * size + j];
			rc = so_fputc((int)data, stream);

			if (rc == SO_EOF)
				return i;
		}
	}

	return nmemb;
}

int so_fgetc(SO_FILE *stream)
{
	int rc = 0;
	int read_character = 0;
	int read_len = 0;


	if (stream->_last_op == WRITE_OP)
		return SO_EOF;

	stream->_last_op = READ_OP;

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

		stream->_left_ptr = 0;
		read_len = BUFFER_SIZE - stream->_left_ptr;
		rc = read(stream->_fd, stream->_buff, read_len);
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

	if (stream->_last_op == READ_OP)
		return SO_EOF;

	/* when the buffer is used for writing there is no need for _left_ptr
	 * because we'll only flush at when the buffer is full or when we are
	 * explicitly told to do so
	 */
	stream->_buff[stream->_right_ptr++] = chr;

	if (stream->_right_ptr == BUFFER_SIZE - 1)
		if (so_fflush(stream) == SO_EOF)
			return SO_EOF;

	stream->_last_op = WRITE_OP;

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
	pid_t pid;
	SO_FILE *fp = NULL;
	int pipe_fd[2];
	int parent_fd;

	if (pipe(pipe_fd))
		return NULL;

	pid = fork();

	if (pid == -1) {
		return NULL;
	} else if (pid == 0) {
		// child process
		// if *type == 'r' we read from the process so we close the
		// reading end of the pipe in the children
		if (*type == 'r') {
			// the child doesn't read, it writes
			close(pipe_fd[0]);
			dup2(pipe_fd[1], STDOUT_FILENO);
		} else if (*type == 'w') {
			// the child doesn't write, it reads
			close(pipe_fd[1]);
			dup2(pipe_fd[0], STDIN_FILENO);
		} else {
			printf("Wrong type popen\n");
			return NULL;
		}

		execlp("sh", "sh", "-c", command, NULL);
		// error

		return NULL;
	}

	if (*type == 'r') {
		// the parent doesn't write, it reads
		close(pipe_fd[1]);
		parent_fd = pipe_fd[0];
	} else if (*type == 'w') {
		// the parent doesn't read, it writes
		close(pipe_fd[0]);
		parent_fd = pipe_fd[1];
	} else {
		printf("Wrong type popen\n");
		return NULL;
	}

	fp = calloc(1, sizeof(SO_FILE));
	if (!fp)
		return NULL;

	fp->_pid = pid;
	fp->_last_op = NONE_OP;
	fp->_fd = parent_fd;

	return fp;
}

int so_pclose(SO_FILE *stream)
{
	int ret_code = 0;
	int rc1;
	int rc2;
	pid_t ret_pid;

	rc1 = so_fflush(stream);
	rc2 = close(stream->_fd);

	ret_pid = waitpid(stream->_pid, &ret_code, 0);

	if (rc1 == -1 || rc2 == -1 || ret_pid == -1) {
		free(stream);
		return -1;
	}

	free(stream);
	return ret_code;
}
