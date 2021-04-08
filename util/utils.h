#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

enum op {
	NONE_OP,
	READ_OP,
	WRITE_OP,
	MOVE_OP
};


int get_flag_by_mode(const char *mode);



#endif /* #define UTILS_H */