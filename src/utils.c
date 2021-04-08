#include "utils.h"


int get_flag_by_mode(const char *mode)
{
	int flags = 0;

	if (!strcmp(mode, "a+"))
		flags = O_RDWR | O_CREAT | O_APPEND;
	else if (!strcmp(mode, "a"))
		flags = O_WRONLY | O_CREAT | O_APPEND;
	else if (!strcmp(mode, "w+"))
		flags = O_RDWR | O_CREAT | O_TRUNC;
	else if (!strcmp(mode, "w"))
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	else if (!strcmp(mode, "r+"))
		flags = O_RDWR;
	else if (!strcmp(mode, "r"))
		flags = O_RDONLY;
	else
		flags = -1;

	return flags;
}
